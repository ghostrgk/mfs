add_executable(simple_server main.cpp cmds.cpp inits.cpp)

set_target_properties(simple_server PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
        C_STANDARD 11
        C_STANDARD_REQUIRED YES
        C_EXTENSIONS NO
        )

target_link_libraries(simple_server PRIVATE fs++)