add_library(mpd MODULE mpdfetch.c)

target_compile_options(mpd PRIVATE
	$<$<CONFIG:Debug>:-O0 -g -fsanitize=undefined>
)

target_link_libraries(mpd PRIVATE
	waybro_api
)


