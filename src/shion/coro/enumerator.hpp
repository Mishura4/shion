//
// Created by miuna on 3/31/2026.
//

#ifndef SHION_CORO_ENUMERATOR_HPP_
#define SHION_CORO_ENUMERATOR_HPP_

#include <shion/common/defines.hpp>

#if !SHION_BUILDING_MODULES
#include <type_traits>
#include <bit>
#include <list>

#include <shion/meta/macros.hpp>
#include <shion/common.hpp>
#include <shion/common/detail.hpp>
#include <shion/meta/type_traits.hpp>
#include <shion/utility/optional.hpp>
#include <shion/coro/promise.hpp>
#endif

namespace SHION_NAMESPACE
{
inline namespace coro
{

SHION_EXPORT template <typename Reference, typename Value = void, typename Allocator = void>
class enumerator;

SHION_EXPORT template <typename Reference, typename Value, typename Allocator>
class enumerable;

}

namespace detail::coro
{
template <typename Reference, typename Value, typename Allocator>
class enumerator_awaitable;

template <typename Reference, typename Value, typename Allocator>
class enumerator_promise;

template <typename Reference, typename Value, typename Allocator>
using enumerator_handle = std_coroutine::coroutine_handle<enumerator_promise<Reference, Value, Allocator>>;

template <typename Reference, typename Value, typename Allocator>
class enumerator_promise_base : public promise_state<
	promise_storage<void, void, Reference, Value>,
	simple_coro_handler
>
{
public:
	using storage = promise_storage<void, void, Reference, Value>;
	using typename storage::yield_expr_t;
	using typename storage::yield_storage;
	using yield_copy_expr_t = typename std::remove_cvref<yield_expr_t>::type const&;
	using yield_move_expr_t = typename std::remove_cvref<yield_expr_t>::type&&;
	using reference = storage::yield_reference_t;
	using value_type = storage::yield_value_t;
	using simple_coro_handler::suspend_and_continue;

	using handle_type = std_coroutine::coroutine_handle<enumerator_promise<Reference, Value, Allocator>>;
	using handle_ptr = handle_type;
	using coroutine_type = enumerator<Reference, Value, Allocator>;
	friend coroutine_type;

	void unhandled_exception()
	{
		std::rethrow_exception(std::current_exception());
	}

	static constexpr auto initial_suspend() noexcept -> std::suspend_never
	{
		return {};
	}

	static constexpr auto final_suspend() noexcept -> std::suspend_always
	{
		return {};
	}

	static constexpr void return_void() noexcept {}
};

template <typename Reference, typename Value, typename Allocator>
class enumerator_promise_yield : public enumerator_promise_base<Reference, Value, Allocator>
{
private:
	using promise_base = enumerator_promise_base<Reference, Value, Allocator>;

public:
	using typename promise_base::suspend_and_continue;
	using typename promise_base::yield_copy_expr_t;
	using typename promise_base::yield_move_expr_t;
	using typename promise_base::yield_storage;

	constexpr auto yield_value(yield_copy_expr_t yield_expr)
		noexcept(std::is_nothrow_constructible_v<yield_storage, yield_copy_expr_t>)
		-> suspend_and_continue
	{
		this->set_yield(static_cast<yield_copy_expr_t>(yield_expr));
		return { *this };
	}

	constexpr auto yield_value(yield_move_expr_t yield_expr)
		noexcept(std::is_nothrow_constructible_v<yield_storage, yield_move_expr_t>)
		-> suspend_and_continue
	{
		this->set_yield(static_cast<yield_move_expr_t>(yield_expr));
		return { *this };
	}

	template <typename Expr>
	constexpr auto yield_value(Expr&& value)
		noexcept(std::is_nothrow_constructible_v<yield_storage, Expr>)
		-> suspend_and_continue
	requires (std::convertible_to<Expr, Reference>)
	{
		this->set_yield(std::forward<Expr>(value));
		return { *this };
	}
};

template <typename Reference, typename Value, typename Allocator>
requires (std::is_reference_v<Reference>)
class enumerator_promise_yield<Reference, Value, Allocator> : public enumerator_promise_base<Reference, Value, Allocator>
{
private:
	using promise_base = enumerator_promise_base<Reference, Value, Allocator>;

public:
	using typename promise_base::suspend_and_continue;
	using typename promise_base::yield_copy_expr_t;
	using typename promise_base::yield_move_expr_t;
	using typename promise_base::yield_storage;

	template <std::convertible_to<Reference> Expr>
	constexpr auto yield_value(Expr&& value)
		noexcept(std::is_nothrow_constructible_v<yield_storage, Expr>)
		-> suspend_and_continue
	{
		this->set_yield(std::forward<Expr>(value));
		return { *this };
	}
};

template <typename Reference, typename Value, typename Allocator>
class enumerator_promise : public enumerator_promise_yield<Reference, Value, Allocator>
{
public:
	using handle_type = coro_handle<enumerator_promise>;

