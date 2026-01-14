cmake_minimum_required(VERSION 3.16)

# Definimos una opción que podemos activar manualmente desde terminal
option(BUILD_FOR_LINUX "Forzar build para PC/Linux" OFF)

# --- LÓGICA DE DECISIÓN MEJORADA ---
if(BUILD_FOR_LINUX)
    # Si yo te digo explícitamente que es Linux, es Linux. (Ignora el .env)
    set(IS_ESP_PLATFORM 0)
    message(STATUS "Build forzado a MODO LINUX")

elseif(DEFINED ENV{IDF_PATH})
    # Si no dije nada, pero veo IDF_PATH, asumo que es ESP32 (para idf.py)
    set(IS_ESP_PLATFORM 1)
    message(STATUS "Entorno ESP-IDF detectado -> MODO ESP32")

else()
    # Si no hay nada, por defecto Linux
    set(IS_ESP_PLATFORM 0)
    message(STATUS "No se detectó IDF -> MODO LINUX")
endif()




if(IS_ESP_PLATFORM)
    include($ENV{IDF_PATH}/tools/cmake/project.cmake)
    project(TransponderESP32)

else()
    project(TransponderLinux CXX)
    set(CMAKE_CXX_STANDARD 17)

    add_compile_definitions(LINUX_PLATFORM)
    
    # 2. Añadimos flags de depuración
    add_compile_options(-g -Wall)

	# Escanamos carpeta de componentes 
    # 1. Obtenemos lista de carpetas dentro de 'components'
    file(GLOB component_dirs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/components components/*)
    
    # 2. Iteramos y añadimos cada subdirectorio
    foreach(component_dir ${component_dirs})
        if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/components/${component_dir})
            # CMake entrará en la carpeta. 
            # Si el componente tiene un 'else()' para Linux, creará la librería.
            # Si el componente es solo para ESP32 (else vacío), no hará nada.
            add_subdirectory(components/${component_dir})
        endif()
    endforeach() 

	add_executable(TransponderLinux 
	main/main.cpp
	main/FlightApplication.cpp)

	target_link_libraries(TransponderLinux 
		PRIVATE
		AppLogic
		PlatformLinux
		pthread
	)

endif()

# Forzar salida a color siempre (ayuda visualmente a separar errores)
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    add_compile_options(-fdiagnostics-color=always)
endif()
