module;

#include <shion/export.hpp>

#if !SHION_IMPORT_STD
#include <functional>
#include <utility>
#include <type_traits>
#include <concepts>
#endif

export module shion:monad;

#if SHION_IMPORT_STD
import std;
#endif

#include "shion/monad/invoke.hpp"
#include "shion/monad/compare.hpp"
#include "shion/monad/object.hpp"
