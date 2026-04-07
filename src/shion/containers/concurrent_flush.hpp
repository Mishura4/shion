//
// Created by miuna on 3/30/2026.
//

#ifndef SHION_CONCURRENT_FLUSH_HPP_
#define SHION_CONCURRENT_FLUSH_HPP_

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
#include <shion/coro/enumerator.hpp>

#endif

namespace SHION_NAMESPACE
{

SHION_EXPORT template <typename T, typename Allocator = std::allocator<T>>
class concurrent_flush
{
public:
	struct node
	{
		template <typename... Args>
		constexpr node(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) :
			value(std::forward<Args>(args)...) {}

		SHION_NO_UNIQUE_ADDRESS T value;
		node*                     next = nullptr;
	};

	using allocator = std::allocator_traits<Allocator>::template rebind_alloc<node>;
	using alloctraits = std::allocator_traits<allocator>;

	struct destroyer
	{
		constexpr destroyer() = default;
		constexpr destroyer(const destroyer&) = delete;
		constexpr destroyer(destroyer&&) = delete;
		constexpr destroyer(allocator& alloc, node* init) :
			allocator{ &alloc },
			current{ init }
		{
		}

		constexpr auto operator=(const destroyer&) -> destroyer& = delete;
		constexpr auto operator=(destroyer&&) -> destroyer& = delete;

		SHION_NO_UNIQUE_ADDRESS allocator* allocator;
		node*                              current;

		constexpr void operator()() noexcept
		{
			while (current)
			{
				advance();
			}
		}

		constexpr void advance() noexcept
		{
			node* next = current->next;
			alloctraits::destroy(*allocator, current);
			alloctraits::deallocate(*allocator, current, 1);
			current = next;
		}

		constexpr ~destroyer()
		{
			(*this)();
		}
	};

	struct iterator
	{
		using value_type     = std::remove_reference_t<T>;
		using reference_type = value_type&;
		using difference_type = void;

		node* current        = nullptr;

		constexpr auto operator*() const noexcept -> T&
		{
			return current->value;
		}

		constexpr auto operator->() const noexcept -> T*
		{
			return current->value;
		}

		constexpr auto operator++(int) const noexcept -> iterator
		{
			return iterator{ current->next };
		}

		constexpr auto operator++() const noexcept -> iterator&
		{
			current = current->next;
			return *this;
		}

		constexpr auto operator==(std::default_sentinel_t) const noexcept -> bool
		{
			return current == nullptr;
		}
	};

	static constexpr void _invert(node*& head)
	{
		node* next    = nullptr;
		node* current = nullptr;

		while (head != nullptr)
		{
			next   = std::exchange(head->next, current);
			current = head;
			head    = next;
		}
		head = current;
	}

public:
	constexpr concurrent_flush() noexcept(noexcept(Allocator())) = default;
	constexpr concurrent_flush(const concurrent_flush& other) = delete;
	constexpr concurrent_flush(concurrent_flush&& other) noexcept :
		_allocator(std::move(other._allocator)),
		_head(other._head.exchange(other._head.load(std::memory_order_relaxed)))
	{
	}

	constexpr ~concurrent_flush()
	{
		node* next = _head.load(std::memory_order_relaxed);
		destroyer{ _allocator, next }();
	}

	constexpr auto operator=(const concurrent_flush& other) -> concurrent_flush& = delete;
	constexpr auto operator=(concurrent_flush&& other) noexcept -> concurrent_flush&
	{
		if (this == &other)
			return *this;

		auto prev = _head.exchange(nullptr);
		if constexpr (alloctraits::propagate_on_container_move_assignment::value)
		{
			_allocator = std::move(other._allocator);
			this->_push(other._head.exchange(nullptr));
		}
		else
		{
			if (_allocator == other._allocator)
			{
				this->_push(other._head.exchange(nullptr));
			}
			// else, we cannot move the chain, but we also don't have to destroy it since it's still owned by the other allocator
		}
		return *this;
	}

	constexpr auto push(T&& value) noexcept -> T&
		requires(std::move_constructible<T>)
	{
		node* node = this->_new(value);
		this->_push(node);
		return node->value;
	}

	constexpr auto push(const T& value) noexcept -> T&
		requires(std::copy_constructible<T>)
	{
		node* node = this->_new(value);
		this->_push(node);
		return node->value;
	}

	template <std::ranges::range Range>
		requires (std::convertible_to<std::ranges::range_reference_t<Range>, T>)
	constexpr auto push_range(Range&& range) noexcept -> void
	{
		return this->_push_range(std::forward<Range>(range));
	}

	template <typename... Args>
	constexpr auto emplace(Args&&... args) noexcept -> T&
		requires (std::constructible_from<T, Args...>)
	{
		node* node = this->_new(std::forward<Args>(args)...);
		this->_push(node);
		return node->value;
	}

	constexpr auto get_allocator() & noexcept -> Allocator&
	{
		return _allocator;
	}

	constexpr auto get_allocator() && noexcept -> Allocator&&
	{
		return std::move(_allocator);
	}

	constexpr auto get_allocator() const& noexcept -> const Allocator&
	{
		return _allocator;
	}

	template <std::invocable<T&&> F>
	constexpr void flush(F&& fun) noexcept(std::is_nothrow_invocable_v<F, T&&>)
	{
		destroyer cleanup{ _allocator, _head.exchange(nullptr, std::memory_order_seq_cst) };
		node*&    current = cleanup.current;
		concurrent_flush::_invert(current);

		while (current != nullptr)
		{
			std::invoke(fun, std::move(current->value));
			cleanup.advance();
		}
	}

	auto pop() -> enumerable<T&&>
	{
		destroyer cleanup{ _allocator, _head.exchange(nullptr, std::memory_order_seq_cst) };
		node*&    current = cleanup.current;
		concurrent_flush::_invert(current);

		while (current != nullptr)
		{
			co_yield std::move(current->value);
			cleanup.advance();
		}
	}

	constexpr void clear() noexcept
	{
		flush(noop);
	}

private:
	template <typename Range>
	constexpr auto _push_range(Range&& range) noexcept -> void
	{
		node* prev = nullptr;
		for (auto&& elem : range)
		{
			node* ptr = _emplace(std::forward<decltype(elem)>(elem));
			ptr->next = prev;
		}

		if (prev != nullptr)
		{
			_push(prev);
		}
	}

	template <typename... Args>
	constexpr auto _new(Args&&... args) noexcept -> node*
	{
		node* ptr = alloctraits::allocate(_allocator, 1);
		alloctraits::construct(_allocator, ptr, std::forward<Args>(args)...);
		return ptr;
	}

	constexpr void _push(node* node) noexcept
	{
		while (!_head.compare_exchange_weak(node->next, node, std::memory_order_release, std::memory_order_relaxed))
		{
		}
	}

	SHION_NO_UNIQUE_ADDRESS allocator _allocator;
	std::atomic<node*>                _head;
};

}

#endif //SHION_CONCURRENT_FLUSH_HPP_
