#pragma once

#include <cstdint>

#define NORMAL_ILIST

namespace fspp {

typedef uint64_t id_t;

// should be multiple of 8
const uint64_t DEFAULT_BLOCK_COUNT = 64 * 1024 * 64;
const uint64_t DEFAULT_INODE_COUNT = 64 * 16;
const uint64_t BLOCK_SIZE = 4096 * 2;
const uint64_t MAX_LINK_NAME_LEN = 62;
const uint64_t ILIST_ZERO_INDIRECTION = 10;
const uint64_t IDS_IN_BLOCK_COUNT = BLOCK_SIZE / sizeof(uint64_t);

#ifdef NORMAL_ILIST
const uint64_t INODE_MAX_BLOCK_COUNT =
    ILIST_ZERO_INDIRECTION + IDS_IN_BLOCK_COUNT + IDS_IN_BLOCK_COUNT * IDS_IN_BLOCK_COUNT;
#else
const uint64_t INODE_MAX_BLOCK_COUNT = 512;
#endif

static_assert(sizeof(char) == sizeof(uint8_t), "char should be 1 byte");
static_assert(DEFAULT_BLOCK_COUNT % 8 == 0);   // current requirement of bitset
static_assert(DEFAULT_INODE_COUNT % 8 == 0);   // current requirement of bitset
static_assert(DEFAULT_BLOCK_COUNT % 64 == 0);  // requirement of layout
static_assert(DEFAULT_INODE_COUNT % 64 == 0);  // requirement of layout
static_assert(BLOCK_SIZE % sizeof(id_t) == 0);

}  // namespace fspp