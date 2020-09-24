#include "fsc/internal/superblock.h"

size_t FileSystemSize(struct SuperBlock* superblock) {
  return BlocksOffset(superblock) + sizeof(Block) * superblock->block_num;
}

uint64_t InodeBitSetOffset(struct SuperBlock* superblock) {
  FSC_UNUSED(superblock);
  return sizeof(struct SuperBlock);
}

uint64_t BlockBitSetOffset(struct SuperBlock* superblock) {
  return InodeBitSetOffset(superblock) + superblock->inode_num / 8;
}

uint64_t InodesOffset(struct SuperBlock* superblock) {
  return BlockBitSetOffset(superblock) + superblock->block_num / 8;
}

uint64_t BlocksOffset(struct SuperBlock* superblock) {
  return InodesOffset(superblock) + sizeof(Inode) * superblock->inode_num;
}
