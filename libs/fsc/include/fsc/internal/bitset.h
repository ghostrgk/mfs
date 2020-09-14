#pragma once

#include <stdbool.h>
#include <stdint.h>

struct BitSet {
  bool has_ownership_;
  uint64_t element_num_;
  uint8_t* bytes_;
};

int BitSet_init(struct BitSet* bit_set, uint64_t element_num, uint8_t* bytes);
void BitSet_free(struct BitSet* bit_set);

void setBit(struct BitSet* bit_set, uint64_t index);
void clearBit(struct BitSet* bit_set, uint64_t index);
uint8_t getBit(struct BitSet* bit_set, uint64_t index);
uint64_t findCleanBit(struct BitSet* bit_set);

