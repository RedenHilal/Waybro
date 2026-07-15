pkg_check_modules(NETLINK REQUIRED IMPORTED_TARGET libnl-3.0)
pkg_check_modules(GENL REQUIRED IMPORTED_TARGET libnl-genl-3.0)

add_library(network MODULE network.c)

target_link_libraries(network PRIVATE 
	waybro_api
	PkgConfig::NETLINK
	PkgConfig::GENL
)


install(TARGETS network 
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${APP_NAME}
)
