cmake_minimum_required(VERSION 3.20)
project(DecrementalConnectivity LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(generator src/gen.cpp)
add_executable(decremental_tree_connectivity src/main.cpp)
target_include_directories(decremental_tree_connectivity PRIVATE include)
