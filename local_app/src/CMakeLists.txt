add_executable(local_app main.cpp)

set_target_properties(local_app PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
        )

target_link_libraries(local_app PRIVATE fs++)