add_library(backlight MODULE brightnessfetch.c)

target_link_libraries(backlight PRIVATE
	waybro_api
)



