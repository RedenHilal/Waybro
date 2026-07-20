add_library(clock MODULE timefetch.c)

target_link_libraries(clock PRIVATE
	waybro_api
)

install(TARGETS clock
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${APP_NAME}
)
