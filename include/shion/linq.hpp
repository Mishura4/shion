#ifndef SHION_LINQ_H_
#define SHION_LINQ_H_

#include <functional>
#include <type_traits>
#include <concepts>
#include <utility>

#include <shion/shion_essentials.hpp>

namespace shion
{

namespace monad
{

struct invoke_t {
	template <typename Fun, typename... Args>
	constexpr auto operator()(Fun&& fun, Args&&... args) -> decltype(auto) {
		return std::invoke(std::forward<Fun>(fun), std::forward<Args>(args)...);
	}
};

inline constexpr auto invoke = invoke_t{};

struct equal_t {
	constexpr auto operator()(const auto& lhs, const auto& rhs) const -> decltype(auto)
	{
		return lhs == rhs;
	}
};

inline constexpr auto equal = equal_t{};

struct not_equal_t {
	constexpr auto operator()(const auto& lhs, const auto& rhs) const -> decltype(auto)
	{
		return lhs != rhs;
	}
};

inline constexpr auto not_equal = not_equal_t{};

struct greater_t {
	constexpr auto operator()(const auto& lhs, const auto& rhs) const -> decltype(auto)
	{
		return lhs > rhs;
	}
};

inline constexpr auto greater = greater_t{};

struct greater_eq_t {
	constexpr auto operator()(const auto& lhs, const auto& rhs) const -> decltype(auto)
	{
		return lhs >= rhs;
	}
};

inline constexpr auto greater_eq = greater_eq_t{};

struct less_t {
	constexpr auto operator()(const auto& lhs, const auto& rhs) const -> decltype(auto)
	{
		return lhs < rhs;
	}
};

inline constexpr auto less = less_t{};

struct less_eq_t {
	constexpr auto operator()(const auto& lhs, const auto& rhs) const -> decltype(auto)
	{
		return lhs <= rhs;
	}
};

inline constexpr auto less_eq = less_eq_t{};

struct spaceship_t {
	constexpr auto operator()(const auto& lhs, const auto& rhs) const -> decltype(auto)
	{
		return lhs <=> rhs;
	}
};

inline constexpr auto spaceship = spaceship_t{};

template <typename T>
struct cast_t {
	template <typename U>
	constexpr auto operator()(U&& u) const -> decltype(auto)
	{
		return static_cast<T>(std::forward<U>(u));
	}
};

template <typename T>
inline constexpr auto cast = cast_t<T>{};

}

namespace tag
{

struct left_t {};
struct right_t {};

inline constexpr auto left = left_t{};
inline constexpr auto right = right_t{};

}

namespace detail
{

template <typename Operand>
class linq_operand
{
private:
	Operand _operand;

public:
	using operand_t = Operand;
	static constexpr inline bool has_operand = true;

	constexpr linq_operand() = default;
	constexpr linq_operand(linq_operand const&) = default;
	constexpr linq_operand(linq_operand&&) = default;

	template <typename U>
	requires (std::constructible_from<Operand, U>)
	constexpr linq_operand(U&& u) : _operand(std::forward<U>(u)) {}

	constexpr linq_operand& operator=(linq_operand&&) = default;
	constexpr linq_operand& operator=(linq_operand const&) = default;

	Operand& operand() & noexcept { return _operand; }
	Operand&& operand() && noexcept { return std::move(_operand); }
	Operand const& operand() const& noexcept { return _operand; }
	Operand const&& operand() const&& noexcept { return std::move(_operand); }

	constexpr decltype(auto) invoke() & requires(std::invocable<Operand&>) { return std::invoke(_operand); }
	constexpr decltype(auto) invoke() && requires(std::invocable<Operand &&>) { return std::invoke(std::move(_operand)); }
	constexpr decltype(auto) invoke() const& requires(std::invocable<Operand const&>) { return std::invoke(_operand); }
	constexpr decltype(auto) invoke() const&& requires(std::invocable<Operand const&&>) { return std::invoke(std::move(_operand)); }

