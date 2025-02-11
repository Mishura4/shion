#ifndef SHION_SUPPLIER_H_
#define SHION_SUPPLIER_H_

#ifndef SHION_USE_MODULES

#include <concepts>
#include <functional>

#else

import std;

#endif

#ifndef SHION_BUILDING_MODULES

#include <shion/shion_essentials.hpp>

#endif

#include <shion/decl.hpp>

namespace shion
{

namespace detail {

class supplier_base
{
public:
	constexpr supplier_base() = default;
	constexpr supplier_base(const supplier_base&) = default;
	constexpr supplier_base(supplier_base&&) = default;
	constexpr ~supplier_base() = default;

	constexpr supplier_base &operator=(const supplier_base&) = default;
	constexpr supplier_base &operator=(supplier_base&&) = default;
};

template <typename T>
class supplier_type : public supplier_base
{
private:
	T _value;

public:
	using supplier_base::supplier_base;
	using supplier_base::operator=;

	template <typename... Args>
	requires(std::constructible_from<T, Args...>)
	constexpr supplier_type(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) : _value(std::forward<Args>(args)...) {}
	
	constexpr auto get() & noexcept -> T& { return _value; }
	constexpr auto get() && noexcept -> T&& { return std::move(_value); }
	constexpr auto get() const& noexcept -> T const& { return _value; }
	constexpr auto get() const&& noexcept -> T const&& { return std::move(_value); }

	constexpr auto operator()() & -> decltype(auto)
	{
		using t = std::add_lvalue_reference_t<T>;
		if constexpr (std::invocable<t>)
		{
			return std::invoke(static_cast<t>(_value));
		}
		else
		{
			return static_cast<t>(_value);
		}
	}

	constexpr auto operator()() && -> decltype(auto)
	{
		using t = std::add_rvalue_reference_t<T>;
		if constexpr (std::invocable<t>)
		{
			return std::invoke(static_cast<t>(_value));
		}
		else
		{
			return static_cast<t>(_value);
		}
	}

	constexpr auto operator()() const& -> decltype(auto)
	{
		using t = std::add_lvalue_reference_t<std::add_const_t<T>>;
		if constexpr (std::invocable<t>)
		{
			return std::invoke(static_cast<t>(_value));
		}
		else
		{
			return static_cast<t>(_value);
		}
	}

	constexpr auto operator()() const&& -> decltype(auto)
	{
		using t = std::add_lvalue_reference_t<std::add_const_t<T>>;
		if constexpr (std::invocable<t>)
		{
			return std::invoke(static_cast<t>(_value));
		}
		else
		{
			return static_cast<t>(_value);
		}
	}
};

template <>
class supplier_type<void> : public supplier_base
{
public:
	using supplier_base::supplier_base;
	using supplier_base::operator=;

	constexpr void get() const noexcept {};
	constexpr void operator()() const noexcept {}
};

template <auto V>
class supplier_constant : supplier_base
{
public:
	using supplier_base::supplier_base;
	using supplier_base::operator=;
	
	constexpr auto get() const noexcept -> decltype(auto) { return V; }
	constexpr auto operator()() const -> decltype(auto)
	{
		if constexpr (std::invocable<decltype(V)>)
		{
			return std::invoke(V);
		}
		else
		{
			return V;
		}
	}
};

template <typename T>
class basic_supplier : supplier_type<T>
{
public:
	using supplier_type<T>::supplier_type;
	using supplier_type<T>::operator=;
	using supplier_type<T>::get;
	using supplier_type<T>::operator();
};

template <auto V>
class basic_supplier<constant<V>> : supplier_constant<V>
{
public:
	using supplier_constant<V>::supplier_constant;
	using supplier_constant<V>::operator=;
	using supplier_constant<V>::get;
	using supplier_constant<V>::operator();
};

}

SHION_DECL template <typename T>
class supplier : detail::basic_supplier<T>
{
	using base = detail::basic_supplier<T>;
	template <typename U>
	using type = std::remove_cvref_t<decltype(std::declval<U>().get())>;

public:
	using base::base;
	using base::operator=;
	using base::get;
	using base::operator();
	using value_type = type<base>;
	using const_value_type = std::add_const_t<value_type>;
	using pointer_type = std::add_pointer_t<value_type>;
	using const_pointer_type = std::add_pointer_t<const_value_type>;
	using reference_type = std::add_lvalue_reference_t<value_type>;
	using const_reference_type = std::add_lvalue_reference_t<const_value_type>;
	using rvalue_reference_type = std::add_rvalue_reference_t<value_type>;
	using const_rvalue_reference_type = std::add_rvalue_reference_t<const_value_type>;

	template <typename U, typename... Args>
	constexpr decltype(auto) operator()(U&& arg1, Args&&... args) & requires (std::invocable<type<supplier&>, U, Args...>)
	{
		return std::invoke(get(), std::forward<U>(arg1), std::forward<Args>(args)...);
	}

	template <typename U, typename... Args>
	constexpr decltype(auto) operator()(U&& arg1, Args&&... args) const& requires (std::invocable<type<supplier const&>, U, Args...>)
	{
		return std::invoke(get(), std::forward<U>(arg1), std::forward<Args>(args)...);
	}

	template <typename U, typename... Args>
	constexpr decltype(auto) operator()(U&& arg1, Args&&... args) && requires (std::invocable<type<supplier&&>, U, Args...>)
	{
		return std::invoke(std::move(*this).get(), std::forward<U>(arg1), std::forward<Args>(args)...);
	}

	template <typename U, typename... Args>
	constexpr decltype(auto) operator()(U&& arg1, Args&&... args) const&& requires (std::invocable<type<supplier const&&>, U, Args...>)
	{
		return std::invoke(std::move(*this).get(), std::forward<U>(arg1), std::forward<Args>(args)...);
	}
};

template <typename T>
supplier(T t) -> supplier<T>;

}

#endif /* SHION_SUPPLIER_H_ */
