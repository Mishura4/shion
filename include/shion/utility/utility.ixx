module;

#include <shion/export.hpp>

#if !SHION_IMPORT_STD

#include <algorithm>
#include <tuple>
#include <utility>
#include <span>
#include <ranges>
#include <string_view>
#include <exception>
#include <stdexcept>
#include <memory>
#include <type_traits>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <charconv>
#include <random>
#include <chrono>
#include <thread>
#include <bit>
#include <compare>
#include <type_traits>
#include <concepts>
#include <functional>
#include <bit>

#include <chrono> // Workaround g++ bug

#endif

export module shion:utility;

#if SHION_IMPORT_STD
import std;
#endif
import :common;
import :meta;

using namespace SHION_NAMESPACE ::literals;

#include "shion/utility/bit_mask.hpp"
#include "shion/utility/supplier.hpp"
#include "shion/utility/tuple.hpp"
#include "shion/utility/optional.hpp"
#include "shion/utility/raii_ptr.hpp"
#include "shion/utility/string_literal.hpp"
#include "shion/utility/unique_handle.hpp"
#include "shion/utility/uuid.hpp"
#include "shion/utility/bit_mask.hpp"
