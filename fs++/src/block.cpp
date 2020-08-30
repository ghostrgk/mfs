#include "fs++/internal/block.h"

#include <cassert>
#include <utility>

namespace fspp::internal {

[[maybe_unused]] Blocks::Blocks(const uint64_t* block_num_ptr, uint64_t* free_block_num_ptr, uint8_t* bit_set_bytes, Block* block_bytes)
    : blocks_ptr_(block_bytes), free_block_num_ptr_(free_block_num_ptr), bit_set_(*block_num_ptr, bit_set_bytes) {
}

Blocks::Blocks(void* blocks_ptr_start, uint64_t* free_block_num_ptr, BitSet blocks_bitset)
    : blocks_ptr_(static_cast<Block*>(blocks_ptr_start)),
      free_block_num_ptr_(free_block_num_ptr),
      bit_set_(std::move(blocks_bitset)) {
}

id_t Blocks::createBlock() {
  assert(*free_block_num_ptr_ != 0);
  --(*free_block_num_ptr_);

  uint64_t id = bit_set_.findCleanBit();
  bit_set_.setBit(id);

  return id;
}

Block& Blocks::getBlockById(uint64_t block_id) {
  return blocks_ptr_[block_id];
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