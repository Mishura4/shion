module;

#include <shion/export.hpp>

#include <compare> // MSVC; it is a mystery as to why it's required here of all places

export module shion;

export import :common;
export import :utility;
export import :meta;
export import :monad;
export import :parexp;
export import :math;
export import :containers;
export import :io;
export import :coro;
