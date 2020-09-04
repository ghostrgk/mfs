#include "support/network.h"

#include <arpa/inet.h>

void swap_bytes(uint8_t* lhs, uint8_t* rhs) {
  uint8_t tmp = *lhs;
  *lhs = *rhs;
  *rhs = tmp;
}

uint64_t hton64(uint64_t host64) {
  uint32_t host_one = 1;
  if (htonl(host_one) == host_one) {
    return host64;
  }

  for (int i = 0; i < 4; ++i) {
    auto* bytes = reinterpret_cast<uint8_t*>(&host64);
    swap_bytes(bytes + i, bytes + 7 - i);
  }

  return host64;
}

uint64_t ntoh64(uint64_t net64) {
  return net64;
}