	template <typename T, typename U>
	constexpr decltype(auto) invoke(T&& lhs, U&& rhs) & requires(std::invocable<Operand&, T, U>) { return std::invoke(_operand, std::forward<T>(lhs), std::forward<U>(rhs)); }
	template <typename T, typename U>
	constexpr decltype(auto) invoke(T&& lhs, U&& rhs) && requires(std::invocable<Operand &&, T, U>) { return std::invoke(std::move(_operand), std::forward<T>(lhs), std::forward<U>(rhs)); }
	template <typename T, typename U>
	constexpr decltype(auto) invoke(T&& lhs, U&& rhs) const& requires(std::invocable<Operand const&, T, U>) { return std::invoke(_operand, std::forward<T>(lhs), std::forward<U>(rhs)); }
	template <typename T, typename U>
	constexpr decltype(auto) invoke(T&& lhs, U&& rhs) const&& requires(std::invocable<Operand const&&, T, U>) { return std::invoke(std::move(_operand), std::forward<T>(lhs), std::forward<U>(rhs)); }
	
	template <typename T> constexpr decltype(auto) invoke_left(T&& left) & requires(std::is_member_pointer_v<Operand>) { return std::invoke(_operand, std::forward<T>(left)); }
	template <typename T> constexpr decltype(auto) invoke_left(T&& left) && requires(std::is_member_pointer_v<Operand>) { return std::invoke(std::move(_operand), std::forward<T>(left)); }
	template <typename T> constexpr decltype(auto) invoke_left(T&& left) const& requires(std::is_member_pointer_v<Operand>) { return std::invoke(_operand, std::forward<T>(left)); }
	template <typename T> constexpr decltype(auto) invoke_left(T&& left) const&& requires(std::is_member_pointer_v<Operand>) { return std::invoke(std::move(_operand), std::forward<T>(left)); }
	
	template <typename T> constexpr decltype(auto) invoke_right(T&& right) & requires(!std::is_member_pointer_v<Operand>) { return std::invoke(_operand, std::forward<T>(right)); }
	template <typename T> constexpr decltype(auto) invoke_right(T&& right) && requires(!std::is_member_pointer_v<Operand>) { return std::invoke(std::move(_operand), std::forward<T>(right)); }
	template <typename T> constexpr decltype(auto) invoke_right(T&& right) const& requires(!std::is_member_pointer_v<Operand>) { return std::invoke(_operand, std::forward<T>(right)); }
	template <typename T> constexpr decltype(auto) invoke_right(T&& right) const&& requires(!std::is_member_pointer_v<Operand>) { return std::invoke(std::move(_operand), std::forward<T>(right)); }
};

template <>
class linq_operand<void>
{
public:
	static constexpr inline bool has_operand = false;

	constexpr linq_operand() = default;
	constexpr linq_operand(linq_operand const&) = default;
	constexpr linq_operand(linq_operand&&) = default;

	constexpr linq_operand& operator=(linq_operand&&) = default;
	constexpr linq_operand& operator=(linq_operand const&) = default;

	constexpr void invoke() = delete;
	constexpr void invoke(auto&&, auto&&) = delete;
	constexpr void invoke_left(auto&&) = delete;
	constexpr void invoke_right(auto&&) = delete;
};


template <typename T>
class linq_left
{
private:
	T _left;

public:
	static constexpr inline bool has_left = true;

	constexpr linq_left() = default;
	template <typename... Args, size_t... Is>
	constexpr linq_left(std::piecewise_construct_t, std::tuple<Args...> args, std::index_sequence<Is...>) : _left(std::get<Is>(args)...) {}

	template <typename L> requires (std::constructible_from<T, L>)
	constexpr linq_left(L&& left) : _left(std::forward<L>(left)) {}

	constexpr linq_left(const linq_left&) = default;
	constexpr linq_left(linq_left&&) = default;
	constexpr ~linq_left() = default;

	constexpr linq_left& operator=(const linq_left&) = default;
	constexpr linq_left& operator=(linq_left&&) = default;
	
	constexpr decltype(auto) left() & noexcept { return _left; }
	constexpr decltype(auto) left() && noexcept { return _left; }
	constexpr decltype(auto) left() const& noexcept { return _left; }
	constexpr decltype(auto) left() const&& noexcept { return _left; }
};

template <>
class linq_left<void>
{
public:
	static constexpr inline bool has_left = false;

	constexpr linq_left() = default;
	constexpr linq_left(const linq_left&) = default;
	constexpr linq_left(linq_left&&) = default;
	constexpr ~linq_left() = default;

	constexpr linq_left& operator=(const linq_left&) = default;
	constexpr linq_left& operator=(linq_left&&) = default;

	constexpr void left() = delete;
};

template <typename T>
class linq_right
{
private:
	T _right;

public:
	static constexpr inline bool has_right = true;

