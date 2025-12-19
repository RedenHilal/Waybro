cmake_minimum_required(VERSION 3.16)
project(sdl_panel C)

set(CMAKE_C_STANDARD 99)


execute_process(
    COMMAND bash -c "../const.sh"
    OUTPUT_VARIABLE BKL_FLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)


message(STATUS "BKL_FLAGS = ${BKL_FLAGS}")

find_package(PkgConfig REQUIRED)

pkg_check_modules(PANGOCAIRO REQUIRED pangocairo)
pkg_check_modules(SYSTEMD REQUIRED libsystemd)
pkg_check_modules(PULSE REQUIRED libpulse)
pkg_check_modules(NETLINK REQUIRED libnl-3.0)
pkg_check_modules(GENL REQUIRED libnl-genl-3.0)

add_executable(waybro 
    src/displayer.c
    src/fetcher.c
    src/brightnessfetch.c
    src/volumefetch.c
    src/handler.c
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
    src/memfetch.c
    src/tempfetch.c
	src/sd_bus_wrapper.c
	src/module/network.c

	test/nlscheme/nlscheme.c
)


target_compile_options(waybro PRIVATE
    $<$<CONFIG:Debug>:-O0 -g -fno-omit-frame-pointer -fsanitize=address,undefined>
)

target_link_options(waybro PRIVATE
    $<$<CONFIG:Debug>:-fsanitize=address,undefined>
)

separate_arguments(BKL_FLAGS)

add_compile_definitions(
    ${BKL_FLAGS}
)

target_include_directories(waybro PRIVATE include)

target_include_directories(waybro PRIVATE 
				${GENL_INCLUDE_DIRS}
				${NETLINK_INCLUDE_DIRS}
				${PANGOCAIRO_INCLUDE_DIRS}
				${SYSTEMD_INCLUDE_DIRS}
				${PULSE_INCLUDE_DIRS}
)

target_link_libraries(waybro PRIVATE
    wayland-client
	${GENL_LIBRARIES}
	${NETLINK_LIBRARIES}
    ${PANGOCAIRO_LIBRARIES}
    ${SYSTEMD_LIBRARIES}
	${PULSE_LIBRARIES}
	cairo
    dl
    m
)

install(TARGETS waybro DESTINATION bin)
