cmake_minimum_required(VERSION 3.10)

project(My3Dsim)

add_executable(My3Dsim
    buffer.c
    avlTree.c
    fcl.c
    flash.c
    ftl.c
    initialize.c
    interface.c
    ssd.c
)

# 添加头文件路径
target_include_directories(My3Dsim PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 添加调试信息
set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -O0 -g")

target_link_libraries(My3Dsim PRIVATE m)
