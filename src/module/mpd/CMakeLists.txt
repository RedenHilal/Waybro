add_library(mpd MODULE mpdfetch.c)

target_link_libraries(mpd PRIVATE
	waybro_api
)

install(TARGETS mpd
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${APP_NAME}
)

