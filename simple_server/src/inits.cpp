#include "inits.h"

#include <cstdio>
#include <netinet/ip.h>

int init_socket(uint16_t port) {
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);

  if (server_fd < 0) {
    perror("Can't create socket");
    return -1;
  }

  struct sockaddr_in address = {.sin_family = AF_INET, .sin_port = htons(port)};
  address.sin_addr.s_addr = INADDR_ANY;

  if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
    perror("Can't bind socket");
    return -1;
  }

  if (listen(server_fd, SOMAXCONN) < 0) {
    perror("Can't set socket to listen mode");
    return -1;
  }

  return server_fd;
}
