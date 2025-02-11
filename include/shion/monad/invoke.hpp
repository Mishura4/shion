#ifndef SHION_MONAD_INVOKE_H_
#define SHION_MONAD_INVOKE_H_

#ifdef SHION_USE_MODULES

import std;

#else

#include <functional>

#endif

#include <shion/decl.hpp>

namespace shion
{

namespace monad
{

SHION_DECL struct invoke_t {
	template <typename Fun, typename... Args>
	constexpr auto operator()(Fun&& fun, Args&&... args) const -> decltype(auto)
	{
		return std::invoke(std::forward<Fun>(fun), std::forward<Args>(args)...);
	}
};

SHION_DECL_INLINE constexpr auto invoke = invoke_t{};

SHION_DECL template <typename T>
struct cast_t {
	template <typename U>
	constexpr auto operator()(U&& u) const -> decltype(auto)
	{
		return static_cast<T>(std::forward<U>(u));
	}
};

SHION_DECL template <typename T>
inline constexpr auto cast = cast_t<T>{};

}

}

#endif /* SHION_MONAD_INVOKE_H_ */
