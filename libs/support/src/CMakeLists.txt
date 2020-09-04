add_library(support STATIC files.cpp network.cpp)

target_include_directories(support PUBLIC ${PROJECT_SOURCE_DIR}/include)

set_target_properties(support PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
        C_STANDARD 11
        C_STANDARD_REQUIRED YES
        C_EXTENSIONS NO
        )