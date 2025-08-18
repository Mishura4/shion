cmake_minimum_required(VERSION 3.31)

if (CMAKE_VERSION VERSION_GREATER_EQUAL 4.0.0)
	set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "a9e1cf81-9932-4810-974b-6eccaf14e457")
elseif (CMAKE_VERSION VERSION_GREATER_EQUAL 3.30)
	set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "0e5b6991-d74f-4b3d-a41c-cf096e0b2508")
endif ()

project(shion LANGUAGES CXX)

include(cmake/AddStdModule.cmake)

option(SHION_BUILD_MODULES "Whether to build the library as a module" on)
option(SHION_IMPORT_STD "Whether to use `import std;` when building the library modules" on)
option(SHION_BUILD_TESTS "Whether to build the tests" on)
set(SHION_STD_MODULE_LOCATION "" CACHE STRING "Specify a custom location for the std module, or build it if empty")

set(SHION_BUILD_TESTS on)
set(CMAKE_CXX_STANDARD 23)

if (LINUX)
	option(SHION_USE_CMAKE_IMPORT_STD "Whether to use CMake's import std feature" off)
	option(SHION_EXTERN_MODULES "Whether to wrap the module symbols in `extern \"C++\"` for traditional linkage" on)
else ()
	option(SHION_USE_CMAKE_IMPORT_STD "Whether to use CMake's import std feature" off)
	option(SHION_EXTERN_MODULES "Whether to wrap the module symbols in `extern \"C++\"` for traditional linkage" on)
endif ()

if (SHION_USE_CMAKE_IMPORT_STD)
	cmake_minimum_required(VERSION 3.30 FATAL_ERROR)

	set(CMAKE_CXX_MODULE_STD 1)
endif ()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	set(CLANG true)
	if (MSVC)
		set(CLANG_CL true)
	endif ()
endif()

if (${BUILD_SHARED_LIBS})
	set(SHION_BUILD_STATIC off)
else ()
	set(SHION_BUILD_STATIC on)
endif ()


set(SHION_STATIC_LIBRARY ${SHION_BUILD_STATIC} CACHE BOOL "Whether to build shion as a static library")
set(SHION_MODULE_LIBRARY ${SHION_BUILD_MODULES} CACHE BOOL "Whether to build shion as a module library")
set(SHION_SHARED_LIBRARY ${BUILD_SHARED_LIBS} CACHE BOOL "Whether to build shion as a shared library")

set(SHION_SHARED_LIBRARY on)
set(SHION_STATIC_LIBRARY on)

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


