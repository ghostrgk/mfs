#pragma once

#include <cassert>
#include <cstdint>

#include "block.h"
#include "compiler.h"

namespace fspp::internal {

class InodesList {
 public:
  /*!
   * @param blocks blocks of filesystem
   * @param block_id id of the block that contains ids of other blocks
   * @param index index of id in the block
   * @return block id from block
   */
  static id_t& resolveIndirection(Blocks* blocks, uint64_t block_id, uint64_t index) {
    return ((uint64_t*)blocks->getBlockById(block_id).bytes)[index];
  }

  id_t getBlockIdByIndex(Blocks* blocks, uint64_t index);

  [[nodiscard]] uint64_t size() const {
    return size_;
  }

  [[nodiscard]] static uint64_t max_size() {
    return INODE_MAX_BLOCK_COUNT;
  }

  void clear() {
    size_ = 0;
  }

  int addBlock(Blocks* blocks, id_t block_id);

  [[maybe_unused]] uint64_t BlocksNeededToAddBlocks(uint64_t additional_blocks_count) {
    FSC_USED_BY_ASSERT(additional_blocks_count);
    return 0;
  }

 private:
  uint64_t size_{0};

  id_t block_ids_[ILIST_ZERO_INDIRECTION]{};
  id_t level1_id_{0};
  id_t level2_id_{0};
};

}  // namespace fspp::internal