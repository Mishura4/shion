cmake_minimum_required(VERSION 3.16)

project(shion LANGUAGES CXX)

option(SHION_BUILD_TESTS "Whether to build the tests" on)
option(SHION_BUILD_MODULES "Whether to build the library as a module" on)
option(SHION_USE_MODULES "Whether to use modules when building the library (forced if SHION_BUILD_MODULES=on)" on)
set(SHION_STD_MODULE_LOCATION "" CACHE STRING "Specify a custom location for the std module, or build it if empty")
option(SHION_USE_CMAKE_IMPORT_STD "Whether to use CMake's import std feature" off)

if (SHION_BUILD_MODULES)
	set(SHION_BUILD_MODULES on)
	set(CMAKE_CXX_SCAN_FOR_MODULES on)
endif()

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
	target_compile_options(shion PRIVATE /W4 /utf-8 /permissive-)
	target_compile_options(shion PRIVATE /wd5244) # '#include 'x' in the purview of module 'y' appears erroneous
	target_compile_options(shion PUBLIC /wd4251) # 'x' must have DLL_EXPORT attribute to be used by 'y'
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

if (SHION_USE_MODULES)
	if (NOT SHION_USE_CMAKE_IMPORT_STD)
		if (CLANG_CL AND NOT SHION_FORCE_USE_MODULES)
			message(FATAL_ERROR "Modules with clang-cl are not supported")
		endif ()
		if (SHION_STD_MODULE_LOCATION STREQUAL "")
			if (WIN32)
				set(SHION_LINK_STD_MODULE on)
				set(CMAKE_EXPERIMENTAL_CXX_MODULE_DYNDEP 1)
				file(COPY
				# you don't want to hardcode the path here in a real-world project,
				# but you get the idea
				"$ENV{VCToolsInstallDir}/modules/std.ixx"
				DESTINATION
				${PROJECT_BINARY_DIR}/stdxx
				)

				add_library(shion_std_module)
				target_sources(shion_std_module PUBLIC FILE_SET CXX_MODULES FILES "${PROJECT_BINARY_DIR}/stdxx/std.ixx")
				target_compile_features(shion_std_module PUBLIC cxx_std_23)
				target_compile_options(shion_std_module PUBLIC /utf-8)

				set(SHION_STD_MODULE_LOCATION "$<TARGET_FILE:shion_std_module>")
			endif()
		else ()
			if (WIN32)
				set_target_properties(shion PROPERTIES COMPILE_FLAGS "/reference \"${SHION_STD_MODULE_LOCATION}\"")
			endif ()
		endif ()
	else()
		set(CMAKE_CXX_MODULE_STD 1)
	endif()
endif ()


if (SHION_BUILD_MODULES)
	add_library(shion_modules)
	file(GLOB_RECURSE SHION_MODULES "${CMAKE_CURRENT_LIST_DIR}/src/modules/*.ixx")

	target_include_directories(shion_modules PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include")
	target_compile_features(shion_modules PUBLIC cxx_std_23)
	target_compile_definitions(shion_modules PUBLIC SHION_USE_MODULES PRIVATE SHION_BUILDING_MODULES SHION_BUILDING_LIBRARY)
	target_sources(shion_modules
		PUBLIC
			FILE_SET CXX_MODULES FILES "${SHION_MODULES}"
	)
	target_link_libraries(shion PUBLIC shion_modules)
	if (SHION_LINK_STD_MODULE)
		target_link_libraries(shion_modules PUBLIC shion_std_module)
	endif ()
	if (MSVC)
		if (NOT CLANG_CL)
			target_compile_options(shion_modules PUBLIC /wd5244) # #include <meow> in the purview of module shion.meow appears erroneous
		endif ()
		target_compile_options(shion_modules PUBLIC /utf-8)
	endif ()
	target_compile_definitions(shion PRIVATE SHION_USE_INTERNAL_MODULES)
else ()
	set(SHION_MODULES "")
endif ()

if (SHION_LINK_STD_MODULE)
	target_link_libraries(shion PUBLIC shion_std_module)
endif ()

file(GLOB_RECURSE SHION_HEADERS "${CMAKE_CURRENT_LIST_DIR}/include/*.hpp")
file(GLOB_RECURSE SHION_SOURCES "${CMAKE_CURRENT_LIST_DIR}/src/shion/*.hpp" "${CMAKE_CURRENT_LIST_DIR}/src/shion/*.cpp")
file(GLOB_RECURSE SHION_TESTS_SOURCES "${CMAKE_CURRENT_LIST_DIR}/src/tests/*.hpp" "${CMAKE_CURRENT_LIST_DIR}/src/tests/*.cpp")

if (SHION_BUILD_TESTS)
	add_executable(tests)
	target_compile_features(tests PUBLIC cxx_std_23)
	target_link_libraries(tests PUBLIC shion)
	target_sources(tests PRIVATE ${SHION_TESTS_SOURCES})
endif ()

target_sources(shion
	PUBLIC ${SHION_HEADERS}
	PRIVATE
		${SHION_SOURCES}
)
