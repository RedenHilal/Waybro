pkg_check_modules(SDBUS REQUIRED IMPORTED_TARGET libsystemd)

add_library(power MODULE fetcher.c)

target_link_libraries(power PRIVATE 
	waybro_api
	PkgConfig::SDBUS
)


install(TARGETS power
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${APP_NAME}
)


