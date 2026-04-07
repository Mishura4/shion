module;

#include <shion/export.hpp>

export module shion;

export import :common;
export import :utility;
export import :meta;
export import :monad;
#ifndef __clang__ // Glitched on clang 22.1.0 https://github.com/llvm/llvm-project/issues/184957#issuecomment-4060708322
export import :parexp;
#endif
export import :math;
export import :containers;
export import :io;
export import :coro;