	constexpr linq_right() = default;
	template <typename... Args, size_t... Is>
	constexpr linq_right(std::piecewise_construct_t, std::tuple<Args...> args, std::index_sequence<Is...>) : _right(std::get<Is>(args)...) {}

	template <typename L> requires (std::constructible_from<T, L>)
	constexpr linq_right(L&& right) : _right(std::forward<L>(right)) {}

	constexpr linq_right(const linq_right&) = default;
	constexpr linq_right(linq_right&&) = default;
	constexpr ~linq_right() = default;

	constexpr linq_right& operator=(const linq_right&) = default;
	constexpr linq_right& operator=(linq_right&&) = default;
	
	constexpr decltype(auto) right() & noexcept { return _right; }
	constexpr decltype(auto) right() && noexcept { return std::move(_right); }
	constexpr decltype(auto) right() const& noexcept { return _right; }
	constexpr decltype(auto) right() const&& noexcept { return std::move(_right); }
};

template <>
class linq_right<void>
{
public:
	static constexpr inline bool has_right = false;

	constexpr linq_right() = default;
	constexpr linq_right(const linq_right&) = default;
	constexpr linq_right(linq_right&&) = default;
	constexpr ~linq_right() = default;

	constexpr linq_right& operator=(const linq_right&) = default;
	constexpr linq_right& operator=(linq_right&&) = default;

	constexpr void right() = delete;
};

template <typename T>
linq_left(T left) -> linq_left<T>;

template <typename T>
linq_right(T right) -> linq_right<T>;

template <typename Left, typename Operand, typename Right>
class linq_base : public linq_left<Left>, linq_operand<Operand>, linq_right<Right>
{
public:
	template <typename L>
	constexpr linq_base(L&& left) noexcept(std::is_nothrow_constructible_v<Left, L>) : detail::linq_left<Left>(std::forward<L>(left)) {}

	template <typename L, typename O, typename R>
	constexpr linq_base(L&& left, O&& operand, R&& right) noexcept(std::is_nothrow_constructible_v<Left, L> && std::is_nothrow_constructible_v<Operand, O> && std::is_nothrow_constructible_v<Right, R>) :
		detail::linq_left<Left>(std::forward<L>(left)),
		detail::linq_operand<Operand>(std::forward<O>(operand)),
		detail::linq_right<Right>(std::forward<R>(right))
	{
	}

	constexpr linq_base(const linq_base&) = default;
	constexpr linq_base(linq_base&&) = default;
	constexpr ~linq_base() = default;

	constexpr linq_base& operator=(const linq_base&) = default;
	constexpr linq_base& operator=(linq_base&&) = default;
};

template <typename Left, typename Operand, typename Right>
class linq_invoke : linq_base<Left, Operand, Right>
{
	using my_base = linq_base<Left, Operand, Right>;
	using my_left = linq_left<Left>;
	using my_right = linq_right<Right>;
	using my_op = linq_operand<Operand>;

	using my_op::invoke;
	using my_left::left;
	using my_right::right;

public:
	using my_base::my_base;
	using my_base::operator=;

	constexpr auto operator()() const -> decltype(auto) requires (std::invocable<Operand, Left, Right>)
	{
		return this->invoke(left(), right());
	}

	template <typename Lhs>
	constexpr auto operator()(Lhs&& lhs) const -> decltype(auto) requires (std::invocable<Operand, typename std::invoke_result<Left, Lhs>::type, Right>)
	{
		return this->invoke(std::invoke(left(), std::forward<Lhs>(lhs)), right());
	}

	template <typename Rhs>
	constexpr auto operator()(Rhs&& rhs) const -> decltype(auto) requires (std::invocable<Operand, Left, typename std::invoke_result<Right, Rhs>::type>)
	{
		return this->invoke(left(), std::invoke(right(), std::forward<Rhs>(rhs)));
	}

	template <typename Lhs>
	constexpr auto operator()(tag::left_t, Lhs&& lhs) const -> decltype(auto) requires (std::invocable<Operand, typename std::invoke_result<Left, Lhs>::type, Right>)
	{
		return this->invoke(std::invoke(left(), std::forward<Lhs>(lhs)), right());
	}

	template <typename Rhs>
	constexpr auto operator()(tag::right_t, Rhs&& rhs) const -> decltype(auto) requires (std::invocable<Operand, Left, typename std::invoke_result<Right, Rhs>::type>)
	{
		return this->invoke(left(), std::invoke(right(), std::forward<Rhs>(rhs)));
	}

