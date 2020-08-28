#pragma once

#include "bitset.h"
#include "config.h"

namespace fspp::internal {

struct Block {
  uint8_t bytes[BLOCK_SIZE];
};

class BlockSpace {
 public:
  BlockSpace() = default;
  BlockSpace(uint64_t* block_num_ptr, uint64_t* free_block_num_ptr, uint8_t* bit_set_bytes, Block* block_bytes);

  Block& getBlockById(uint64_t block_id);

  uint64_t createBlock();
  void deleteBlock(uint64_t block_id);

  uint64_t getFreeBlockNum();

 private:
  uint64_t* block_num_ptr_{nullptr};
  uint64_t* free_block_num_ptr_{nullptr};
  BitSet bit_set_{};

  Block* blocks_{nullptr};
};

}  // namespace fspp::internal