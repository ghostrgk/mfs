add_library(fs++ STATIC
        bitset.cpp
        block.cpp
        filesystem.cpp
        filesystem_client.cpp
        ilist.cpp
        inode.cpp)

target_include_directories(fs++ PUBLIC ${PROJECT_SOURCE_DIR}/include)

set_target_properties(fs++ PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
        )