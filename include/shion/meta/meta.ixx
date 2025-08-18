module;

#include <shion/export.hpp>
#include <shion/common/defines.hpp>

#if !SHION_IMPORT_STD
#  include <concepts>
#  include <type_traits>
#  include <chrono>
#  include <tuple>
#endif

export module shion:meta;

#if SHION_IMPORT_STD
import std;
#endif

import :common;

using namespace SHION_NAMESPACE ::literals;

#include "shion/meta/type_traits.hpp"
#include "shion/meta/type_list.hpp"
