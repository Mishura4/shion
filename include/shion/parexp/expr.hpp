#ifndef SHION_LINQ_H_
#define SHION_LINQ_H_

#include <shion/defines.hpp>

#if !SHION_BUILDING_MODULES

#include <functional>
#include <type_traits>
#include <concepts>
#include <utility>
#include <array>
#include <functional>

#include <shion/utility/supplier.hpp>
#include <shion/monad/invoke.hpp>
#include <shion/monad/compare.hpp>

#endif

namespace shion
{

SHION_EXPORT struct placeholder_t
{
	template <typename T> consteval operator T&() & { throw std::bad_cast{}; }
	template <typename T> consteval operator const T&() const& { throw std::bad_cast{}; }
	template <typename T> consteval operator T&&() && { throw std::bad_cast{}; }
	template <typename T> consteval operator const T&&() const&& { throw std::bad_cast{}; }
};

SHION_EXPORT inline constexpr auto placeholder = placeholder_t{};

SHION_EXPORT template <size_t>
struct placeholder_n_t : placeholder_t
{
};

SHION_EXPORT template <size_t N>
inline constexpr auto placeholder_n = placeholder_n_t<N>{};

namespace detail
{

template <typename... Args>
using linq_tuple = std::tuple<Args...>;

template <typename Operand>
class expr_left : supplier<Operand>
{
public:
	using operand_t = Operand;
	static constexpr inline bool has_operand = true;

	constexpr expr_left() = default;
	constexpr expr_left(expr_left const&) = default;
	constexpr expr_left(expr_left&&) = default;

	template <size_t... Is, typename... Us>
	requires (std::constructible_from<Operand, Us...>)
	constexpr expr_left(std::index_sequence<Is...>, linq_tuple<Us...> us) : supplier<Operand>(get<Is>(us)...) {}

	constexpr expr_left& operator=(expr_left&&) = default;
	constexpr expr_left& operator=(expr_left const&) = default;

	constexpr Operand& operand() & noexcept { return this->get(); }
	constexpr Operand&& operand() && noexcept { return std::move(*this).get(); }
	constexpr Operand const& operand() const& noexcept { return this->get(); }
	constexpr Operand const&& operand() const&& noexcept { return std::move(*this).get(); }
};

template <>
class expr_left<void>
{
public:
	constexpr expr_left() = default;
	constexpr expr_left(expr_left const&) = default;
	constexpr expr_left(expr_left&&) = default;

	constexpr expr_left& operator=(expr_left&&) = default;
	constexpr expr_left& operator=(expr_left const&) = default;

	constexpr void invoke() = delete;
	constexpr void invoke(auto&&, auto&&) = delete;
	constexpr void invoke_left(auto&&) = delete;
	constexpr void invoke_right(auto&&) = delete;
};

// For each argument N:
// Detect if callable can be called with argument N, if yes, use it
// Detect is callable argument N

template <template <typename> typename Trait, typename... Args>
inline constexpr bool trait_to_array[] = { Trait<Args>::value... };

template <template <typename> typename Trait, typename... Args>
inline constexpr size_t find_nth_impl(size_t n = 0) noexcept
{
	constexpr auto& values = trait_to_array<Trait, Args...>;

	size_t i = 0;
	size_t count = 0;
	while (i < sizeof...(Args))
	{
		if (values[i])
		{
			if (count < n)
				++count;
			else
				return i;
		}
		++i;
	}
	return sizeof...(Args);
}

template <template <typename> typename Trait, typename... Args>
struct find_nth_t
{
	static constexpr size_t value = find_nth_impl<Trait, Args...>();
};

template <template <typename> typename Trait, typename... Args>
struct count_t
{
	inline constexpr size_t operator()(size_t begin = 0, size_t end = sizeof...(Args)) noexcept
	{
		constexpr auto& values = trait_to_array<Trait, Args...>;

		size_t i = begin;
		size_t count = 0;
		while (i < end)
		{
			if (values[i])
			{
				++count;
			}
			++i;
		}
		return count;
	}

