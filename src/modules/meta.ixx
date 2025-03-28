module;

#include <shion/export.hpp>

#if !SHION_IMPORT_STD
#  include <concepts>
#  include <type_traits>
#  include <chrono>
#endif

export module shion:meta;

#if SHION_IMPORT_STD
import std;
#endif

#include "shion/meta/type_traits.hpp"
#include "shion/meta/type_list.hpp"
