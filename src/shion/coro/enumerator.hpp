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

namespace coro
{

SHION_EXPORT template <typename Controller, typename Reference, typename Value = void>
class basic_enumerator;

SHION_EXPORT template <typename Controller, typename Reference, typename Value = void>
class basic_enumerable;

template <typename Controller, typename Ref, typename Value>
class basic_enumerator_promise : public basic_coro_promise<
	Controller, void, void, Ref, Value
>
{
private:
	using base = basic_coro_promise<Controller, void, void, Ref, Value>;

public:
	using base::base;
	using awaitable = basic_enumerator<Controller, Ref, Value>;
	
	static constexpr auto initial_suspend() noexcept -> std::suspend_always
	{
		return {};
	}
};

template <typename Controller, typename Ref, typename Value>
class basic_enumerable_promise : public basic_enumerator_promise<
	Controller, Ref, Value
>
{
private:
	using base = basic_enumerator_promise<Controller, Ref, Value>;

public:
	using base::base;
	using awaitable = basic_enumerable<Controller, Ref, Value>;
	
	constexpr auto get_return_object() noexcept -> basic_enumerable<Controller, Ref, Value>;
	
	static constexpr auto initial_suspend() noexcept -> std::suspend_never
	{
		return {};
	}
};

}

namespace coro
{

/**
 * @brief A coroutine that is also an <a href="https://en.cppreference.com/cpp/iterator/input_iterator">input iterator</a>.
 * It starts BEFORE the first element, and needs to be incremented once before retrieving the first value.
 * 
 * @details Its main purpose is to <a href="https://en.wikipedia.org/wiki/Generator_(computer_programming)">generate values</a> as a range, similarly to C#'s <a href="https://learn.microsoft.com/en-us/dotnet/api/system.collections.generic.ienumerator-1?view=net-10.0">IEnumerator&lt;T&gt;</a>.
 */
template <typename Controller, typename Reference, typename Value>
class basic_enumerator : public basic_coroutine<Reference, basic_enumerator_promise<Controller, Reference, Value>>
{
	using base = basic_coroutine<Reference, basic_enumerator_promise<Controller, Reference, Value>>;
	
public:
	using promise_type = basic_enumerator_promise<Controller, Reference, Value>;
	using stored_type = promise_type::yield_type;
	using value_type = std::remove_cvref_t<stored_type>;
	using difference_type = std::ptrdiff_t;
	using reference = std::conditional_t<std::is_reference_v<Reference>, Reference, const Reference&>;
	using const_reference = reference;
	using rvalue_reference = Reference&&;
	using pointer = typename std::conditional<std::is_reference_v<reference>, std::add_pointer_t<reference>, void>::type;
	using iterator_category = std::forward_iterator_tag;
	using sentinel = std::default_sentinel_t;

private:
	friend basic_enumerable<Controller, Reference, Value>;
	friend promise_type;

public:
	using awaitable = basic_awaitable<reference, typename promise_type::promise_state&>;

	using base::base;
	
	constexpr basic_enumerator(sentinel) noexcept :
		base()
	{}

	constexpr bool valid() const noexcept
	{
		return this->is_valid_state();
	}

	[[nodiscard]] constexpr auto next() noexcept SHION_LIFETIMEBOUND -> std::optional<awaitable>
	{
		if (!*this || !++(*this))
			return {};
		return awaitable{ this->get_promise_state() };
	}

	constexpr auto operator++() & -> basic_enumerator&
	{
		SHION_ASSERT(valid());
		SHION_ASSERT(!this->done());
		this->get_coroutine_handle().resume();
		return *this;
	}

	[[nodiscard]] constexpr auto operator++() && -> basic_enumerator
	{
		return std::move(++(*this));
	}

	constexpr void operator++(int)
	{
		++(*this);
	}

	constexpr auto operator*() const& noexcept -> reference
	{
		SHION_ASSERT(valid());
		SHION_ASSERT(this->get_promise_state().has_carry(), "enumerator must have enumerated a value (forgot ++?)");
		return static_cast<reference>(this->get_promise_state().get_carry());
	}

	constexpr auto operator*() && noexcept -> rvalue_reference
	{
		return iter_move(*this);
	}

	constexpr auto operator->() const& noexcept -> pointer
		requires (!std::is_void_v<pointer>)
	{
		return static_cast<pointer>(&(**this));
	}

	constexpr auto operator->() && noexcept -> pointer
		requires (!std::is_void_v<pointer>)
	{
		return static_cast<pointer>(&(iter_move(*this)));
	}

