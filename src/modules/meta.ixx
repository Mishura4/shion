module;

#include <shion/export.hpp>

#if !SHION_IMPORT_STD
#  include <concepts>
#  include <type_traits>
#  include <chrono>
#endif

export module shion:meta;

import std;

#include "shion/meta/type_traits.hpp"
#include "shion/meta/type_list.hpp"
