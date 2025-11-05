module;

#include <shion/export.hpp>
#include <shion/common/defines.hpp>

#if !SHION_IMPORT_STD
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <syncstream>
#include <version>

#if _WIN32
#include <Windows.h>
#endif

#endif

module shion;

#if SHION_IMPORT_STD
import std;
#endif

#include "exception.cpp"
#include "assert.cpp"
