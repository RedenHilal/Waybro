add_library(clock MODULE timefetch.c)

target_link_libraries(clock PRIVATE
	waybro_api
)


