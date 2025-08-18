module;

#include <shion/export.hpp>
#include <shion/common/defines.hpp>

#if !SHION_IMPORT_STD
#include <cstring>
#include <cstddef>
#include <cstdio>
#include <tuple>
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
#include <span>
#include <bit>
#include <source_location>

#include <chrono> // Workaround g++ bug
#endif

export module shion:io;

#if SHION_IMPORT_STD
import std;
#endif

import :common;
import :utility;
import :meta;
import :monad;
import :coro;

using namespace SHION_NAMESPACE ::literals;
using namespace std::string_view_literals;
using namespace std::chrono_literals;

#include "shion/io/utils.hpp"
#include "shion/io/serializer.hpp"
#include "shion/io/logger.hpp"