	inline static constexpr auto value = count_t{}();
};

template <template <typename> typename Trait, size_t N, typename... Args>
inline constexpr auto find_nth = find_nth_t<Trait, Args...>{}(N);

template <template <typename> typename Trait, size_t N, typename... Args>
inline constexpr auto find_nth_invocable = find_nth_t<Trait, Args...>{}(N);

template <template <typename> typename Trait, typename... Args>
inline constexpr auto find_first = find_nth_t<Trait, Args...>{}();

template <template <typename, typename> typename T, typename Rhs>
struct left_fold_t
{
	template <typename Lhs>
	struct type
	{
		static inline constexpr auto value = T<Lhs, Rhs>::value;
	};
};

template <size_t From, size_t... Is, typename Arg, typename BoundedTuple, typename Fun>
constexpr auto _do_wrap(BoundedTuple&& bounded, Fun&& fun, Arg&& arg, std::index_sequence<Is...>)
{
	return [&bounded, f = std::forward<Fun>(fun), &arg]<typename... Args>(Args&&... args) mutable constexpr -> decltype(auto) {
		(void)bounded;
		return std::forward<Fun>(f)(std::forward_like<BoundedTuple>(std::get<From + Is>(bounded))()..., std::forward<Arg>(arg), std::forward<Args>(args)...);
	};
}

template <size_t From, size_t... Is, typename BoundedTuple, typename Fun>
constexpr auto _do_wrap(BoundedTuple&& bounded, Fun&& fun, std::index_sequence<Is...>)
{
	return [&bounded, f = std::forward<Fun>(fun)]<typename... Args>(Args&&... args) mutable constexpr -> decltype(auto) {
		(void)bounded;
		return std::forward<Fun>(f)(std::forward_like<BoundedTuple>(std::get<From + Is>(bounded))()..., std::forward<Args>(args)...);
	};
}

template <typename T>
using invocable_fold = typename left_fold_t<std::is_invocable, T>::type;

template <typename T, typename... Args>
constexpr auto _find_next = [](size_t from) consteval
{
	size_t i = 0;
	size_t count = 0;
	std::array<bool, sizeof...(Args)> values = { std::invocable<Args, T>... };
	while (i < sizeof...(Args))
	{
		if (values[i])
		{
			if (count < from)
				++count;
			else
				return i;
		}
		++i;
	}
	return i;
};

template <size_t From, size_t I, typename... BoundedArgs, typename Fun, typename... Args>
constexpr auto _wrap(const linq_tuple<BoundedArgs...>& bounded, Fun&& fun, linq_tuple<Args...> args)
{
	if constexpr (From >= sizeof...(BoundedArgs))
	{
		auto bind_all_args = [&]<size_t... Is>(std::index_sequence<Is...>) constexpr {
			return [f = std::forward<Fun>(fun), args_ = std::move(args)]() mutable constexpr { return std::forward<Fun>(f)(std::get<I + Is>(args_)...); };
		};

		return bind_all_args(std::make_index_sequence<sizeof...(Args) - I>{});
	}
	else
	{
		if constexpr (I < std::tuple_size_v<linq_tuple<Args...>>)
		{
			using arg_t = std::tuple_element_t<I, linq_tuple<Args...>>;
			constexpr size_t to = _find_next<arg_t, BoundedArgs...>(I);

			return _wrap<to + 1, I + 1>(bounded, _do_wrap<From>(bounded, std::forward<Fun>(fun), std::get<I>(args), std::make_index_sequence<to - From>{}), std::move(args));
		}
		else
		{
			return _do_wrap<From>(bounded, std::forward<Fun>(fun), std::make_index_sequence<sizeof...(BoundedArgs) - From>{});
		}
	}
}

template <typename Left, typename... Bound>
class expr_base : public expr_left<Left>
{
	using my_seq = std::index_sequence_for<Bound...>;

	linq_tuple<supplier<Bound>...> _args;

public:
	template <typename... Ls, typename... Args_>
	constexpr expr_base(linq_tuple<Ls...> left_args, Args_&&... args) :
		expr_left<Left>(std::make_index_sequence<sizeof...(Ls)>{}, std::move(left_args)),
		_args{std::forward<Args_>(args)...}
	{
	}
	template <typename L, typename... Args_>
	constexpr expr_base(L&& left_args, Args_&&... args) :
		expr_left<Left>(std::make_index_sequence<1>{}, std::forward_as_tuple(std::forward<L>(left_args))),
		_args{std::forward<Args_>(args)...}
	{
	}

	template <typename... Args>
	requires (sizeof...(Bound) > 0)
	constexpr auto operator()(Args&&... args) const -> decltype(auto)
	{
		return _wrap<0, 0>(_args, [this]<typename... Args_>(Args_&&... args_) constexpr noexcept { return this->operand()(std::forward<Args_>(args_)...); }, std::forward_as_tuple(std::forward<Args>(args)...))();
	}

	constexpr auto operator()() const -> decltype(auto) requires(sizeof...(Bound) == 0 && std::invocable<Left>)
	{
		return this->operand()();
	}

	constexpr expr_base(const expr_base&) = default;
	constexpr expr_base(expr_base&&) = default;
	constexpr ~expr_base() = default;

	constexpr expr_base& operator=(const expr_base&) = default;
	constexpr expr_base& operator=(expr_base&&) = default;
};

}

SHION_EXPORT template <typename Left, typename... Args>
class expr : detail::expr_base<Left, Args...>
{
	using my_base = detail::expr_base<Left, Args...>;
	
private:
	using base_l = detail::expr_left<Left>;

	template <typename O, typename R> constexpr decltype(auto) binary_predicate(O&& op, R&& r) &;
	template <typename O, typename R> constexpr decltype(auto) binary_predicate(O&& op, R&& r) &&;
	template <typename O, typename R> constexpr decltype(auto) binary_predicate(O&& op, R&& r) const&;
	template <typename O, typename R> constexpr decltype(auto) binary_predicate(O&& op, R&& r) const&&;

public:
	using my_base::my_base;
	using my_base::operator=;
	using my_base::operator();
	using left_t = Left;
	
	template <typename R> constexpr auto operator>(R&& r) & noexcept { return (*this).binary_predicate(monad::greater, std::forward<R>(r)); }
	template <typename R> constexpr auto operator>(R&& r) const& noexcept;
	template <typename R> constexpr auto operator==(R&& r) & noexcept { return (*this).binary_predicate(monad::equal, std::forward<R>(r)); }
	template <typename R> constexpr auto operator==(R&& r) const& noexcept { return (*this).binary_predicate(monad::equal, std::forward<R>(r)); }
};

SHION_EXPORT template <typename Left, typename... Args>
expr(Left&& left, Args&&... args) -> expr<Left, Args...>;

}

#endif /* SHION_LINQ_H_ */
