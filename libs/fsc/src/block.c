#include "fsc/internal/block.h"

int Blocks_init(struct Blocks* blocks, void* blocks_ptr_start, uint64_t* free_block_num_ptr,
                struct BitSet blocks_bitset) {
  blocks->blocks_ptr_ = blocks_ptr_start;
  blocks->free_block_num_ptr_ = free_block_num_ptr;
  blocks->bit_set_ = blocks_bitset;
  return 0;
}

Block* getBlockById(struct Blocks* blocks, uint64_t block_id) {
  return &(blocks->blocks_ptr_[block_id]);
}

int createBlock(struct Blocks* blocks, id_t* created_id) {
  if (*blocks->free_block_num_ptr_ == 0) {
    return -1;
  }

  --(*blocks->free_block_num_ptr_);

  id_t new_block_id = findCleanBit(&(blocks->bit_set_));
  setBit(&(blocks->bit_set_), new_block_id);

  *created_id = new_block_id;
  return 0;
}

int deleteBlock(struct Blocks* blocks, id_t block_id) {
  if (!getBit(&(blocks->bit_set_), block_id)) {
    return -1;
  }

  clearBit(&(blocks->bit_set_), block_id);
  ++(*blocks->free_block_num_ptr_);

  return 0;
}

uint64_t getFreeBlockNum(struct Blocks* blocks) {
  return *blocks->free_block_num_ptr_;
}
