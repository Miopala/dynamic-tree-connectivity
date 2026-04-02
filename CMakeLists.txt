cmake_minimum_required(VERSION 3.20)
project(DecrementalConnectivity LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT MSVC)
    add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -O3 -march=native -flto)
endif()

include_directories(include)

add_executable(decremental_tree_connectivity src/main.cpp)