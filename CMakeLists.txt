cmake_minimum_required(VERSION 3.16)
project(Waybro C)

set(CMAKE_C_STANDARD 99)
set(APP_NAME "waybro")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/src/module")
set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)

execute_process(
    COMMAND bash -c "../const.sh"
    OUTPUT_VARIABLE BKL_FLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)


message(STATUS "BKL_FLAGS = ${BKL_FLAGS}")

find_package(PkgConfig REQUIRED)
pkg_check_modules(PANGO REQUIRED IMPORTED_TARGET pango)
pkg_check_modules(PANGOCAIRO REQUIRED IMPORTED_TARGET pangocairo)
pkg_check_modules(LIBCONFIG REQUIRED IMPORTED_TARGET libconfig)

add_executable(waybro 
	src/bar.c
	src/config.c
    src/displayer.c
	src/layout.c
	src/loader.c
    src/mainpoll.c
	src/module.c
	src/poll.c
	src/render.c
    src/style.c
    src/xdg-shell-client.c
    src/wlr-layer-shell.c
    src/waylandstuff.c
)


target_compile_options(waybro PRIVATE
	$<$<CONFIG:Debug>:-O0 -g -fsanitize=undefined,address>
)

target_link_options(waybro PRIVATE
	$<$<CONFIG:Debug>:-O0 -g -fsanitize=undefined,address>
)

separate_arguments(BKL_FLAGS)

add_compile_definitions(
    ${BKL_FLAGS}
)

target_include_directories(waybro PRIVATE include)
target_link_libraries(waybro PRIVATE
    wayland-client
	PkgConfig::PANGO
	PkgConfig::PANGOCAIRO
	PkgConfig::LIBCONFIG
	cairo
    dl
    m
)

add_library(waybro_api INTERFACE)

target_include_directories(waybro_api INTERFACE include)

target_compile_options(waybro_api INTERFACE
	$<$<CONFIG:Debug>:-O0 -g -fsanitize=undefined,address -static-libasan>
)

target_link_options(waybro_api INTERFACE
	$<$<CONFIG:Debug>:-O0 -g -fsanitize=undefined,address -static-libasan>
)

add_subdirectory(src/module/)

install(TARGETS waybro DESTINATION bin)

set_target_properties(waybro PROPERTIES BUILD_RPATH "\$ORIGIN/src/module")
set_target_properties(waybro PROPERTIES INSTALL_RPATH "$ORIGIN/module")