set(SHION_HEADERS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(SHION_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/src")

file(GLOB_RECURSE SHION_HEADERS "${SHION_HEADERS_DIR}/shion/*.hpp")
file(GLOB_RECURSE SHION_SOURCES "${SHION_SOURCE_DIR}/shion/*.hpp" "${SHION_SOURCE_DIR}/shion/*.cpp")
file(GLOB_RECURSE SHION_MODULE_INTERFACES "${SHION_HEADERS_DIR}/shion/*.ixx")
file(GLOB_RECURSE SHION_MODULE_PARTITIONS "${SHION_HEADERS_DIR}/shion/*.cppm")
file(GLOB_RECURSE SHION_MODULE_IMPLEMENTATIONS "${SHION_SOURCE_DIR}/shion/*.cxx")
file(GLOB_RECURSE SHION_TESTS_SOURCES "${CMAKE_CURRENT_LIST_DIR}/src/tests/*.hpp" "${CMAKE_CURRENT_LIST_DIR}/src/tests/*.cpp")
file(GLOB_RECURSE SHION_TEST_MODULES "${SHION_SOURCE_DIR}/tests/*.ixx")

if (NOT DEFINED CMAKE_INSTALL_BINDIR)
	set(CMAKE_INSTALL_BINDIR bin/)
endif ()
if (NOT DEFINED CMAKE_INSTALL_INCLUDEDIR)
	set(CMAKE_INSTALL_INCLUDEDIR include/)
endif ()
if (NOT DEFINED CMAKE_INSTALL_LIBDIR)
	set(CMAKE_INSTALL_LIBDIR lib/)
endif ()

if (SHION_IMPORT_STD)
	set(SHION_PRIVATE_BUILD_DEFINITIONS ${SHION_PRIVATE_BUILD_DEFINITIONS} SHION_IMPORT_STD=1)
	if (NOT SHION_USE_CMAKE_IMPORT_STD AND NOT SHION_STD_MODULE_TARGET)
		AddStdModule(shion_std_module)
		set(SHION_STD_MODULE_TARGET shion_std_module)

		target_compile_features(shion_std_module PUBLIC cxx_std_23)

		install(TARGETS shion_std_module
			EXPORT shion_std_module
			RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
			LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
			ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
			INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
			FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
			FILE_SET CXX_MODULES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
		)

		install(EXPORT shion_std_module
			DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/shion
			NAMESPACE shion::
			CXX_MODULES_DIRECTORY modules
		)
	else()
		set(CMAKE_CXX_MODULE_STD 1)
	endif ()
endif ()

function(create_library lib bin type)
	add_library(shion-${lib} ${type})
	set_target_properties(shion-${lib} PROPERTIES OUTPUT_NAME ${bin})

	target_compile_options(shion-${lib} PRIVATE ${SHION_PRIVATE_BUILD_OPTIONS})
	target_compile_options(shion-${lib} PUBLIC ${SHION_PUBLIC_BUILD_OPTIONS})
	target_compile_definitions(shion-${lib} PRIVATE
		${SHION_PRIVATE_BUILD_DEFINITIONS}
		SHION_BUILD SHION_BUILD_ROOT="${CMAKE_SOURCE_DIR}"
	)
	target_compile_definitions(shion-${lib} PUBLIC ${SHION_PUBLIC_BUILD_DEFINITIONS})
	target_include_directories(shion-${lib}
		PUBLIC
			"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
			"$<INSTALL_INTERFACE:include>"
	)
	target_include_directories(shion-${lib} PRIVATE ${SHION_SOURCE_DIR})
	
	target_sources(shion-${lib}
		PUBLIC FILE_SET HEADERS
			BASE_DIRS ${SHION_HEADERS_DIR}
			FILES ${SHION_HEADERS}
	)

	if (NOT SHION_STD_MODULE_LOCATION STREQUAL "")
		target_compile_options(shion-${lib} PRIVATE "/reference \"${SHION_STD_MODULE_LOCATION}\"")
	endif ()

	if (SHION_STD_MODULE_TARGET)
		target_link_libraries(shion-${lib} PUBLIC ${SHION_STD_MODULE_TARGET})
	endif ()

	if (SHION_BUILD_MODULES)
		target_compile_definitions(shion-${lib} PUBLIC SHION_MODULES)
		target_sources(shion-${lib}
			PUBLIC FILE_SET CXX_MODULES
				BASE_DIRS ${SHION_HEADERS_DIR}
				FILES ${SHION_MODULE_INTERFACES} ${SHION_MODULE_PARTITIONS}
		)

		if (SHION_MODULE_IMPLEMENTATIONS)
			target_sources(shion-${lib}
				PRIVATE
					${SHION_MODULE_IMPLEMENTATIONS}
			)
		endif ()

		foreach (file in ${SHION_MODULE_IMPLEMENTATIONS}})
			set_source_files_properties(${file} PROPERTIES
				CMAKE_CXX_SCAN_FOR_MODULES ON
			)
		endforeach ()
	else ()
		target_sources(shion-${lib}
			PRIVATE
				${SHION_SOURCES}
		)
	endif ()

	install(TARGETS shion-${lib}
		EXPORT shion-${lib}
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
		INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
		FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
		FILE_SET CXX_MODULES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
	)

	install(EXPORT shion-${lib}
		DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/shion
		NAMESPACE shion::
		CXX_MODULES_DIRECTORY modules
	)
endfunction()

if (SHION_STATIC_LIBRARY)
	create_library(static "$<IF:$<CONFIG:Debug>,libshiond,libshion>" STATIC)
	

	set(SHION_DEFAULT_LIB shion-static)
endif ()

if (SHION_SHARED_LIBRARY)
	create_library(shared "$<IF:$<CONFIG:Debug>,shiond,shion>" SHARED)
	
	target_compile_definitions(shion-shared PUBLIC SHION_SHARED)

	set(SHION_DEFAULT_LIB shion-shared)
endif ()

add_library(shion::shion ALIAS ${SHION_DEFAULT_LIB})

if (SHION_BUILD_TESTS)
	if (NOT SHION_BUILD_MODULES)
		message(FATAL_ERROR "Tests require modules to be built.")
	else ()
		add_executable(tests)
		target_compile_features(tests PUBLIC cxx_std_23)
		target_link_libraries(tests PUBLIC shion::shion)

		target_compile_options(tests PRIVATE ${SHION_PRIVATE_BUILD_OPTIONS})
		target_compile_options(tests PUBLIC ${SHION_PUBLIC_BUILD_OPTIONS})
		target_compile_definitions(tests PRIVATE ${SHION_PRIVATE_BUILD_DEFINITIONS})
		target_compile_definitions(tests PUBLIC ${SHION_PUBLIC_BUILD_DEFINITIONS})
		target_include_directories(tests PUBLIC ${SHION_HEADERS_DIR})

		target_sources(tests PRIVATE ${SHION_TESTS_SOURCES})
		target_sources(tests PRIVATE FILE_SET CXX_MODULES FILES ${SHION_TEST_MODULES})
	endif ()
endif ()
