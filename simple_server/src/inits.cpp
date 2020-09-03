#include "inits.h"
#include "support.h"

#include <cstdio>
#include <netinet/ip.h>
#include <sys/epoll.h>

int init_socket(uint16_t port) {
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);

  if (server_fd < 0) {
    LOG_ERROR_WITH_ERRNO_MSG("socket creation failed");
    return -1;
  }

  struct sockaddr_in address = {.sin_family = AF_INET, .sin_port = htons(port)};
  address.sin_addr.s_addr = INADDR_ANY;

  if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
    LOG_ERROR_WITH_ERRNO_MSG("socket bind failed");
    return -1;
  }

  if (listen(server_fd, SOMAXCONN) < 0) {
    LOG_ERROR_WITH_ERRNO_MSG("setting socket to listen mode failed");
    return -1;
  }

  return server_fd;
}

int init_server_epoll(int listen_fd) {
  int epoll_fd = epoll_create1(0);
  if (epoll_fd < 0) {
    LOG_ERROR_WITH_ERRNO_MSG("epoll creation failed");
    return -1;
  }

  struct epoll_event event = {.events = EPOLLIN};
  event.data.fd = listen_fd;
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &event) < 0) {
    LOG_ERROR_WITH_ERRNO_MSG("adding server fd to epoll queue failed");
    return -1;
  }

  return epoll_fd;
}
