cmake_minimum_required(VERSION 3.16)

project(shion LANGUAGES CXX)

if (MSVC AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	set(CLANG_CL true)
endif()

set(SHION_STATIC_LIBRARY ON CACHE BOOL "Whether to build libshion as a static library")

if (SHION_STATIC_LIBRARY)
	add_library(shion STATIC)
else ()
	add_library(shion SHARED)
	target_compile_definitions(shion PUBLIC SHION_SHARED_LIBRARY)
endif ()

target_compile_definitions(shion PRIVATE SHION_BUILD)

if (MSVC)
	target_compile_options(shion PRIVATE /wd4100 /W4 /utf-8 /permissive- /wd4189)
	target_compile_options(shion PUBLIC /wd4251)
	target_compile_definitions(shion PRIVATE NOMINMAX)
	if (NOT CLANG_CL)
		target_compile_options(shion PUBLIC /Zc:preprocessor)
	endif()
elseif (UNIX)
	target_compile_options(shion PRIVATE -Wall -Wextra -pedantic -pedantic-errors)
endif ()

if (UNIX OR CLANG_CL)
	target_compile_options(shion PRIVATE "-Wno-unknown-attributes" "-Wno-c++98-compat")
endif ()

target_compile_features(shion PUBLIC cxx_std_23)

target_include_directories(shion PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include")

file(GLOB_RECURSE SHION_HEADERS "${CMAKE_CURRENT_LIST_DIR}/include/*.hpp")
file(GLOB_RECURSE SHION_SOURCES "${CMAKE_CURRENT_LIST_DIR}/src/*.hpp" "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp")

target_sources(shion PUBLIC ${SHION_HEADERS} PRIVATE ${SHION_SOURCES})
