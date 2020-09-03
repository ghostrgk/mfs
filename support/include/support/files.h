#pragma once

#include <cstdlib>

ssize_t readall(int fd, void *buf, size_t count);
ssize_t writeall(int fd, const void *buf, size_t count);
