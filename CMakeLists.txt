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
include_directories(src/opt/mem2reg)
include_directories(src/opt/mx)
include_directories(src/opt/dce)
include_directories(src/opt/ssa)
include_directories(src/opt/cfg)
include_directories(src/opt/sccp)
include_directories(src/opt/info)
include_directories(src/opt/simu)
include_directories(src/opt/loop)
include_directories(src/opt/local)
include_directories(src/opt/inline)



file(GLOB_RECURSE sources src/*.cpp)
add_executable(mxc ${sources})

target_link_libraries(mxc PUBLIC antlr4-runtime)