	template <typename Lhs, typename Rhs>
	constexpr auto operator()(Lhs&& lhs, Rhs&& rhs) const -> decltype(auto) requires (std::invocable<Operand, typename std::invoke_result<Left, Lhs>::type, typename std::invoke_result<Right, Rhs>::type>)
	{
		return this->invoke(std::invoke(left(), std::forward<Lhs>(lhs)), std::invoke(right(), std::forward<Rhs>(rhs)));
	}
};

}

template <typename Left = void, typename Operand = void, typename Right = void>
class linq : detail::linq_invoke<Left, Operand, Right>
{
	using my_base = detail::linq_invoke<Left, Operand, Right>;
	
private:
	using base_l = detail::linq_left<Left>;
	using base_o = detail::linq_operand<Operand>;
	using base_r = detail::linq_right<Right>;

	template <typename O, typename R> constexpr decltype(auto) binary_predicate(O&& op, R&& r) &;
	template <typename O, typename R> constexpr decltype(auto) binary_predicate(O&& op, R&& r) &&;
	template <typename O, typename R> constexpr decltype(auto) binary_predicate(O&& op, R&& r) const&;
	template <typename O, typename R> constexpr decltype(auto) binary_predicate(O&& op, R&& r) const&&;

public:
	using my_base::my_base;
	using my_base::operator=;
	using my_base::operator();
	using base_l::has_left;
	using base_r::has_right;
	using base_o::has_operand;
	using base_l::left;
	using base_r::right;
	using left_t = Left;
	using operand_t = Operand;
	using right_t = Right;
	
	template <typename R> constexpr auto operator>(R&& r) & noexcept { return (*this).binary_predicate(monad::greater, std::forward<R>(r)); }
	template <typename R> constexpr auto operator>(R&& r) const& noexcept;
	template <typename R> constexpr auto operator==(R&& r) & noexcept { return (*this).binary_predicate(monad::equal, std::forward<R>(r)); }
	template <typename R> constexpr auto operator==(R&& r) const& noexcept { return (*this).binary_predicate(monad::equal, std::forward<R>(r)); }
};

template <typename Left>
linq(Left&& left) -> linq<Left>;

template <typename Left, typename Op>
linq(Left&& left, Op&& op) -> linq<Left, Op>;

template <typename Left, typename Op, typename Right>
linq(Left&& left, Op&& op, Right&& right) -> linq<Left, Op, Right>;

template <typename Left, typename Op, typename Right>
linq(Left&& left, Op&& op, detail::linq_right<Right> right) -> linq<Left, Op, Right>;

template <typename Left, typename Operand, typename Right>
template <typename O, typename R>
constexpr decltype(auto) linq<Left, Operand, Right>::binary_predicate(O&& op, R&& r) &
{
	return shion::linq((*this).left(), std::forward<O>(op), std::forward<R>(r));
}

template <typename Left, typename Operand, typename Right>
template <typename O, typename R>
constexpr decltype(auto) linq<Left, Operand, Right>::binary_predicate(O&& op, R&& r) &&
{
	return shion::linq(std::move(*this).left(), std::forward<O>(op), std::forward<R>(r));
}

template <typename Left, typename Operand, typename Right>
template <typename O, typename R>
constexpr decltype(auto) linq<Left, Operand, Right>::binary_predicate(O&& op, R&& r) const&
{
	return shion::linq((*this).left(), std::forward<O>(op), std::forward<R>(r));
}

template <typename Left, typename Operand, typename Right>
template <typename O, typename R>
constexpr decltype(auto) linq<Left, Operand, Right>::binary_predicate(O&& op, R&& r) const&&
{
	return shion::linq(std::move(*this).left(), std::forward<O>(op), std::forward<R>(r));
}

template <typename Left, typename Operand, typename Right>
template <typename R>
constexpr auto linq<Left, Operand, Right>::operator>(R&& r) const & noexcept { return shion::linq((*this).left(), monad::greater, std::forward<R>(r)); }

template <typename Lhs, typename Rhs>
using call = linq<

struct test_s {
	int a;
};

inline constexpr auto linq_test = linq(&test_s::a) == 25;
inline constexpr auto linq_result = linq_test(test_s{25});

inline constexpr auto test = std::invocable<const monad::greater_t&, int, int>;

}

#endif /* SHION_LINQ_H_ */
