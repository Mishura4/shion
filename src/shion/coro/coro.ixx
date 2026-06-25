module;

#include <shion/export.hpp>
#include <shion/common/defines.hpp>
#include <shion/meta/macros.hpp>

#if !SHION_IMPORT_STD
#include <algorithm>
#include <atomic>
#include <atomic>
#include <concepts>
#include <condition_variable>
#include <coroutine>
#include <exception>
#include <format>
#include <functional>
#include <iostream>
#include <memory>
#include <memory>
#include <mutex>
#include <ranges>
#include <shared_mutex>
#include <source_location>
#include <span>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
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
