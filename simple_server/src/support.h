#pragma once

#include <iostream>
#include <string>

#include <cstdint>
#include <cstring>

#define LOG_USER_ERROR(msg) std::cerr << (msg) << std::endl;
#define LOG(msg) std::cout << (msg) << std::endl
#define LOG_ERROR(msg) std::cerr << "ERROR: " << msg << std::endl
#define LOG_INFO(msg) std::cerr << "INFO: " << msg << std::endl
#define LOG_ERROR_WITH_ERRNO_MSG(msg) \
  std::cerr << "ERROR: " << msg << " (errno message=" << std::string(strerror(errno)) << ")" << std::endl

#define SIMPLE_SERV_HANDLE_ERROR(msg) \
  do {                                \
    perror(msg);                      \
    std::abort();                     \
  } while (false)

const uint64_t MAX_QUERY_LEN = 4096;
const uint64_t MAX_EPOLL_EVENTS = 10;
const uint64_t PORT = 8800;