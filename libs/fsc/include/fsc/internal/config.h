#pragma once

#include <stdint.h>

#define NORMAL_ILIST

typedef uint64_t id_t;

// should be multiple of 8
const uint64_t DEFAULT_BLOCK_COUNT = 64 * 1024 * 64;
const uint64_t DEFAULT_INODE_COUNT = 64 * 16;

// 4096 * 2
#define BLOCK_SIZE (4096 * 2)

#define MAX_LINK_NAME_LEN 62

#define ILIST_ZERO_INDIRECTION 10

const uint64_t IDS_IN_BLOCK_COUNT = BLOCK_SIZE / sizeof(uint64_t);

#ifdef NORMAL_ILIST
const uint64_t INODE_MAX_BLOCK_COUNT =
    ILIST_ZERO_INDIRECTION + IDS_IN_BLOCK_COUNT + IDS_IN_BLOCK_COUNT * IDS_IN_BLOCK_COUNT;
#else
const uint64_t INODE_MAX_BLOCK_COUNT = 512;
#endif
