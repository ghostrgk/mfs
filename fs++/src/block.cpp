#include "fs++/internal/block.h"

#include <cassert>
#include <utility>

namespace fspp::internal {

[[maybe_unused]] Blocks::Blocks(const uint64_t* block_num_ptr, uint64_t* free_block_num_ptr, uint8_t* bit_set_bytes,
                                Block* block_bytes)
    : blocks_ptr_(block_bytes), free_block_num_ptr_(free_block_num_ptr), bit_set_(*block_num_ptr, bit_set_bytes) {
}

Blocks::Blocks(void* blocks_ptr_start, uint64_t* free_block_num_ptr, BitSet blocks_bitset)
    : blocks_ptr_(static_cast<Block*>(blocks_ptr_start)),
      free_block_num_ptr_(free_block_num_ptr),
      bit_set_(std::move(blocks_bitset)) {
}

int Blocks::createBlock(id_t* created_id) {
  if (*free_block_num_ptr_ == 0) {
    return -1;
  }

  --(*free_block_num_ptr_);

  id_t new_block_id = bit_set_.findCleanBit();
  bit_set_.setBit(new_block_id);

  *created_id = new_block_id;
  return 0;
}

Block& Blocks::getBlockById(uint64_t block_id) {
  return blocks_ptr_[block_id];
}

int Blocks::deleteBlock(id_t block_id) {
  if (!bit_set_.getBit(block_id)) {
    return -1;
  }

  bit_set_.clearBit(block_id);
  ++(*free_block_num_ptr_);
  
  return 0;
}

uint64_t Blocks::getFreeBlockNum() {
  return *free_block_num_ptr_;
}

}  // namespace fspp::internal