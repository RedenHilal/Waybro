add_library(memory MODULE memfetch.c)

target_link_libraries(memory PRIVATE
	waybro_api
)

install(TARGETS memory
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${APP_NAME}
)
