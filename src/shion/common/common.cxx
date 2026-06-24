module;

#include <shion/export.hpp>
#include <shion/common/defines.hpp>

#if _WIN32
#include <Windows.h>
#endif

#if !SHION_IMPORT_STD
#include <cstdlib>
#include <cassert>
#include <filesystem>
#include <iostream>
#include <syncstream>
#include <version>
#include <source_location>
#include <ranges>
#include <algorithm>
#include <atomic>
#endif

module shion;

#if SHION_IMPORT_STD
import std;
#endif

#include "exception.cpp"
#include "assert.cpp"
