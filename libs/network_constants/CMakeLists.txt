cmake_minimum_required(VERSION 3.10)

project(network_constants
        VERSION 0.1
        LANGUAGES C CXX)

add_library(network_constants INTERFACE)
target_include_directories(network_constants INTERFACE ${PROJECT_SOURCE_DIR}/include)