	constexpr operator bool() const noexcept
	{
		return *this != sentinel{};
	}

	constexpr friend bool operator==(const basic_enumerator& e, sentinel) noexcept
	{
		return !e.valid() || e.done();
	}
	
	friend constexpr auto iter_move(const basic_enumerator& e) noexcept -> rvalue_reference
	{
		SHION_ASSERT(e.valid());
		SHION_ASSERT(e.get_promise_state().has_carry(), "enumerator must have enumerated a value (forgot ++?)");
		return static_cast<rvalue_reference>(std::move(e.get_promise_state()).get_carry());
	}
};

/**
 * @brief A coroutine that is also an <a href="https://en.cppreference.com/cpp/iterator/input_range">input range</a>.
 * It is a wrapper around basic_enumerator.
 * 
 * @details Its main purpose is to <a href="https://en.wikipedia.org/wiki/Generator_(computer_programming)">generate values</a> as a range, similarly to C#'s <a href="https://learn.microsoft.com/en-us/dotnet/api/system.collections.generic.ienumerator-1?view=net-10.0">IEnumerator&lt;T&gt;</a>.
 */
SHION_EXPORT template <typename Controller, typename Reference, typename Value>
class basic_enumerable
{
public:
	using enumerator_t   = basic_enumerator<Controller, Reference, Value>;
	using iterator_type  = enumerator_t;
	using sentinel       = iterator_type::sentinel;
	using value_type     = iterator_type::value_type;
	using reference_type = iterator_type::reference;
	using promise_type   = basic_enumerable_promise<Controller, Reference, Value>;

	friend promise_type;

	constexpr basic_enumerable() noexcept = default;
	constexpr basic_enumerable(enumerator_t&& enumerator) noexcept :
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
	constexpr basic_enumerable(detail::coro_handle<typename iterator_type::promise_type> promise) noexcept :
		_coroutine(promise)
	{
	}

	iterator_type _coroutine{};
};

template <typename Controller, typename Ref, typename Value>
constexpr auto basic_enumerable_promise<Controller, Ref, Value>::get_return_object() noexcept -> basic_enumerable<Controller, Ref, Value>
{
	using handle_t = detail::coro_handle<basic_enumerator_promise<Controller, Ref, Value>>;
	return basic_enumerable<Controller, Ref, Value>{ handle_t::from_promise(*this) };
}

}

SHION_EXPORT template <typename Reference, typename Value = void, typename Allocator = void>
using enumerator = coro::basic_enumerator<detail::coro::simple_continuation_controller, Reference, Value>;

SHION_EXPORT template <typename Reference, typename Value = void, typename Allocator = void>
using enumerable = coro::basic_enumerable<detail::coro::simple_continuation_controller, Reference, Value>;

// Compile time checks
#if SHION_BUILDING_MODULES
// Testing that it is indeed an iterator
static_assert(std::same_as<std::iter_value_t<enumerator<int>>, int>);
static_assert(std::same_as<std::iter_reference_t<enumerator<int>>, const int&>);
static_assert(std::same_as<std::iter_reference_t<const enumerator<int>>, const int&>);
static_assert(std::same_as<decltype(iter_move(std::declval<enumerator<int>&>())), int&&>);
static_assert(std::same_as<std::iter_rvalue_reference_t<enumerator<int>>, int&&>);
static_assert(std::weakly_incrementable<enumerator<int>>);
static_assert(std::indirectly_readable<enumerator<int>>);
static_assert(std::input_iterator<enumerator<int>>);

// Testing the accessor
static_assert(std::same_as<enumerator<int>::awaitable::reference, const int&>);
static_assert(std::same_as<enumerator<int&>::awaitable::reference, int&>);

// Testing references work properly
static_assert(std::same_as<std::iter_value_t<enumerator<int&&>>, int>);
static_assert(std::same_as<std::iter_reference_t<enumerator<int&&>>, int&&>);
static_assert(std::same_as<std::iter_reference_t<const enumerator<int&&>>, int&&>);
static_assert(std::same_as<std::iter_rvalue_reference_t<enumerator<int&&>>, int&&>);
static_assert(std::indirectly_readable<enumerator<int&>>);
static_assert(std::input_iterator<enumerator<int&>>);

// Enumerable
static_assert(std::sentinel_for<enumerator<int>::sentinel, enumerator<int>>);
static_assert(std::ranges::range<enumerable<int>>);
static_assert(std::ranges::input_range<enumerable<int>>);
#endif

}

#endif
