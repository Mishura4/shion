module;

#include <shion/export.hpp>

#if !SHION_IMPORT_STD
#  include <concepts>
#  include <type_traits>
#  include <chrono>
#  include <tuple>
#endif

export module shion:meta;

import :common;

#if SHION_IMPORT_STD
import std;
#endif

#include "shion/meta/type_traits.hpp"
#include "shion/meta/type_list.hpp"
