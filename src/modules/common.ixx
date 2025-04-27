module;

#include <shion/export.hpp>

#if !SHION_IMPORT_STD
#  include <chrono>
#  include <concepts>
#  include <type_traits>
#  include <string_view>
#  include <source_location>
#  include <format>
#  include <exception>
#  include <string>
#  include <functional>
#endif

#include <cstddef>
#include <cstdint>
#include <cassert>

export module shion:common;

#if SHION_IMPORT_STD
import std;
#endif

#if SHION_EXTERN_MODULES
extern "C++" {
#endif

#include "shion/common/defines.hpp"
#include "shion/common/types.hpp"
#include "shion/common/cast.hpp"
#include "shion/common/literals.hpp"
#include "shion/common/exception.hpp"
#include "shion/common/tools.hpp"
	
#if SHION_EXTERN_MODULES
}
	
#if SHION_EXTERN_MODULES
extern "C++" {
#endif
#if SHION_MODULE_LIBRARY or !SHION_EXTERN_MODULES
#include "shion/common/exception.cpp"
#endif
}

#endif