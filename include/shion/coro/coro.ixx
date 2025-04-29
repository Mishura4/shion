module;

#include <shion/export.hpp>

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
#include <functional>
#include <mutex>
#include <atomic>
#include <shared_mutex>
#include <format>
#include <coroutine>
#include <iostream>
#include <condition_variable>
#endif

export module shion:coro;

#if SHION_IMPORT_STD
import std;
#endif
import :common;
import :utility;
import :meta;

using namespace SHION_NAMESPACE ::literals;
	
#include "shion/coro.hpp"
