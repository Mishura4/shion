module;

#include <shion/export.hpp>
#include <shion/common/defines.hpp>
#include <shion/meta/macros.hpp>

#if !SHION_IMPORT_STD
#include <algorithm>
#include <utility>
#include <span>
#include <ranges>
#include <string_view>
#include <exception>
#include <stdexcept>
#include <memory>
#include <type_traits>
#include <concepts>
#include <variant>
#include <filesystem>
#include <fstream>
#include <functional>
#include <list>
#include <format>
#include <unordered_set>
#include <unordered_map>
#include <source_location>
#if defined(__cpp_lib_generator) && __cpp_lib_generator >= 202207L
#include <generator>
#endif
#endif

export module shion:containers;

#if SHION_IMPORT_STD
import std;
#endif
import :common;
import :utility;
import :meta;
import :coro;

using namespace SHION_NAMESPACE ::literals;

#if SHION_EXTERN_MODULES
extern "C++" {
#endif

#include "shion/containers/hive.hpp"
#include "shion/containers/concurrent_flush.hpp"

#if SHION_EXTERN_MODULES
}
#endif
