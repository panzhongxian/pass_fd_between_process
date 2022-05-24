#include <arpa/inet.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cassert>
#include <string>

struct event_base* base;
int g_server_port;
constexpr int kRepeateTime = 1000;
constexpr int kClientAmount = 100;

struct Client {
  int counter;
  int last_server_port;
  event* ptr;
  std::string buffer;
};

void do_read(evutil_socket_t fd, short, void* arg);
void do_write(evutil_socket_t fd, short, void* arg);

void do_write(evutil_socket_t fd, short, void* arg) {
  auto client = static_cast<Client*>(arg);
  char buffer[12] = {"01234567890"};
  buffer[0] = strlen(buffer) - 1;
  for (int i = 0; i < kRepeateTime; i++) {
    if (send(fd, buffer, 11, 0) < 0) {
      close(fd);
      return;
    }
  }

  auto read_event = event_new(base, fd, EV_READ | EV_TIMEOUT | EV_PERSIST, do_read, arg);
  client->ptr = read_event;
  struct timeval three_seconds = {3, 0};
  event_add(read_event, &three_seconds);
}

void do_check(Client* client) {
  auto pos = client->buffer.find_first_of("0123456789");
  int counter = 0;
  int last_port = g_server_port ^ 1;
  int port;
  auto next_pos = pos;

  while (pos != std::string::npos) {
    next_pos = client->buffer.find_first_not_of("0123456789", pos);
    counter++;
    if (next_pos == std::string::npos) {
      port = atoi(client->buffer.substr(pos).c_str());
      break;
    }
    port = atoi(client->buffer.substr(pos, next_pos - pos).c_str());
    pos = client->buffer.find_first_of("0123456789", next_pos);
    assert((port ^ last_port) == 1);
    last_port = port;
  }
  assert(counter == kRepeateTime);
}
void do_read(evutil_socket_t fd, short event, void* arg) {
  auto client = static_cast<Client*>(arg);
  auto event_self = client->ptr;

  if (event & EV_TIMEOUT) {
    do_check(client);
    event_del(event_self);
    return;
  }
  char buffer[1024];
  while (true) {
    int ret = recv(fd, buffer, sizeof(buffer), 0);
    if (ret < 0) {
      if (errno == EAGAIN) {
        return;
      }
      perror("recv");
      event_del(event_self);
      _exit(0);
    } else if (ret == 0) {
      do_check(client);
      close(fd);
      event_del(event_self);
      return;
    } else {
      client->buffer += std::string(buffer, ret);
    }
  }
}

int main(int argc, char* argv[]) {
  g_server_port = atoi(argv[1]);

  struct sockaddr_in sin {};
  base = event_base_new();
  if (!base) {
    return 1;
  }

  // 创建socket监听TCP端口
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = inet_addr("127.0.0.1");
  sin.sin_port = htons(g_server_port);
  Client clients[kClientAmount];
  for (int i = 0; i < kClientAmount; i++) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    evutil_make_socket_nonblocking(fd);
    int ret = connect(fd, (sockaddr*)&sin, sizeof(sin));
    if (ret != 0 && errno != EINPROGRESS) {
      perror("connect");
      return 0;
    }
    auto connect_event = event_new(base, fd, EV_WRITE, do_write, clients + i);
    clients[i] = Client{0, (g_server_port ^ 1), connect_event};
    event_add(connect_event, NULL);
  }

  event_base_dispatch(base);

  return 0;
}
