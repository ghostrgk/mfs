#pragma once

#include "bitset.h"
#include "config.h"

typedef struct Block {
  uint8_t bytes[BLOCK_SIZE];
} Block;

struct Blocks {
  Block* blocks_ptr_;
  uint64_t* free_block_num_ptr_;
  struct BitSet bit_set_;
};

int Blocks_init(struct Blocks* blocks, void* blocks_ptr_start, uint64_t* free_block_num_ptr,
                struct BitSet blocks_bitset);

Block* getBlockById(struct Blocks* blocks, uint64_t block_id);
int createBlock(struct Blocks* blocks, id_t* created_id);
int deleteBlock(struct Blocks* blocks, id_t block_id);

uint64_t getFreeBlockNum(struct Blocks* blocks);
