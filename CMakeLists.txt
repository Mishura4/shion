cmake_minimum_required(VERSION 3.16)

if (CMAKE_VERSION VERSION_GREATER_EQUAL 4.0.0)
	set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "a9e1cf81-9932-4810-974b-6eccaf14e457")
elseif (CMAKE_VERSION VERSION_GREATER_EQUAL 3.30)
	set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "0e5b6991-d74f-4b3d-a41c-cf096e0b2508")
endif ()

project(shion LANGUAGES CXX)

option(SHION_BUILD_MODULES "Whether to build the library as a module" on)
option(SHION_IMPORT_STD "Whether to use `import std;` when building the library modules" off)
option(SHION_BUILD_TESTS "Whether to build the tests" ${SHION_BUILD_MODULES})
set(SHION_STD_MODULE_LOCATION "" CACHE STRING "Specify a custom location for the std module, or build it if empty")

set(CMAKE_CXX_STANDARD 23)

if (LINUX)
	option(SHION_USE_CMAKE_IMPORT_STD "Whether to use CMake's import std feature" ${SHION_BUILD_MODULES})
	option(SHION_EXTERN_MODULES "Whether to wrap the module symbols in `extern \"C++\"` for traditional linkage" on)
else ()
	option(SHION_USE_CMAKE_IMPORT_STD "Whether to use CMake's import std feature" off)
	option(SHION_EXTERN_MODULES "Whether to wrap the module symbols in `extern \"C++\"` for traditional linkage" on)
endif ()

if (SHION_USE_CMAKE_IMPORT_STD)
	cmake_minimum_required(VERSION 3.30 FATAL_ERROR)

	set(CMAKE_CXX_MODULE_STD 1)
endif ()

