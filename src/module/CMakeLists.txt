file(GLOB MODULE_DIRS LIST_DIRECTORIES true RELATIVE ${CMAKE_SOURCE_DIR} "*")
message(STATUS "${CMAKE_SOURCE_DIR}")

foreach(dir ${MODULE_DIRS})
		if (IS_DIRECTORY "${CMAKE_SOURCE_DIR}/${dir}" AND EXISTS "${CMAKE_SOURCE_DIR}/${dir}/CMakeLists.txt")
				message(STATUS "Including ${dir}")
				add_subdirectory("${CMAKE_SOURCE_DIR}/${dir}")
		endif()
endforeach()
