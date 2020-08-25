#pragma once

#include "block.h"
#include "inode.h"

namespace fspp::internal {

struct SuperBlock {
  uint64_t block_num{0};
  uint64_t free_block_num{0};
  uint64_t inode_num{0};
  uint64_t free_inode_num{0};

  [[nodiscard]] std::size_t FileSystemSize() const {
    return BlocksOffset() + sizeof(Block) * block_num;
  }

  [[nodiscard]] uint64_t InodeBitSetOffset() const {
    return sizeof(SuperBlock);
  }

  [[nodiscard]] uint64_t BlockBitSetOffset() const {
    return InodeBitSetOffset() + inode_num / 8;
  }

  [[nodiscard]] uint64_t InodesOffset() const {
    return BlockBitSetOffset() + block_num / 8;
  }

  [[nodiscard]] uint64_t BlocksOffset() const {
    return InodesOffset() + sizeof(Inode) * inode_num;
  }
};

}  // namespace fspp::internal
