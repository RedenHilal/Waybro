cmake_minimum_required(VERSION 3.16)
project(sdl_panel C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS_DEBUG "-g -O0 -fsanitize=address -fno-omit-frame-pointer")
set(CMAKE_BUILD_TYPE Debug)

include_directories(include)

# Add executable
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
)

target_link_libraries(mypanel 
    wayland-client
    cairo
    dl
    m
)