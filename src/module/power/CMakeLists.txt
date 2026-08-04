pkg_check_modules(SDBUSCPP REQUIRED IMPORTED_TARGET sdbus-c++)

add_library(power MODULE PowerFetch.cpp PowerStatus.cpp)

target_compile_features(power PRIVATE cxx_std_20)

target_link_libraries(power PRIVATE 
	waybro_api
	PkgConfig::SDBUSCPP
)


install(TARGETS power
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${APP_NAME}
)


