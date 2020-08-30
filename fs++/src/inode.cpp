#include <fs++/internal/inode.h>

#include <cstring>

#include <fs++/internal/logging.h>

namespace fspp::internal {

[[maybe_unused]] Inodes::Inodes(const uint64_t* inode_num_ptr, uint64_t* free_inode_num_ptr, uint8_t* bit_set_bytes,
                                Blocks* blocks, Inode* inode_bytes)
    : /*inode_num_ptr_(inode_num_ptr),*/
      inodes_ptr_start_(inode_bytes),
      blocks_(blocks),
      free_inode_num_ptr_(free_inode_num_ptr),
      bit_set_(*inode_num_ptr, bit_set_bytes) {
}

Inode& Inodes::getInodeById(uint64_t inode_id) {
  return inodes_ptr_start_[inode_id];
}

int Inodes::read(Inode* inode_ptr, void* buffer, uint64_t offset, uint64_t count) const {
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

  if (buffer_offset == count) {
    return buffer_offset;
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

int Inodes::write(Inode* inode_ptr, const void* buffer, uint64_t offset, uint64_t count) {
  auto& inode = *inode_ptr;
  const auto* byte_buffer = static_cast<const uint8_t*>(buffer);
  uint64_t count_down = count;

  if (offset + count > inode.file_size) {
    if (extend(inode, offset + count) < 0) {
      FSPP_LOG("INODE", "can't extend inodes");
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

  if (buffer_offset == count) {
    return buffer_offset;
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

int Inodes::append(Inode* inode_ptr, const void* buffer, uint64_t count) {
  return write(inode_ptr, buffer, inode_ptr->file_size, count);
}

uint64_t Inodes::createInode() {
  assert(*free_inode_num_ptr_ != 0);
  --(*free_inode_num_ptr_);

  uint64_t id = bit_set_.findCleanBit();
  bit_set_.setBit(id);

  clearInode(&getInodeById(id));

  return id;
}

void Inodes::deleteInode(uint64_t inode_id) {
  assert(bit_set_.getBit(inode_id));

  if (Inode& inode = getInodeById(inode_id); inode.is_dir) {
    // delete all files and subdirectories
    for (uint64_t i = 0; i * sizeof(Link) < inode.file_size; ++i) {
      Link link;
      read(&inode, &link, i * sizeof(Link), sizeof(Link));
      if (link.is_alive) {
        deleteInode(link.inode_id);
      }
    }
  }

  // todo: delete InodeList blocks
  auto& inode = getInodeById(inode_id);
  for (uint64_t i = 0; i < inode.blocks_count; ++i) {
    if (blocks_->deleteBlock(inode.inodes_list.getBlockIdByIndex(blocks_, i)) < 0) {
      std::abort();
    }
  }
  bit_set_.clearBit(inode_id);

  ++(*free_inode_num_ptr_);
}

int Inodes::addDirectoryEntry(Inode* inode_ptr, const char* name, bool is_dir) {
  assert(strlen(name) <= MAX_LINK_NAME_LEN);

  uint64_t new_inode_id = createInode();
  getInodeById(new_inode_id).is_dir = is_dir;

  Link new_link = {.is_alive = true, .inode_id = new_inode_id};
  strcpy(new_link.name, name);

  for (uint64_t i = 0; i * sizeof(Link) < inode_ptr->file_size; ++i) {
    Link link;
    read(inode_ptr, &link, i * sizeof(Link), sizeof(Link));
    if (!link.is_alive) {
      if (write(inode_ptr, &new_link, i * sizeof(Link), sizeof(Link)) < 0) {
        return -1;
      }

      return 0;
    }
  }

  if (append(inode_ptr, &new_link, sizeof(Link)) < 0) {
    return -1;
  }

  return 0;
}

int Inodes::addBlockToInode(Inode& inode, uint64_t block_id) {
  if (inode.inodes_list.addBlock(blocks_, block_id) < 0) {
    return -1;
  }

  ++inode.blocks_count;
  return 0;
}

[[maybe_unused]] int Inodes::gcLaterRename(uint64_t) {
  return 0;
}

int Inodes::clearInode(Inode* inode_ptr) {
  inode_ptr->inodes_list.clear();
  inode_ptr->file_size = 0;
  inode_ptr->blocks_count = 0;

  return 0;
}

Block& Inodes::getBlockByIndex(Inode& inode, uint64_t index) const {
  return blocks_->getBlockById(inode.inodes_list.getBlockIdByIndex(blocks_, index));
}

int Inodes::extend(Inode& inode, uint64_t new_size) {
  uint64_t exact_block_count = (new_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
  if (exact_block_count - inode.blocks_count > blocks_->getFreeBlockNum() ||
      exact_block_count > InodesList::max_size()) {
    return -1;
  }

  while (inode.blocks_count != exact_block_count) {
    id_t new_block_id;

    if (blocks_->createBlock(&new_block_id) < 0 || addBlockToInode(inode, new_block_id) < 0) {
      return -1;
    }
  }

  inode.file_size = new_size;

  return 0;
}
Inodes::Inodes(void* inodes_ptr_start, Blocks* blocks, uint64_t* free_inode_num_ptr, BitSet inodes_bitset)
    : inodes_ptr_start_(static_cast<Inode*>(inodes_ptr_start)),
      blocks_(blocks),
      free_inode_num_ptr_(free_inode_num_ptr),
      bit_set_(std::move(inodes_bitset)) {
}

}  // namespace fspp::internal