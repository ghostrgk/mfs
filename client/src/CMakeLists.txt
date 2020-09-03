add_executable(client main.cpp)

set_target_properties(client PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
        )

target_link_libraries(client PRIVATE support)