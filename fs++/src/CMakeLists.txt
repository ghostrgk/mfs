add_library(fs++ STATIC filesystem_client.cpp block.cpp inode.cpp bitset.cpp filesystem.cpp)

target_include_directories(fs++ PUBLIC ${PROJECT_SOURCE_DIR}/include)

set_target_properties(fs++ PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
        )