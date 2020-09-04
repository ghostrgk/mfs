#include "support/network.h"

#include <arpa/inet.h>

static void swap_bytes(uint8_t* lhs, uint8_t* rhs) {
  uint8_t tmp = *lhs;
  *lhs = *rhs;
  *rhs = tmp;
}

static void change_byte_order64(uint64_t* num_ptr) {
  for (int i = 0; i < 4; ++i) {
    auto* bytes = reinterpret_cast<uint8_t*>(num_ptr);
    swap_bytes(bytes + i, bytes + 7 - i);
  }
}

uint64_t hton64(uint64_t host64) {
  uint32_t host_one = 1;
  if (htonl(host_one) != host_one) {
    change_byte_order64(&host64);
  }

  return host64;
}

uint64_t ntoh64(uint64_t net64) {
  uint32_t net_one = 1;
  if (ntohl(net_one) != net_one) {
    change_byte_order64(&net64);
  }

  return net64;
}