#include <fs++/internal/ilist.h>

namespace fspp::internal {

#ifdef NORMAL_ILIST
uint64_t InodesList::getBlockIdByIndex(Blocks* blocks, uint64_t index) {
  assert(index < size_);
  if (index < ILIST_ZERO_INDIRECTION) {
    return block_ids_[index];
  }

  index -= ILIST_ZERO_INDIRECTION;

  if (index < IDS_IN_BLOCK_COUNT) {
    return resolveIndirection(blocks, level1_id, index);
  }

  index -= IDS_IN_BLOCK_COUNT;

  assert(index < IDS_IN_BLOCK_COUNT * IDS_IN_BLOCK_COUNT);

  id_t resolved_level1_id = resolveIndirection(blocks, level2_id, index / IDS_IN_BLOCK_COUNT);
  index %= IDS_IN_BLOCK_COUNT;

  return resolveIndirection(blocks, resolved_level1_id, index);
}

int InodesList::addBlock(Blocks* blocks, uint64_t block_id) {
  if (size_ + 1 == INODE_MAX_BLOCK_COUNT) {
    return -1;
  }

  uint64_t index = size_;
  ++size_;

  if (index < ILIST_ZERO_INDIRECTION) {
    block_ids_[index] = block_id;
    return 0;
  }

  index -= ILIST_ZERO_INDIRECTION;

  if (index == 0) {
    level1_id = blocks->createBlock();
  }

  if (index < IDS_IN_BLOCK_COUNT) {
    resolveIndirection(blocks, level1_id, index) = block_id;
    return 0;
  }

  index -= IDS_IN_BLOCK_COUNT;

  assert(index < IDS_IN_BLOCK_COUNT * IDS_IN_BLOCK_COUNT);

  if (index == 0) {
    level2_id = blocks->createBlock();
  }

  id_t& resolved_level1_id = resolveIndirection(blocks, level2_id, index / IDS_IN_BLOCK_COUNT);
  if (index % IDS_IN_BLOCK_COUNT == 0) {
    resolved_level1_id = blocks->createBlock();
  }

  index %= IDS_IN_BLOCK_COUNT;

  resolveIndirection(blocks, resolved_level1_id, index) = block_id;
  return 0;
}
#endif

}  // namespace fspp::internal