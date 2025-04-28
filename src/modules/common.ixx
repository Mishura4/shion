module;

#include <shion/export.hpp>

#if !SHION_IMPORT_STD
#	include <chrono>
#	include <concepts>
#	include <type_traits>
#	include <string_view>
#	include <source_location>
#	include <format>
#	include <exception>
#	include <string>
#	include <functional>
#	include <fstream>
#endif

#include <cstddef>
#include <cstdint>
#include <cassert>

export module shion:common;

#if SHION_IMPORT_STD
import std;
#endif

#include "shion/common/defines.hpp"
#include "shion/common/assert.hpp"
#include "shion/common/detail.hpp"
#include "shion/common/types.hpp"
#include "shion/common/cast.hpp"
#include "shion/common/literals.hpp"
#include "shion/common/exception.hpp"
#include "shion/common/tools.hpp"
	
// #include "shion/common/exception.cpp"
