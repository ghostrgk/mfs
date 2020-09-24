add_library(fsc STATIC
        bitset.c
        block.c
        filesystem.c
        filesystem_client.c
        ilist.c
        inode.c superblock.c string.c string.c)

target_include_directories(fsc PUBLIC ${PROJECT_SOURCE_DIR}/include)

set_target_properties(fsc PROPERTIES
        C_STANDARD 11
        C_STANDARD_REQUIRED YES
        )