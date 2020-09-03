#include "support/files.h"

#include <climits>
#include <unistd.h>

ssize_t readall(int fd, void* buf, size_t count) {
  if (count > SSIZE_MAX) {
    return -1;
  }
  ssize_t signed_count = count;

  ssize_t bytes_read = 0;
  while (bytes_read != signed_count) {
    ssize_t bytes_read_last_time = read(fd, (char*)buf + bytes_read, signed_count - bytes_read);
    if (bytes_read_last_time < 0) {
      return -1;
    }

    bytes_read += bytes_read_last_time;
  }

  return bytes_read;
}

ssize_t writeall(int fd, const void* buf, size_t count) {
  if (count > SSIZE_MAX) {
    return -1;
  }
  ssize_t signed_count = count;

  ssize_t bytes_written = 0;
  while (bytes_written != signed_count) {
    ssize_t bytes_written_last_time = write(fd, (char*)buf + bytes_written, signed_count - bytes_written);
    if (bytes_written_last_time < 0) {
      return -1;
    }

    bytes_written += bytes_written_last_time;
  }

  return bytes_written;
}
