#pragma once

#include <cassert>
#include <cstdint>

#include "block.h"

namespace fspp::internal {

class InodesList {
 public:
  /*!
   * @param blocks blocks of filesystem
   * @param block_id id of the block that contains ids of other blocks
   * @param index index of id in the block
   * @return block id from block
   */
  static uint64_t& resolveIndirection(BlockSpace* blocks, uint64_t block_id, uint64_t index) {
    return ((uint64_t*)blocks->getBlockById(block_id).bytes)[index];
  }

  uint64_t getBlockIdByIndex(BlockSpace* blocks, uint64_t index);

  [[nodiscard]] uint64_t size() const {
    return size_;
  }

  [[nodiscard]] static uint64_t max_size() {
    return INODE_MAX_BLOCK_COUNT;
  }
  void clear() {
    size_ = 0;
  }

  void setSize(uint64_t new_size) {
    size_ = new_size;
  }

  int addBlock(BlockSpace* blocks, uint64_t block_id);

 private:
  uint64_t size_{0};

  uint64_t block_ids_[ILIST_ZERO_INDIRECTION]{};
  uint64_t level1_id{0};
  uint64_t level2_id{0};
};

}  // namespace fspp::internal