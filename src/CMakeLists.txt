# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

idf_component_register(SRCS ${app_sources})

# Ensure C++20 for std::span and related features
target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_20)
