pkg_check_modules(SDBUSCPP REQUIRED IMPORTED_TARGET sdbus-c++)

add_library(bluetooth MODULE bluezfetch.cpp BlueZState.cpp)

target_compile_features(bluetooth PRIVATE cxx_std_20)

target_link_libraries(bluetooth PRIVATE 
	waybro_api
	PkgConfig::SDBUSCPP
)

install(TARGETS bluetooth
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${APP_NAME}
)
