add_executable(server main.cpp)

set_target_properties(server PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
        )

target_link_libraries(server PRIVATE fs++)