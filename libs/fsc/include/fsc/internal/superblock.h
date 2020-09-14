#pragma once

#include <stddef.h>

#include "block.h"
#include "inode.h"

struct SuperBlock {
  uint64_t block_num;
  uint64_t free_block_num;
  uint64_t inode_num;
  uint64_t free_inode_num;
};

size_t FileSystemSize(struct SuperBlock* superblock);
uint64_t InodeBitSetOffset(struct SuperBlock* superblock);
uint64_t BlockBitSetOffset(struct SuperBlock* superblock);
uint64_t InodesOffset(struct SuperBlock* superblock);
uint64_t BlocksOffset(struct SuperBlock* superblock);
