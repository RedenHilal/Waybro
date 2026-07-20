add_library(backlight MODULE brightnessfetch.c)

target_link_libraries(backlight PRIVATE
	waybro_api
)

install(TARGETS backlight
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${APP_NAME}
)
