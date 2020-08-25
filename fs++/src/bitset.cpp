#include "fs++/internal/bitset.h"

#include <cstdlib>
#include <cstring>

namespace fspp::internal {

BitSet::BitSet(uint64_t element_num) : element_num_(element_num) {
  assert(element_num_ % 8 == 0);

  uint64_t bitset_size = element_num_ / 8;
  bytes_ = new uint8_t[bitset_size];
  memset(bytes_, 0, bitset_size);
}

BitSet::BitSet(uint64_t element_num, uint8_t* bytes)
    : has_ownership_(false), element_num_(element_num), bytes_(bytes) {
  assert(element_num_ % 8 == 0);
}

BitSet::~BitSet() {
  if (has_ownership_) {
    delete[] bytes_;
  }
}

void BitSet::setBit(uint64_t index) {
  uint8_t* byte_ptr = getByteByIndex(index);
  assert(byte_ptr != nullptr);

  *byte_ptr |= bitmaskByBitIndex(index % 8);
}

void BitSet::clearBit(uint64_t index) {
  uint8_t* byte_ptr = getByteByIndex(index);
  assert(byte_ptr != nullptr);

  uint8_t inverted_mask = ~bitmaskByBitIndex(index % 8);
  *byte_ptr &= inverted_mask;
}

uint8_t BitSet::getBit(uint64_t index) {
  uint8_t* byte_ptr = getByteByIndex(index);
  assert(byte_ptr != nullptr);

  uint8_t bit = *byte_ptr & bitmaskByBitIndex(index % 8);
  return (bit == 0) ? 0 : 1;
}

uint64_t BitSet::findCleanBit() {
  for (uint64_t index = 0; index < element_num_ / 8; ++index) {
    if (bytes_[index] == 0xff) {
      continue;
    }

    for (uint64_t bit_index = 0; bit_index < 8; ++bit_index) {
      uint64_t exact_index = index * 8 + bit_index;
      if (getBit(exact_index) == 0) {
        return exact_index;
      }
    }
  }

  std::abort();
}

uint8_t BitSet::bitmaskByBitIndex(uint8_t bit_index) {
  switch (bit_index) {
    case 0:
      return 0x1;
    case 1:
      return 0x2;
    case 2:
      return 0x4;
    case 3:
      return 0x8;
    case 4:
      return 0x10;
    case 5:
      return 0x20;
    case 6:
      return 0x40;
    case 7:
      return 0x80;
    default:
      std::abort();
  }
}

uint8_t* BitSet::getByteByIndex(uint64_t index) {
  if (index >= element_num_) {
    return nullptr;
  }

  return &bytes_[index / 8];
}

BitSet::BitSet(BitSet&& other) noexcept {
  has_ownership_ = other.has_ownership_;
  element_num_ = other.element_num_;
  bytes_ = other.bytes_;

  other.has_ownership_ = false;
  other.element_num_ = 0;
  other.bytes_ = nullptr;
}

BitSet& BitSet::operator=(BitSet&& other) noexcept {
  if (has_ownership_) {
    delete[] bytes_;
  }

  has_ownership_ = other.has_ownership_;
  element_num_ = other.element_num_;
  bytes_ = other.bytes_;

  other.has_ownership_ = false;
  other.element_num_ = 0;
  other.bytes_ = nullptr;

  return *this;
}

BitSet::BitSet() {
  has_ownership_ = false;
  element_num_ = 0;
  bytes_ = nullptr;
}

}  // namespace fspp::internal
