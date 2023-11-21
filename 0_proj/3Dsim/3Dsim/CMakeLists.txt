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

# Link the necessary libraries (math library)
target_link_libraries(My3Dsim PRIVATE m)

