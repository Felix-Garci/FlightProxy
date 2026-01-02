file(GLOB_RECURSE SOURCES "src/*.cpp")

if(COMMAND idf_component_register)
	idf_component_register(SRCS ${SOURCES}
						   INCLUDE_DIRS "include"
						   REQUIRES Core 
						   )
else()
    if(SOURCES)
        add_library(Channel ${SOURCES})
        target_include_directories(Channel PUBLIC "include")
        target_link_libraries(Channel PUBLIC Core)
    else()
        # SI NO HAY .CPP (Templates):
        add_library(Channel INTERFACE)
        target_include_directories(Channel INTERFACE "include")
        target_link_libraries(Channel INTERFACE Core)
    endif()
endif()
