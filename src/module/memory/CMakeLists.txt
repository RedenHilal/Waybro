add_library(memory MODULE memfetch.c)

target_link_libraries(memory PRIVATE
	waybro_api
)



