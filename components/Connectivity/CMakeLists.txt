file(GLOB_RECURSE SOURCES "src/*.cpp")

if(COMMAND idf_component_register)

	idf_component_register(SRCS ${SOURCES}
						   INCLUDE_DIRS "include"
						   REQUIRES esp_wifi
									nvs_flash
									esp_event
									Core 
						   )

endif()

