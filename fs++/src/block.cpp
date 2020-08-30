#include "fs++/internal/block.h"

#include <cassert>

namespace fspp::internal {

Blocks::Blocks(const uint64_t* block_num_ptr, uint64_t* free_block_num_ptr, uint8_t* bit_set_bytes,
                       Block* block_bytes)
    : /*block_num_ptr_(block_num_ptr),*/
      free_block_num_ptr_(free_block_num_ptr),
      bit_set_(*block_num_ptr, bit_set_bytes),
      block_bytes_(block_bytes) {
}

id_t Blocks::createBlock() {
  assert(*free_block_num_ptr_ != 0);
  --(*free_block_num_ptr_);

  uint64_t id = bit_set_.findCleanBit();
  bit_set_.setBit(id);

  return id;
}

Block& Blocks::getBlockById(uint64_t block_id) {
  return block_bytes_[block_id];
}

void Blocks::deleteBlock(uint64_t block_id) {
  assert(bit_set_.getBit(block_id));

  bit_set_.clearBit(block_id);
  ++(*free_block_num_ptr_);
}

uint64_t Blocks::getFreeBlockNum() {
  return *free_block_num_ptr_;
}

}  // namespace fspp::internal