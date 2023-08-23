cmake_minimum_required(VERSION 3.20)
project(MxCompiler)

# set(CMAKE_BINARY_DIR "${CMAKE_SOURCE_DIR}/out")

set(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

include_directories(/usr/include/antlr4-runtime/)
include_directories(src/generated/)
include_directories(src/ast/)
include_directories(src/asm/)
include_directories(src/ir/)
include_directories(src/opt/)


file(GLOB_RECURSE sources src/*.cpp)
add_executable(mxc ${sources})

target_link_libraries(mxc PUBLIC antlr4-runtime)
