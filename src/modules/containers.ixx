module;

#include <shion/export.hpp>
#include <shion/common/detail.hpp>

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
#endif

export module shion:containers;

#if SHION_IMPORT_STD
import std;
#endif
import :common;
import :utility;
import :meta;

using namespace SHION_NAMESPACE ::literals;

#if SHION_EXTERN_MODULES
extern "C++" {
#endif
	
#include "shion/containers/hive.hpp"

#if SHION_EXTERN_MODULES
}
#endif
