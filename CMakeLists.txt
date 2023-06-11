project(false-nine)

cmake_minimum_required(VERSION 3.1)

# support C++14 features used by LLVM 10.0.0
set(CMAKE_CXX_STANDARD 14)

find_package(LLVM 10 REQUIRED CONFIG)
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

message(${LLVM_INCLUDE_DIRS})
include_directories(./include)

add_library(falsenine MODULE
    ./src/Common.cc
    ./src/AccessPath.cc
    ./src/DeadMemoryFreePass.cc
    ./src/AccessPathDFA.cc
)

add_library(memorygraph MODULE
    ./src/Common.cc
    ./src/MemoryGraph.cc
    ./src/MemoryGraphPass.cc
)

set_target_properties(falsenine PROPERTIES
    COMPILE_FLAGS "-fno-rtti -g -fPIC"
)
