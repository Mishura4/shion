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
#include <mutex>
#include <format>
#include <coroutine>
#endif

export module shion:coro;

#if SHION_IMPORT_STD
import std;
#endif
import :common;
import :utility;
import :meta;

#if SHION_EXTERN_MODULES
extern "C++" {
#endif
	
#include "shion/coro.hpp"

#if SHION_EXTERN_MODULES
}
#endif
