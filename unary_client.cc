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
  int index;
  int counter;
  int last_server_port;
  event* ptr;
};

void do_read(evutil_socket_t fd, short, void* arg);
void do_write(evutil_socket_t fd, short, void* arg);

void do_write(evutil_socket_t fd, short, void* arg) {
  auto client = static_cast<Client*>(arg);
  char buffer[12] = {"01234567890"};
  buffer[0] = strlen(buffer) - 1;
  if (send(fd, buffer, 11, 0) < 0) {
    close(fd);
    return;
  }

  auto read_event = event_new(base, fd, EV_READ, do_read, arg);
  client->ptr = read_event;
  event_add(read_event, NULL);
}

void do_read(evutil_socket_t fd, short, void* arg) {
  auto client = static_cast<Client*>(arg);
  auto event_self = client->ptr;
  char buffer[256];
  int ret = recv(fd, buffer, sizeof(buffer), 0);
  if (ret < 0) {
    if (errno == EAGAIN) {
      event_add(event_self, NULL);
      return;
    }
    perror("recv");
    _exit(0);
  } else if (ret == 0) {
    close(fd);
    return;
  }
  client->counter++;
  if (client->counter == kRepeateTime) {
    return;
  }
  std::string s(buffer, ret);
  int port = atoi(s.substr(s.find_first_of("0123456789")).c_str());
  if (client->index == 0) {
    printf("port: %d, last_port: %d\n", port, client->last_server_port);
  }
  assert((port ^ client->last_server_port) == 1);
  client->last_server_port = port;
  auto write_event = event_new(base, fd, EV_WRITE, do_write, arg);
  client->ptr = write_event;
  event_add(write_event, NULL);
}

int main(int argc, char* argv[]) {
  g_server_port = atoi(argv[1]);

  struct sockaddr_in sin {};
  base = event_base_new();
  if (!base) {
    return 1;
  }

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
    clients[i] = Client{i, 0, (g_server_port ^ 1), connect_event};
    event_add(connect_event, NULL);
  }

  event_base_dispatch(base);

  return 0;
}
