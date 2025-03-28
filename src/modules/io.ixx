module;

#include <shion/export.hpp>
#include <shion/common/detail.hpp>

#if !SHION_IMPORT_STD
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
#endif

export module shion:io;

#if SHION_IMPORT_STD
import std;
#endif
import :common;
import :utility;

#if SHION_EXTERN_MODULES
extern "C++" {
#endif
	
#include "shion/io/logger.hpp"
#include "shion/io/utils.hpp"

#if SHION_MODULE_LIBRARY or !SHION_EXTERN_MODULES
#include "shion/io/logger.cpp"
#endif

#if SHION_EXTERN_MODULES
}
#endif