	constexpr auto get_return_object() noexcept -> enumerator<Reference, Value, Allocator>;
};


/**
 * @brief Workaround clang bug. I was trying to let the compiler implicitly convert enumerator to enumerable
 * through enumerator_promise::get_return_object, but something weird was happening with a half-initialized
 * enumerator being destroyed...
 */
template <typename Reference, typename Value, typename Allocator>
class enumerable_promise : public enumerator_promise<Reference, Value, Allocator>
{
public:
	constexpr auto get_return_object() noexcept -> enumerable<Reference, Value, Allocator>;
};

template <typename Reference, typename Value, typename Allocator>
class enumerator_awaitable : public detail::coro::awaitable<
	enumerator_handle<Reference, Value, Allocator>
>
{
public:
	class accessor;
	using enumerator = enumerator<Reference, Value, Allocator>;

	constexpr enumerator_awaitable(SHION_LIFETIMEBOUND enumerator& e) noexcept :
		_enumerator(&e)
	{
	}

	class accessor
	{
	public:
		using reference = enumerator_promise<Reference, Value, Allocator>::reference;
		using pointer = std::conditional_t<std::is_void_v<reference>, void, std::add_pointer<reference>>;
		using value_type = enumerator_promise<Reference, Value, Allocator>::value_type;

		constexpr explicit operator bool() const noexcept
		{
			return has_value();
		}

		constexpr bool has_value() const noexcept
		{
			return *_awaitable->_enumerator != typename enumerator::sentinel{};
		}

		constexpr auto operator*() const noexcept -> reference
		{
			SHION_ASSERT(has_value());
			return **_awaitable->_enumerator;
		}

		constexpr auto operator->() const noexcept -> pointer
			requires (!std::is_void_v<pointer>)
		{
			SHION_ASSERT(has_value());
			return _awaitable->_enumerator->operator->();
		}

		constexpr auto get() const noexcept(std::is_nothrow_constructible_v<value_type>) -> std::optional<value_type>
		{
			if (*_awaitable->_enumerator == typename enumerator::sentinel{})
				return std::nullopt;

			return std::optional<value_type>{ std::in_place, **this };
		}

	private:
		friend class enumerator_awaitable;

		constexpr accessor(enumerator_awaitable& awaitable) noexcept :
			_awaitable{ &awaitable }
		{
		}

		enumerator_awaitable* _awaitable{};
	};

	using handle_type = enumerator_handle<Reference, Value, Allocator>;
	using reference = accessor;
	using pointer = accessor::pointer;
	using value_type = accessor::value_type;

	constexpr bool await_ready() const noexcept
	{
		return _enumerator->done();
	}

	constexpr auto await_resume() -> accessor
	{
		return accessor{ *this };
	}

private:
	enumerator* _enumerator{};
};


}

inline namespace coro
{

/**
 * @brief Iterator coroutine.
 */
template <typename Reference, typename Value, typename Allocator>
class enumerator
{
public:
	using promise_type = detail::coro::enumerator_promise<Reference, Value, Allocator>;
	using difference_type = std::ptrdiff_t;
	using value_type = promise_type::yield_value_t;
	using reference = typename std::conditional<std::is_reference_v<Reference>, Reference, Reference&>::type;
	using pointer = typename std::conditional<std::is_reference_v<Reference>, std::add_pointer_t<Reference>, void>::type;
	using iterator_category = std::forward_iterator_tag;
	using sentinel = std::default_sentinel_t;
	using awaitable = detail::coro::enumerator_awaitable<Reference, Value, Allocator>;

	friend awaitable;

private:
	using handle_type = promise_type::handle_type;

	friend enumerable<Reference, Value, Allocator>;
	friend detail::coro::enumerator_promise<Reference, Value, Allocator>;

	constexpr enumerator(handle_type handle) noexcept :
		_handle{ std::move(handle) }
	{
	}

	handle_type _handle = nullptr;

public:
	constexpr enumerator() noexcept = default;
	constexpr enumerator(enumerator&& other) noexcept :
		_handle(std::exchange(other._handle, nullptr))
	{}

	constexpr enumerator(sentinel) noexcept :
		_handle(nullptr)
	{}

	constexpr ~enumerator() noexcept
	{
		if (_handle)
			_handle.destroy();
	}

	constexpr auto operator=(enumerator&& other) noexcept -> enumerator&
	{
		if (_handle)
			_handle.destroy();
		_handle = std::exchange(other._handle, nullptr);
		return *this;
	}

	constexpr auto operator=(std::convertible_to<sentinel> auto) noexcept -> enumerator&
	{
		if (auto prev = std::exchange(_handle, nullptr))
			prev.destroy();
		return *this;
	}

	friend constexpr void swap(enumerator& a, enumerator& b) noexcept
	{
		using std::swap;
		swap(a._handle, b._handle);
	}

	constexpr bool valid() const noexcept
	{
		return _handle != nullptr;
	}

	constexpr bool done() const noexcept
	{
		SHION_ASSERT(valid());
		return _handle.done();
	}

	[[nodiscard]] constexpr auto next() noexcept -> awaitable
	{
		return { *this };
	}

	constexpr auto operator++() -> enumerator&
	{
		SHION_ASSERT(valid());
		SHION_ASSERT(!done());
		_handle.resume();
		return *this;
	}

	constexpr void operator++(int)
	{
		++(*this);
	}

	constexpr auto operator*() const noexcept -> reference
	{
		SHION_ASSERT(valid());
		return static_cast<reference>(_handle.promise().get_yield());
	}

	constexpr auto operator->() const noexcept -> pointer
		requires (!std::is_void_v<pointer>)
	{
		return static_cast<pointer>(&(**this));
	}

	constexpr operator bool() const noexcept
	{
		return *this != sentinel{};
	}

	constexpr friend bool operator==(const enumerator& e, sentinel) noexcept
	{
		return !e.valid() || e.done();
	}
};

SHION_EXPORT template <typename Reference, typename Value = void, typename Allocator = void>
class enumerable
{
public:
	using enumerator     = enumerator<Reference, Value, Allocator>;
	using iterator_type  = enumerator;
	using sentinel       = iterator_type::sentinel;
	using value_type     = iterator_type::value_type;
	using reference_type = iterator_type::reference;
	using promise_type   = detail::coro::enumerable_promise<Reference, Value, Allocator>;
	using handle_type    = promise_type::handle_type;

	friend promise_type;

	constexpr enumerable() noexcept = default;
	constexpr enumerable(enumerator&& enumerator) noexcept :
		_coroutine(std::move(enumerator))
	{
	}

	constexpr auto begin() noexcept -> iterator_type
	{
		return std::exchange(_coroutine, end());
	}

	static constexpr auto end() noexcept -> sentinel
	{
		return sentinel{};
	}

private:
	constexpr enumerable(handle_type promise) noexcept :
		_coroutine(promise)
	{
	}

	iterator_type _coroutine{};
};

}

template <typename Reference, typename Value, typename Allocator>
constexpr auto detail::coro::enumerator_promise<Reference, Value, Allocator>::get_return_object() noexcept
	-> enumerator<Reference, Value, Allocator>
{
	return enumerator<Reference, Value, Allocator>{ handle_type::from_promise(*this) };
}

template <typename Reference, typename Value, typename Allocator>
constexpr auto detail::coro::enumerable_promise<Reference, Value, Allocator>::get_return_object() noexcept
	-> enumerable<Reference, Value, Allocator>
{
	return enumerable<Reference, Value, Allocator>{ enumerator_promise<Reference, Value, Allocator>::handle_type::from_promise(*this) };
}

// Compile time checks
#if SHION_BUILDING_MODULES
// Testing that it is indeed an iterator
static_assert(std::same_as<std::iter_value_t<enumerator<int>>, int>);
static_assert(std::same_as<std::iter_reference_t<enumerator<int>>, int&>);
static_assert(std::same_as<std::iter_reference_t<const enumerator<int>>, int&>);
static_assert(std::same_as<std::iter_rvalue_reference_t<enumerator<int>>, int&&>);
static_assert(std::weakly_incrementable<enumerator<int>>);
static_assert(std::indirectly_readable<enumerator<int>>);
static_assert(std::input_iterator<enumerator<int>>);

// Testing references work properly
static_assert(std::same_as<std::iter_value_t<enumerator<int&&>>, int>);
static_assert(std::same_as<std::iter_reference_t<enumerator<int&&>>, int&&>);
static_assert(std::same_as<std::iter_reference_t<const enumerator<int&&>>, int&&>);
static_assert(std::same_as<std::iter_rvalue_reference_t<enumerator<int&&>>, int&&>);
static_assert(std::indirectly_readable<enumerator<int&>>);
static_assert(std::input_iterator<enumerator<int&>>);

// Testing the accessor
static_assert(std::same_as<enumerator<int>::awaitable::reference, enumerator<int>::awaitable::accessor>);
static_assert(std::same_as<enumerator<int&>::awaitable::reference, enumerator<int&>::awaitable::accessor>);
static_assert(std::same_as<
	decltype(*std::declval<enumerator<int>::awaitable::accessor>()),
	std::iter_reference_t<enumerator<int>>
	>);
static_assert(std::same_as<
	decltype(*std::declval<enumerator<int&&>::awaitable::accessor>()),
	std::iter_reference_t<enumerator<int&&>>
>);

// Enumerable
static_assert(std::sentinel_for<enumerator<int>::sentinel, enumerator<int>>);
static_assert(std::ranges::range<enumerable<int>>);
static_assert(std::ranges::input_range<enumerable<int>>);
#endif

}

#endif
