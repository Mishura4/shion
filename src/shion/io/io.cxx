module;

#include <shion/export.hpp>
#include <shion/common/defines.hpp>

#include <variant>
#include <filesystem>
#include <fstream>
#include <functional>
#include <mutex>
#include <memory>
#include <utility>
#include <format>
#include <chrono>

module shion;

using namespace SHION_NAMESPACE ::literals;
using namespace std::string_view_literals;
using namespace std::chrono_literals;

#include "logger.cpp"
