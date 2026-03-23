add_library(hyprworkspace MODULE workspacefetch.c)

target_link_libraries(hyprworkspace PRIVATE
	waybro_api
)

install(TARGETS hyprworkspace
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${APP_NAME}
)
