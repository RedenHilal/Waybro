pkg_check_modules(PULSEAUDIO REQUIRED IMPORTED_TARGET libpulse)

add_library(pulseaudio MODULE volumefetch.c)

target_link_libraries(pulseaudio PRIVATE 
	waybro_api
	PkgConfig::PULSEAUDIO
)


install(TARGETS pulseaudio
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${APP_NAME}
)


