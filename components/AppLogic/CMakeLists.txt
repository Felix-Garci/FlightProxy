file(GLOB_RECURSE SOURCES "src/*.cpp")

if(IS_ESP_PLATFORM)

	idf_component_register(SRCS ${SOURCES}
						   INCLUDE_DIRS "include"
						   REQUIRES Core 
									Channel
						   )

else()
	add_library(AppLogic ${SOURCES})
	target_include_directories(AppLogic PUBLIC "include")
	target_link_libraries(AppLogic PUBLIC Core
										  Channel
					      )

endif()
