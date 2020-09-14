#include "fsc/internal/bitset.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static uint8_t* getByteByIndex(struct BitSet* bit_set, uint64_t index) {
  if (index >= bit_set->element_num_) {
    return NULL;
  }

  return &(bit_set->bytes_[index / 8]);
}

static uint8_t bitmaskByBitIndex(uint8_t bit_index) {
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
      abort();
  }
}

int BitSet_init(struct BitSet* bit_set, uint64_t element_num, uint8_t* bytes) {
  assert(element_num % 8 == 0);
  if (bytes == NULL) {
    uint64_t bitset_size = element_num / 8;
    bytes = malloc(sizeof(uint8_t) * bitset_size);
    memset(bytes, 0, bitset_size);
    bit_set->has_ownership_ = true;
  } else {
    bit_set->has_ownership_ = false;
  }

  bit_set->bytes_ = bytes;
  bit_set->element_num_ = element_num;

  return 0;
}

void BitSet_free(struct BitSet* bit_set) {
  if (bit_set->has_ownership_) {
    free(bit_set->bytes_);
  }
}

void setBit(struct BitSet* bit_set, uint64_t index) {
  uint8_t* byte_ptr = getByteByIndex(bit_set, index);
  assert(byte_ptr != NULL);

  *byte_ptr |= bitmaskByBitIndex(index % 8);
}

void clearBit(struct BitSet* bit_set, uint64_t index) {
  uint8_t* byte_ptr = getByteByIndex(bit_set, index);
  assert(byte_ptr != NULL);

  uint8_t inverted_mask = ~bitmaskByBitIndex(index % 8);
  *byte_ptr &= inverted_mask;
}

uint8_t getBit(struct BitSet* bit_set, uint64_t index) {
  uint8_t* byte_ptr = getByteByIndex(bit_set, index);
  assert(byte_ptr != NULL);

  uint8_t bit = *byte_ptr & bitmaskByBitIndex(index % 8);
  return (bit == 0) ? 0 : 1;
}

uint64_t findCleanBit(struct BitSet* bit_set) {
  for (uint64_t index = 0; index < bit_set->element_num_ / 8; ++index) {
    if (bit_set->bytes_[index] == 0xff) {
      continue;
    }

    for (uint64_t bit_index = 0; bit_index < 8; ++bit_index) {
      uint64_t exact_index = index * 8 + bit_index;
      if (getBit(bit_set, exact_index) == 0) {
        return exact_index;
      }
    }
  }

  abort();
}
