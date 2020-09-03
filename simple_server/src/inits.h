#pragma once

#include <cstdint>

int init_socket(uint16_t port);
int init_server_epoll(int listen_fd);