if (MSVC AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	set(CLANG_CL true)
endif()

set(SHION_STATIC_LIBRARY (NOT ${BUILD_SHARED_LIBS}) CACHE BOOL "Whether to build shion as a static library")
set(SHION_MODULE_LIBRARY ${SHION_BUILD_MODULES} CACHE BOOL "Whether to build shion as a module library")
set(SHION_SHARED_LIBRARY ${BUILD_SHARED_LIBS} CACHE BOOL "Whether to build shion as a shared library")

set(SHION_SHARED_LIBRARY on)
set(SHION_STATIC_LIBRARY off)
set(SHION_MODULE_LIBRARY off)

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

if (SHION_BUILD_MODULES)
	set(SHION_BUILD_MODULES on)
	set(CMAKE_CXX_SCAN_FOR_MODULES on)
	if (SHION_EXTERN_MODULES)
		set(SHION_PUBLIC_BUILD_DEFINITIONS ${SHION_PUBLIC_BUILD_DEFINITIONS} SHION_EXTERN_MODULES)
	endif ()
endif()

if (UNIX OR CLANG_CL)
	set(SHION_PRIVATE_BUILD_OPTIONS ${SHION_PRIVATE_BUILD_OPTIONS} "-Wno-unknown-attributes" "-Wno-c++98-compat")
endif ()

set(SHION_HEADERS_DIR "${CMAKE_CURRENT_LIST_DIR}/include")
set(SHION_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/src")

file(GLOB_RECURSE SHION_HEADERS "${SHION_HEADERS_DIR}/shion/*.hpp")
file(GLOB_RECURSE SHION_SOURCES "${SHION_SOURCE_DIR}/shion/*.hpp" "${SHION_SOURCE_DIR}/shion/*.cpp" "${SHION_HEADERS_DIR}/shion/*.cpp")
file(GLOB_RECURSE SHION_MODULES "${SHION_SOURCE_DIR}/modules/*.ixx" "${SHION_SOURCE_DIR}/modules/*.cpp")
file(GLOB_RECURSE SHION_TESTS_SOURCES "${CMAKE_CURRENT_LIST_DIR}/src/tests/*.hpp" "${CMAKE_CURRENT_LIST_DIR}/src/tests/*.cpp")
file(GLOB_RECURSE SHION_TEST_MODULES "${SHION_SOURCE_DIR}/tests/*.ixx")

if (SHION_IMPORT_STD)
	set(SHION_PRIVATE_BUILD_DEFINITIONS ${SHION_PRIVATE_BUILD_DEFINITIONS} SHION_IMPORT_STD)
	if (NOT SHION_USE_CMAKE_IMPORT_STD)
		if (SHION_STD_MODULE_LOCATION STREQUAL "")
			if (MSVC AND NOT CLANG_CL)
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
		endif ()
		
		if (SHION_STD_MODULE_LOCATION STREQUAL "")
			message(FATAL_ERROR "Building the std modules with this compiler is not supported")
		endif ()
	else()
		set(CMAKE_CXX_MODULE_STD 1)
	endif ()
endif ()

function(create_library lib bin type)
	add_library(shion-${lib} ${type})
	set_target_properties(shion-${lib} PROPERTIES OUTPUT_NAME ${bin})

	target_compile_options(shion-${lib} PRIVATE ${SHION_PRIVATE_BUILD_OPTIONS})
	target_compile_options(shion-${lib} PUBLIC ${SHION_PUBLIC_BUILD_OPTIONS})
	target_compile_definitions(shion-${lib} PRIVATE ${SHION_PRIVATE_BUILD_DEFINITIONS} SHION_BUILD)
	target_compile_definitions(shion-${lib} PUBLIC ${SHION_PUBLIC_BUILD_DEFINITIONS})
	target_include_directories(shion-${lib} PUBLIC ${SHION_HEADERS_DIR})
	
	target_sources(shion-${lib}
		PUBLIC
			${SHION_HEADERS}
	)
	if (NOT SHION_STD_MODULE_LOCATION STREQUAL "")
		target_compile_options(shion-${lib} PRIVATE "/reference \"${SHION_STD_MODULE_LOCATION}\"")
	endif ()
	if (SHION_LINK_STD_MODULE)
		target_link_libraries(shion-${lib} PUBLIC shion_std_module)
	endif ()
	if (SHION_BUILD_MODULES)
		target_compile_definitions(shion-${lib} PUBLIC SHION_MODULES)
		target_sources(shion-${lib}
			PUBLIC
				FILE_SET CXX_MODULES FILES "${SHION_MODULES}"
		)
		if (MSVC)
			if (NOT CLANG_CL)
				target_compile_options(shion-${lib} PUBLIC /wd5244) # #include <meow> in the purview of module shion.meow appears erroneous
			endif ()
		endif ()
	endif ()
	
	add_library(shion::${lib} ALIAS shion-${lib})
endfunction()

if (SHION_SHARED_LIBRARY)
	create_library(shared "$<IF:$<CONFIG:Debug>,shiond,shion>" SHARED)
	
	target_compile_definitions(shion-shared PUBLIC SHION_SHARED)
	target_sources(shion-shared
		PRIVATE
			${SHION_SOURCES}
	)

	add_library(shion ALIAS shion-shared)
endif ()

if (SHION_STATIC_LIBRARY)
	create_library(static "$<IF:$<CONFIG:Debug>,libshiond,libshion>" STATIC)

	target_sources(shion-static
		PRIVATE
			${SHION_SOURCES}
	)

	if (NOT SHION_SHARED_LIBRARY)
		add_library(shion ALIAS shion-static)
	endif ()
endif ()

if (SHION_MODULE_LIBRARY)
	if (NOT SHION_BUILD_MODULES)
		message(FATAL_ERROR "Building the module library (SHION_MODULE_LIBRARY=on) requires SHION_BUILD_MODULES=on")
	endif ()
	create_library(module "$<IF:$<CONFIG:Debug>,modshiond,modshion>" STATIC)

	target_compile_definitions(shion-module PUBLIC SHION_MODULE_LIBRARY)
	
	if (NOT SHION_SHARED_LIBRARY AND NOT SHION_STATIC_LIBRARY)
		add_library(shion ALIAS shion-module)
	endif ()
endif()

if (SHION_BUILD_TESTS)
	if (NOT SHION_BUILD_MODULES)
		message(FATAL_ERROR "Tests require modules to be built.")
	else ()
		add_executable(tests)
		target_compile_features(tests PUBLIC cxx_std_23)
		target_link_libraries(tests PUBLIC shion)

		target_compile_options(tests PRIVATE ${SHION_PRIVATE_BUILD_OPTIONS})
		target_compile_options(tests PUBLIC ${SHION_PUBLIC_BUILD_OPTIONS})
		target_compile_definitions(tests PRIVATE ${SHION_PRIVATE_BUILD_DEFINITIONS})
		target_compile_definitions(tests PUBLIC ${SHION_PUBLIC_BUILD_DEFINITIONS})
		target_include_directories(tests PUBLIC ${SHION_HEADERS_DIR})

		target_sources(tests PRIVATE ${SHION_TESTS_SOURCES})
		target_sources(tests PRIVATE FILE_SET CXX_MODULES FILES ${SHION_TEST_MODULES})
	endif ()
endif ()
