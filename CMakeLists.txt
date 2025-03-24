cmake_minimum_required(VERSION 3.16)

if (CMAKE_VERSION VERSION_GREATER_EQUAL 4.0.0)
	set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "a9e1cf81-9932-4810-974b-6eccaf14e457")
elseif (CMAKE_VERSION VERSION_GREATER_EQUAL 3.30)
	set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "0e5b6991-d74f-4b3d-a41c-cf096e0b2508")
endif ()

project(shion LANGUAGES CXX)

option(SHION_BUILD_TESTS "Whether to build the tests" on)
option(SHION_BUILD_MODULES "Whether to build the library as a module" on)
option(SHION_USE_MODULES "Whether to use modules when building the library (forced if SHION_BUILD_MODULES=on)" on)
set(SHION_STD_MODULE_LOCATION "" CACHE STRING "Specify a custom location for the std module, or build it if empty")

set(CMAKE_CXX_STANDARD 23)

if (SHION_BUILD_MODULES)
	set(SHION_BUILD_MODULES on)
	set(CMAKE_CXX_SCAN_FOR_MODULES on)
endif()

if (LINUX)
	option(SHION_USE_CMAKE_IMPORT_STD "Whether to use CMake's import std feature" ${SHION_BUILD_MODULES})
else ()
	option(SHION_USE_CMAKE_IMPORT_STD "Whether to use CMake's import std feature" off)
endif ()

if (SHION_USE_CMAKE_IMPORT_STD)
	cmake_minimum_required(VERSION 3.30 FATAL_ERROR)

	set(CMAKE_CXX_MODULE_STD 1)
endif ()

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
set(SHION_PRIVATE_BUILD_OPTIONS "")
set(SHION_PUBLIC_BUILD_OPTIONS "")
set(SHION_PRIVATE_BUILD_DEFINITIONS "")
set(SHION_PUBLIC_BUILD_DEFINITIONS "")

if (MSVC)
	set(SHION_PRIVATE_BUILD_OPTIONS ${SHION_PRIVATE_BUILD_OPTIONS}
		/W4 /utf-8 /permissive-
	)
	set(SHION_PRIVATE_BUILD_DEFINITIONS ${SHION_PRIVATE_BUILD_DEFINITIONS}
		NOMINMAX
	)
	set(SHION_PUBLIC_BUILD_OPTIONS ${SHION_PUBLIC_BUILD_OPTIONS}
		/wd4251 # 'meow' must have DLL_EXPORT attribute to be used by 'bork'
		/permissive-
	)
	if (NOT CLANG_CL)
		set(SHION_PUBLIC_BUILD_OPTIONS ${SHION_PUBLIC_BUILD_OPTIONS} /Zc:preprocessor)
	endif()
elseif (UNIX)
	set(SHION_PRIVATE_BUILD_OPTIONS ${SHION_PRIVATE_BUILD_OPTIONS} -Wall -Wextra -pedantic -pedantic-errors)
endif ()

if (UNIX OR CLANG_CL)
	set(SHION_PRIVATE_BUILD_OPTIONS ${SHION_PRIVATE_BUILD_OPTIONS} "-Wno-unknown-attributes" "-Wno-c++98-compat")
endif ()

target_compile_options(shion PRIVATE ${SHION_PRIVATE_BUILD_OPTIONS})
target_compile_options(shion PUBLIC ${SHION_PUBLIC_BUILD_OPTIONS})
target_compile_definitions(shion PRIVATE ${SHION_PRIVATE_BUILD_DEFINITIONS})
target_compile_definitions(shion PUBLIC ${SHION_PUBLIC_BUILD_DEFINITIONS})

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
					"$ENV{VCToolsInstallDir}/modules/std.ixx"
					DESTINATION
					${PROJECT_BINARY_DIR}/stdxx
				)

				add_library(shion_std_module)
				target_sources(shion_std_module PUBLIC FILE_SET CXX_MODULES FILES "${PROJECT_BINARY_DIR}/stdxx/std.ixx")
				target_compile_features(shion_std_module PUBLIC cxx_std_23)
				target_compile_options(shion_std_module PRIVATE ${SHION_PRIVATE_BUILD_OPTIONS})
				target_compile_options(shion_std_module PUBLIC ${SHION_PUBLIC_BUILD_OPTIONS})
				target_compile_definitions(shion_std_module PRIVATE ${SHION_PRIVATE_BUILD_DEFINITIONS})
				target_compile_definitions(shion_std_module PUBLIC ${SHION_PPUBLIC_BUILD_DEFINITIONS})

				set(SHION_STD_MODULE_LOCATION "$<TARGET_FILE:shion_std_module>")
			endif()
		else ()
			if (WIN32)
				set_target_properties(shion PROPERTIES COMPILE_FLAGS "/reference \"${SHION_STD_MODULE_LOCATION}\"")
			endif ()
		endif ()
	endif()
endif ()


if (SHION_BUILD_MODULES)
	file(GLOB_RECURSE SHION_MODULES "${CMAKE_CURRENT_LIST_DIR}/src/modules/*.ixx")

	target_compile_definitions(shion PUBLIC SHION_MODULES)
	target_sources(shion
		PUBLIC
			FILE_SET CXX_MODULES FILES "${SHION_MODULES}"
	)
	if (MSVC)
		if (NOT CLANG_CL)
			target_compile_options(shion PUBLIC /wd5244) # #include <meow> in the purview of module shion.meow appears erroneous
		endif ()
	endif ()
else ()
	set(SHION_MODULES "")
endif ()

if (SHION_LINK_STD_MODULE)
	target_link_libraries(shion PUBLIC shion_std_module)
endif ()

file(GLOB_RECURSE SHION_HEADERS "${CMAKE_CURRENT_LIST_DIR}/include/*.hpp")
file(GLOB_RECURSE SHION_SOURCES "${CMAKE_CURRENT_LIST_DIR}/src/shion/*.hpp" "${CMAKE_CURRENT_LIST_DIR}/src/shion/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/include/shion/*.cpp")
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
