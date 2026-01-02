file(GLOB_RECURSE SOURCES "src/*.cpp")

file(GLOB_RECURSE HEADERS 
    "include/*.h" 
    "include/*.hpp"
)

if(NOT COMMAND idf_component_register )
	add_library(PlatformLinux  ${SOURCES} ${HEADERS})
	target_include_directories(PlatformLinux PUBLIC "include")
	target_link_libraries(PlatformLinux PUBLIC Core)
endif()
