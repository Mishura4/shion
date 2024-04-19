cmake_minimum_required(VERSION 3.16)

project(shion LANGUAGES CXX)

add_library(shion)

if (MSVC)
	target_compile_options(shion PRIVATE /W4 /utf-8 /permissive- /Zc:preprocessor)
elseif (UNIX)
	target_compile_options(shion PRIVATE -Wall -Wextra -pedantic -pedantic-errors)
endif ()

target_compile_features(shion PUBLIC cxx_std_23)

target_include_directories(shion PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include")

file(GLOB_RECURSE SHION_HEADERS "${CMAKE_CURRENT_LIST_DIR}/include/*.hpp")
file(GLOB_RECURSE SHION_SOURCES "${CMAKE_CURRENT_LIST_DIR}/src/*.hpp" "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp")

target_sources(shion PUBLIC ${SHION_HEADERS} PRIVATE ${SHION_SOURCES})
