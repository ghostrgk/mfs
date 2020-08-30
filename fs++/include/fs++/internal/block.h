#pragma once

#include "bitset.h"
#include "config.h"

namespace fspp::internal {

struct Block {
  uint8_t bytes[BLOCK_SIZE];
};

static_assert(sizeof(Block) == BLOCK_SIZE);

class Blocks {
 public:
  Blocks() = default;
  [[maybe_unused]] Blocks(const uint64_t* block_num_ptr, uint64_t* free_block_num_ptr, uint8_t* bit_set_bytes,
                          Block* block_bytes);
  Blocks(void* blocks_ptr_start, uint64_t* free_block_num_ptr, BitSet blocks_bitset);

  Block& getBlockById(uint64_t block_id);

  // todo: maybe it should indicate fail
  uint64_t createBlock();
  void deleteBlock(uint64_t block_id);

  uint64_t getFreeBlockNum();

 private:
  Block* blocks_ptr_{nullptr};

  uint64_t* free_block_num_ptr_{nullptr};
  BitSet bit_set_{};
};

}  // namespace fspp::internal