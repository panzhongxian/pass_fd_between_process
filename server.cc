#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <sys/un.h>
#include <unistd.h>

#include <string>

extern "C" {
#include "apue_send_recv_fd/apue.h"
}

using std::string;

struct event_base* base;
int g_server_port;

string get_uds_name(bool pair_flag) {
  int port = pair_flag ? (g_server_port ^ 1) : g_server_port;
  return "test_server_port_01_" + std::to_string(port);
}

void do_tcp_write(evutil_socket_t fd, short, void* arg) {
  auto event_self = static_cast<event*>(arg);
  char buffer[256];
  int len = snprintf(buffer, sizeof(buffer), "rsp from port: %d\n", g_server_port);
  if (send(fd, buffer, len - 1, 0) < 0) {
    perror("send");
    _exit(0);
  }

  sockaddr_un un{};
  un.sun_family = AF_UNIX;
  strcpy(un.sun_path, get_uds_name(true).c_str());
  int uds_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (connect(uds_fd, (sockaddr*)&un, sizeof(sockaddr_un)) < 0) {
    perror("connect uds");
  }
  send_fd(uds_fd, fd);
  close(uds_fd);
  close(fd);
  event_del(event_self);
}

void do_tcp_read(evutil_socket_t fd, short, void* arg) {
  auto event_self = static_cast<event*>(arg);
  char len = 0;
  int ret = recv(fd, &len, 1, 0);
  if (ret < 0) {
    perror("recv1");
    _exit(0);
  } else if (ret == 0) {
    event_del(event_self);
    shutdown(fd, SHUT_RDWR);
    close(fd);
    return;
  }
  char buffer[256];
  char remaining = len;

  while (true) {
    int curr_len = recv(fd, buffer, remaining, 0);
    if (curr_len < 0) {
      // TODO(jasonzxpan) EAGAIN
      perror("recv2");
      _exit(0);
    } else if (curr_len == 0) {
      close(fd);
      return;
    } else {
      remaining -= curr_len;
      if (remaining == 0) {
        break;
      }
    }
  }
  printf("len: %d, content: %.*s\n", len, len, buffer);

  // 删除 读事件，避免无法结束
  auto write_event = event_new(base, fd, EV_WRITE, do_tcp_write, event_self_cbarg());
  event_add(write_event, NULL);
  event_del(event_self);
  // TODO(jasonzxpan) send to another server
}

void do_tcp_accept(evutil_socket_t listener, short event, void* arg) {
  sockaddr_storage ss{};
  socklen_t slen = sizeof(ss);
  int fd = accept(listener, (struct sockaddr*)&ss, &slen);
  if (fd < 0) {
    perror("accept");
  } else if (fd > FD_SETSIZE) {
    close(fd);
  } else {
    evutil_make_socket_nonblocking(fd);
    auto read_event = event_new(base, fd, EV_READ, do_tcp_read, event_self_cbarg());
    event_add(read_event, NULL);
  }
}

void do_uds_read(evutil_socket_t fd, short, void*) {
  int tcp_fd = recv_fd(fd, write);
  if (tcp_fd < 0) {
    perror("accept");
  } else if (tcp_fd > FD_SETSIZE) {
    close(tcp_fd);
  } else {
    close(fd);
    evutil_make_socket_nonblocking(tcp_fd);
    auto read_event = event_new(base, tcp_fd, EV_READ, do_tcp_read, event_self_cbarg());
    event_add(read_event, NULL);
  }
}

void do_uds_accept(evutil_socket_t listener, short, void*) {
  sockaddr_un un{};
  socklen_t slen = sizeof(un);
  int fd = accept(listener, (struct sockaddr*)&un, &slen);
  if (fd < 0) {
    perror("accept");
  } else if (fd > FD_SETSIZE) {
    close(fd);
  } else {
    evutil_make_socket_nonblocking(fd);
    auto read_event = event_new(base, fd, EV_READ, do_uds_read, event_self_cbarg());
    event_add(read_event, NULL);
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
  sin.sin_addr.s_addr = 0;
  sin.sin_port = htons(g_server_port);
  int tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
  evutil_make_socket_nonblocking(tcp_fd);
  if (bind(tcp_fd, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
    perror("bind tcp port");
    return 0;
  }
  if (listen(tcp_fd, 100) < 0) {
    perror("listen on tcp port");
    return 0;
  }

  auto tcp_listen_event = event_new(base, tcp_fd, EV_READ | EV_PERSIST, do_tcp_accept, (void*)base);
  event_add(tcp_listen_event, NULL);

  // 创建UDS
  sockaddr_un un{};
  un.sun_family = AF_UNIX;
  strcpy(un.sun_path, get_uds_name(false).c_str());
  int uds_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (uds_fd < 0) {
    perror("uds_fd");
    return 0;
  }
  evutil_make_socket_nonblocking(uds_fd);
  int size = offsetof(struct sockaddr_un, sun_path) + strlen(un.sun_path);
  if (bind(uds_fd, (struct sockaddr*)&un, size) < 0) {
    perror("bind uds address");
    return 0;
  }
  if (listen(uds_fd, 100) < 0) {
    perror("listen on uds address");
    return 0;
  }

  auto uds_read_event = event_new(base, uds_fd, EV_READ | EV_PERSIST, do_uds_accept, (void*)base);
  event_add(uds_read_event, NULL);

  event_base_dispatch(base);

  return 0;
}
