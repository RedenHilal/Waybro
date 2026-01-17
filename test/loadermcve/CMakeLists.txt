cmake_minimum_required(VERSION 3.10)
project(dlopen_mcve C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)


add_executable(loader main.c)

target_compile_options(loader PRIVATE
)

target_link_options(loader PRIVATE
)

target_link_libraries(loader PRIVATE dl)

set_target_properties(loader PROPERTIES BUILD_RPATH "\$ORIGIN/module")

add_subdirectory(module/)


