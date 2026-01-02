file(GLOB_RECURSE SOURCES "src/*.cpp")

if(IS_ESP_PLATFORM)

	idf_component_register(SRCS ${SOURCES}
						   INCLUDE_DIRS "include"
						   )
else()
	add_library(Core INTERFACE)
	target_include_directories(Core INTERFACE "include")
	


endif()



