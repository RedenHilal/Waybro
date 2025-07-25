cmake_minimum_required(VERSION 3.16)
project(sdl_panel C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS_DEBUG "-g -O0 -fsanitize=address -fno-omit-frame-pointer")
set(CMAKE_BUILD_TYPE Debug)

execute_process(
    COMMAND bash -c "../const.sh"
    OUTPUT_VARIABLE BKL_FLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

separate_arguments(BKL_FLAGS)
message(STATUS "BKL_FLAGS = ${BKL_FLAGS}")

find_package(PkgConfig REQUIRED)
pkg_check_modules(PANGOCAIRO REQUIRED pangocairo)

include_directories(include)

add_executable(mypanel 
    src/displayer.c
    src/fetcher.c
    src/brightnessfetch.c
    src/volumefetch.c
    src/handler.c
    src/netfetch.c
    src/bluefetch.c
    src/workspacefetch.c
    src/timefetch.c
    src/mpdfetch.c
    src/xdg-shell-client.c
    src/wlr-layer-shell.c
    src/mainpoll.c
    src/waylandstuff.c
    src/readconfig.c
    src/style.c
    src/proc.c
)


add_compile_definitions(
    ${BKL_FLAGS}
)

target_include_directories(mypanel PRIVATE ${PANGOCAIRO_INCLUDE_DIRS})

target_link_libraries(mypanel PRIVATE
    wayland-client
    ${PANGOCAIRO_LIBRARIES}
    cairo
    dl
    m
)
