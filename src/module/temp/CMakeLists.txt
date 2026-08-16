add_library(temperature MODULE tempfetch.c)

target_link_libraries(temperature PRIVATE
	waybro_api
)

install(TARGETS temperature
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${APP_NAME}
)
