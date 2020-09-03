#pragma once

#include <iostream>
#include <string>

#include <cstring>

#define LOG_USER_ERROR(msg) std::cerr << (msg) << std::endl;
#define LOG(msg) std::cout << (msg) << std::endl

#define LOG_ERROR(msg) std::cerr << "ERROR: " << msg << std::endl
#define LOG_INFO(msg) std::cerr << "INFO: " << msg << std::endl
#define LOG_ERROR_WITH_ERRNO_MSG(msg) \
  std::cerr << "ERROR: " << msg << " (errno message=" << std::string(strerror(errno)) << ")" << std::endl
