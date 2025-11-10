function(AddStdModule NAME)
	set(CMAKE_EXPERIMENTAL_CXX_MODULE_DYNDEP 1)
	add_library(${NAME} STATIC EXCLUDE_FROM_ALL)

	if (MSVC)
		set(ModuleSystemDir "$ENV{VCToolsInstallDir}")
		cmake_path(APPEND ModuleSystemDir "modules")


		cmake_path(APPEND ModuleSystemDir "std.ixx" OUTPUT_VARIABLE ModulePath)
		list(APPEND SystemModules ${ModulePath})
		cmake_path(APPEND ModuleSystemDir "std.compat.ixx" OUTPUT_VARIABLE ModulePath)
		list(APPEND SystemModules ${ModulePath})
	else ()
		include(CheckCXXSourceCompiles)

		check_cxx_source_compiles("
			#include <cstdint>
			
			int a = _LIBCPP_STD_VER;
			int main() {}
		" IS_LIBCXX)
		if (IS_LIBCXX)
			set(ModulesJsonName "libc++.modules.json")
		else ()
			set(ModulesJsonName "libstdc++.modules.json")
		endif ()

		set(ModulesJsonDetect ${CMAKE_CXX_COMPILER} -print-file-name=${ModulesJsonName})

		execute_process(
			COMMAND ${ModulesJsonDetect}
			OUTPUT_VARIABLE ModulesJsonPath
			RESULT_VARIABLE ModulesJsonResult
			ERROR_VARIABLE  ModulesJsonError
			OUTPUT_STRIP_TRAILING_WHITESPACE
		)

		if (NOT ${ModulesJsonResult} EQUAL 0)
			message(FATAL_ERROR "Cannot build std module: compiler ${CMAKE_CXX_COMPILER} didn't provide modules.json (${ModulesJsonResult})")
		endif ()

		cmake_path(ABSOLUTE_PATH ModulesJsonPath NORMALIZE)
		cmake_path(REMOVE_FILENAME ModulesJsonPath OUTPUT_VARIABLE ModulesJsonDir)

		file(READ "${ModulesJsonPath}" ModulesJson)

		cmake_path(REMOVE_FILENAME ModulesJsonPath OUTPUT_VARIABLE ModuleSystemDir)
		
		string(JSON ModuleCount              LENGTH ${ModulesJson} modules)
		math(EXPR ModuleCount "${ModuleCount} - 1")

		foreach (modi RANGE 0 ${ModuleCount})
			string(JSON ModuleSystemPath         GET    ${ModulesJson} modules ${modi} source-path)
			cmake_path(ABSOLUTE_PATH ModuleSystemPath BASE_DIRECTORY ${ModuleSystemDir} NORMALIZE)
			list(APPEND SystemModules ${ModuleSystemPath})
			string(JSON ModuleSystemIncludeCount LENGTH ${ModulesJson} modules ${modi} local-arguments system-include-directories)
			math(EXPR ModuleSystemIncludeCount "${ModuleSystemIncludeCount} - 1")
			foreach (inci RANGE 0 ${ModuleSystemIncludeCount})
				string(JSON IncludePath GET ${ModulesJson} modules ${modi} local-arguments system-include-directories ${inci})
				cmake_path(ABSOLUTE_PATH IncludePath BASE_DIRECTORY ${ModuleSystemDir} NORMALIZE)
				list(APPEND ModuleSystemInclude ${IncludePath})
			endforeach ()
		endforeach ()

		if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
			target_compile_options(${NAME} PRIVATE "-Wno-reserved-module-identifier")
		endif ()

		target_include_directories(${NAME} SYSTEM PRIVATE ${ModuleSystemInclude})
	endif ()

	set(ModulePath "${PROJECT_BINARY_DIR}")

	foreach (file ${SystemModules})
		cmake_path(GET file FILENAME ModuleFileName)
		set(modfile ${CMAKE_CURRENT_BINARY_DIR})
		cmake_path(APPEND modfile ${ModuleFileName})

		file(COPY_FILE
			${file}
			${modfile}
		)

		list(APPEND Modules ${modfile})
	endforeach ()

	set(SHION_LINK_STD_MODULE on PARENT_SCOPE)

	target_sources(${NAME}
		PUBLIC FILE_SET CXX_MODULES
		BASE_DIRS ${CMAKE_CURRENT_BINARY_DIR}
		FILES ${Modules}
	)

	target_compile_features(${NAME} PUBLIC cxx_std_23)
	target_compile_options(${NAME} PRIVATE ${SHION_PRIVATE_BUILD_OPTIONS})
	target_compile_options(${NAME} PUBLIC ${SHION_PUBLIC_BUILD_OPTIONS})
	target_compile_definitions(${NAME} PRIVATE ${SHION_PRIVATE_BUILD_DEFINITIONS})
	target_compile_definitions(${NAME} PUBLIC ${SHION_PPUBLIC_BUILD_DEFINITIONS})
endfunction ()
