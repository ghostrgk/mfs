#include "fs++/internal/inode.h"

#include <algorithm>
#include <cstring>

namespace fspp::internal {

InodeSpace::InodeSpace(uint64_t* inode_num_ptr, uint64_t* free_inode_num_ptr, uint8_t* bit_set_bytes,
                       BlockSpace* blocks, Inode* inode_bytes)
    : inode_num_ptr_(inode_num_ptr),
      free_inode_num_ptr_(free_inode_num_ptr),
      bit_set_(*inode_num_ptr_, bit_set_bytes),
      blocks_(blocks),
      inodes_(inode_bytes) {
}

Inode& InodeSpace::getInodeById(uint64_t inode_id) {
  return inodes_[inode_id];
}

int InodeSpace::read(Inode* inode_ptr, void* buffer, uint64_t offset, uint64_t count) const {
  auto& inode = *inode_ptr;
  auto* byte_buffer = static_cast<uint8_t*>(buffer);
  uint64_t count_down = count;

  assert(offset + count <= inode.file_size);

  uint64_t buffer_offset = 0;
  uint64_t block_index = offset / BLOCK_SIZE;

  if (offset % BLOCK_SIZE != 0) {
    uint64_t block_offset = offset % BLOCK_SIZE;
    uint64_t start_size = std::min(count_down, BLOCK_SIZE - block_offset);

    auto& block = getBlockByIndex(inode, block_index);
    memcpy(buffer, block.bytes + block_offset, start_size);

    count_down -= start_size;
    offset += start_size;
    buffer_offset += start_size;

    ++block_index;
  }

  assert(offset % BLOCK_SIZE == 0);

  for (; block_index < inode.blocks_count && offset < inode.file_size && buffer_offset < count; ++block_index) {
    auto& block = getBlockByIndex(inode, block_index);
    const uint64_t read_size = std::min(count_down, BLOCK_SIZE);

    memcpy(byte_buffer + buffer_offset, block.bytes, read_size);

    count_down -= read_size;
    offset += read_size;
    buffer_offset += read_size;
  }

  return buffer_offset;
}

int InodeSpace::write(Inode* inode_ptr, const void* buffer, uint64_t offset, uint64_t count) {
  auto& inode = *inode_ptr;
  const auto* byte_buffer = static_cast<const uint8_t*>(buffer);
  uint64_t count_down = count;

  if (offset + count > inode.file_size) {
    if (extend(inode, offset + count) < 0) {
      return -1;
    }
  }

  assert(offset + count <= inode.file_size);

  uint64_t buffer_offset = 0;
  uint64_t block_index = offset / BLOCK_SIZE;

  if (offset % BLOCK_SIZE != 0) {
    uint64_t block_offset = offset % BLOCK_SIZE;
    uint64_t start_size = std::min(count_down, BLOCK_SIZE - block_offset);

    auto& block = getBlockByIndex(inode, block_index);
    memcpy(block.bytes + block_offset, buffer, start_size);

    count_down -= start_size;
    offset += start_size;
    buffer_offset += start_size;

    ++block_index;
  }

  assert(offset % BLOCK_SIZE == 0);

  for (; block_index < inode.blocks_count && offset < inode.file_size && buffer_offset < count; ++block_index) {
    auto& block = getBlockByIndex(inode, block_index);
    const uint64_t write_size = std::min(count_down, BLOCK_SIZE);

    memcpy(block.bytes, byte_buffer + buffer_offset, write_size);

    count_down -= write_size;
    offset += write_size;
    buffer_offset += write_size;
  }

  return buffer_offset;
}

int InodeSpace::append(Inode* inode_ptr, const void* buffer, uint64_t count) {
  return write(inode_ptr, buffer, inode_ptr->file_size, count);
}

uint64_t InodeSpace::createInode() {
  assert(*free_inode_num_ptr_ != 0);
  --(*free_inode_num_ptr_);

  uint64_t id = bit_set_.findCleanBit();
  bit_set_.setBit(id);

  return id;
}

void InodeSpace::deleteInode(uint64_t inode_id) {
  assert(bit_set_.getBit(inode_id));

  auto& inode = getInodeById(inode_id);
  for (uint64_t i = 0; i < inode.blocks_count; ++i) {
    blocks_->deleteBlock(inode.inodes_list.getBlockIdByIndex(i));
  }
  bit_set_.clearBit(inode_id);
}

int InodeSpace::addDirectoryEntry(Inode* inode_ptr, const char* name) {
  assert(strlen(name) <= MAX_LINK_NAME_LEN);
  uint64_t new_inode_id = createInode();
  Link link = {.inode_id = new_inode_id};
  strcpy(link.name, name);

  if (append(inode_ptr, &link, sizeof(Link)) < 0) {
    return -1;
  }

  return 0;
}

int InodeSpace::addBlockToInode(Inode& inode, uint64_t block_id) {
  if (inode.inodes_list.addBlock(block_id) < 0) {
    return -1;
  }

  ++inode.blocks_count;
  return 0;
}

#ifdef NORMAL_ILIST
uint64_t InodesList::getBlockIdByIndex(uint64_t index) {
  assert(index < 10);
  return block_ids_[index];
}
#endif
}  // namespace fspp::internal