#ifndef SHION_CORO_EVENT_HOOK_H_
#define SHION_CORO_EVENT_HOOK_H_

#include <shion/common/defines.hpp>

#if !SHION_BUILDING_MODULES
#	include <shion/meta/macros.hpp>
#	include <shion/coro/coro.hpp>
#	include <shion/coro/awaitable.hpp>

#	include <variant>
#	include <type_traits>
#	include <coroutine>
#	include <atomic>
#	include <memory>
#endif
/*
namespace SHION_NAMESPACE
{

template <typename Allocator>
class event_hook_base;

namespace detail::event_hook
{

template <typename Event>
class simple_awaitable
{
public:
	using value_type = std::remove_cv_t<std::remove_reference_t<Event>>;
	using reference_type = std::conditional_t<std::is_reference_v<Event>, Event, const Event&>;
	using resumed_type = reference_type;

	constexpr bool await_ready() const noexcept { return false; }

	constexpr void await_suspend(std::coroutine_handle<> awaiter)
	{
		_coro = awaiter;
	}

	constexpr auto await_resume() const -> resumed_type
	{
		if (auto* ex_ptr = std::get_if<2>(&_event); ex_ptr != nullptr)
			std::rethrow_exception(*ex_ptr);
		return *std::get_if<1>(&_event);
	}

protected:
	void resume_event(reference_type event)
	{
		_event.template emplace<1>(std::addressof(event));
		_coro.resume();
	}

	void resume_exception(std::exception_ptr exception)
	{
		_event.template emplace<2>(exception);
		_coro.resume();
	}

private:
	using variant = std::variant<std::monostate, std::add_pointer_t<reference_type>, std::exception_ptr>;
	variant _event;
	std::coroutine_handle<> _coro{};
};

template <>
class simple_awaitable<void>
{
public:
	using value_type = void;
	using reference_type = void;
	using resumed_type = reference_type;

	constexpr bool await_ready() const noexcept { return false; }

	constexpr void await_suspend(std::coroutine_handle<> awaiter)
	{
		_coro = awaiter;
	}

	constexpr auto await_resume() const -> resumed_type
	{
		if (auto* ex_ptr = std::get_if<1>(&_event); ex_ptr != nullptr)
			std::rethrow_exception(*ex_ptr);
	}

protected:
	void resume_event()
	{
		_coro.resume();
	}

	void resume_exception(std::exception_ptr exception)
	{
		_event.template emplace<1>(exception);
		_coro.resume();
	}

private:
	using variant = std::variant<std::monostate, std::exception_ptr>;
	variant _event;
	std::coroutine_handle<> _coro{};
};

template <typename Event>
struct async_promise_holder
{
	std::shared_ptr<shion::async_single_promise<Event>> _promise;
};

template <typename Event>
class async_awaitable : private async_promise_holder<std::add_pointer_t<Event>>, SHION_NAMESPACE:: async_awaitable<std::add_pointer_t<Event>>
{
	using base = SHION_NAMESPACE:: async_awaitable<std::add_pointer_t<Event>>;

public:
	using typename base::value_type;
	using typename base::reference_type;

	async_awaitable(std::shared_ptr<shion::async_single_promise<Event>> promise) :
		async_promise_holder<Event>{ std::move(promise) },
		base(this->_promise.get())
	{}

	using base::operator=; // use awaitable's assignment operator
	using base::await_ready; // expose await_ready as public

	constexpr auto await_resume() -> reference_type
	{
		return static_cast<reference_type>(*base::await_resume());
	}
};

template <>
class async_awaitable<void> : private async_promise_holder<void>, SHION_NAMESPACE:: async_awaitable<void>
{
	using base = SHION_NAMESPACE:: async_awaitable<void>;
public:
	using base::value_type;
	using base::reference_type;

	async_awaitable(std::shared_ptr<shion::promise<void>> promise) :
		async_promise_holder<void>{ std::move(promise) },
		base(this->_promise.get())
	{}

	using base::operator=; // use awaitable's assignment operator
	using base::await_ready; // expose await_ready as public
};

struct event_hook_node_base
{
	event_hook_node_base() = default;

	event_hook_node_base* next{};

	constexpr event_hook_node_base* invert()
	{
		event_hook_node_base* head    = this;
		event_hook_node_base* next    = nullptr;
		event_hook_node_base* current = nullptr;

		while (head != nullptr)
		{
			next    = std::exchange(head->next, current);
			current = head;
			head    = next;
		}
		return current;
	}

	template <typename T, typename Fun>
	constexpr auto traverse(this T* self, const Fun& fun)
	{
		auto* current = static_cast<T*>(this)->event_hook_node_base::next;
		while (current != nullptr)
		{
			auto next = static_cast<T*>(current->event_hook_node_base::next);
			fun(current);
			current = next;
		}
	}
protected:
	constexpr ~event_hook_node_base() = default;
};

template <typename Event>
struct async_event_hook_node : event_hook_node_base
{
	using promise_type = basic_promise<std::add_pointer_t<Event>>;
	using promise_ptr = std::shared_ptr<promise_type>;

	template <typename Allocator>
	async_event_hook_node(const Allocator& alloc = {}) :
		promise{std::allocate_shared<promise_type>(alloc)},
		awaitable{promise.get()}
	{}

	promise_ptr            promise{};
	async_awaitable<Event> awaitable{};
};

template <typename Event>
struct basic_event_hook_node : event_hook_node_base
{
	using promise_type = basic_promise<std::add_pointer_t<Event>>;
	using promise_ptr = std::shared_ptr<promise_type>;

	template <typename Allocator>
	basic_event_hook_node(const Allocator& alloc = {}) :
		promise{std::allocate_shared<promise_type>(alloc)},
		awaitable{promise.get()}
	{}

	promise_ptr            promise{};
	async_awaitable<Event> awaitable{};
};

template <typename Node, typename T = void>
struct event_hook_allocator
{
	using type = typename std::allocator_traits<T>::template rebind_alloc<Node>;
};

template <typename Node>
struct event_hook_allocator<Node, void>
{
	using type = std::allocator<Node>;
};

template <typename Node, typename Allocator>
class event_hook_base
{
protected:
	using alloctraits = std::allocator_traits<Allocator>;
	
	template <typename Event, typename ExceptionHandler>
	void _resume_chain(Event&& event, ExceptionHandler&& handler)
	{
		auto* head = _head.exchange(nullptr, std::memory_order_acquire);

		auto current = head->invert();
		while (current != nullptr)
		{
			try
			{
				current->resume(std::forward<Event>(event));
			}
			catch (...)
			{
				auto exception = std::current_exception();
				std::invoke(std::forward<ExceptionHandler>(handler), exception);
			}

			auto next = current->next;
			alloctraits::destroy(_allocator, current);
			alloctraits::deallocate(_allocator, current, 1);
			current = next;
		}
	}
	
	void _rethrow_any() const
	{
		if (this->_exception)
		{
			std::rethrow_exception(this->_exception);
		}
	}

public:
	constexpr event_hook_base() noexcept(std::is_nothrow_default_constructible_v<Allocator>)
		requires(std::is_default_constructible_v<Allocator>)
	{
	}

	constexpr event_hook_base(Allocator allocator) noexcept(std::is_nothrow_move_constructible_v<Allocator>) : _allocator(std::move(allocator))
	{
	}

	constexpr event_hook_base(const event_hook_base& other) = delete;

	constexpr event_hook_base(event_hook_base&& other) noexcept :
		_allocator(std::move(other._allocator)),
		_head(other._head.exchange(nullptr))
	{
	}
	
	constexpr ~event_hook_base()
	{
		_exception = std::make_exception_ptr(std::runtime_error{ "event_hook was destroyed while there were still suspended coroutines" });
		_resume_chain([](std::exception_ptr& ex) {
			#ifndef NDEBUG
				try
				{
					std::rethrow_exception(ex);
				}
				catch (const std::exception& e)
				{
					std::cerr << "[FATAL] Exception in event_hook destructor: " << e.what() << std::endl;
				}
			#endif
			std::terminate();
		});
	}
	
	constexpr event_hook_base& operator=(const event_hook_base& other) = delete;
	constexpr event_hook_base& operator=(event_hook_base&& other) noexcept
	{
		if (this != &other)
		{
			auto prev = _head.exchange(nullptr);
			_resume_chain([](std::exception_ptr& ex) {
				#ifndef NDEBUG
				try
				{
					std::rethrow_exception(ex);
				}
				catch (const std::exception& e)
				{
					std::cerr << "[FATAL] Exception in event_hook move assignment cleanup: " << e.what() << std::endl;
				}
				#endif
				std::terminate();
			});
		}
		
		return *this;
	}
	
	constexpr friend void swap(event_hook_base& lhs, event_hook_base& rhs) = delete;

	constexpr bool await_ready() const noexcept { return false; }

	constexpr void await_suspend(std::coroutine_handle<> awaiting_coro)
	{
		auto* ptr = alloctraits::allocate(_allocator, 1);
		alloctraits::construct(_allocator, ptr, awaiting_coro, nullptr);
		_push(ptr);
	}

private:
	SHION_NO_UNIQUE_ADDRESS Allocator _allocator;
	std::atomic<Node*>     _head{ nullptr };
	std::exception_ptr                _exception{};
};

}

SHION_EXPORT template <
	typename Event = void,
	typename Allocator = void
>
class event_hook : public detail::event_hook::event_hook_base<
	detail::event_hook::basic_event_hook_node<Event>,
	typename detail::event_hook::event_hook_allocator<Allocator>::type
>
{
	using base = detail::event_hook::event_hook_base<
		detail::event_hook::basic_event_hook_node<Event>,
		typename detail::event_hook::event_hook_allocator<Allocator>::type
	>;

	struct scope_cleanup
	{
		event_hook* me;
			
		~scope_cleanup()
		{
			me->_current_event = nullptr;
		}
	};

public:
	using base::base;
	using base::operator=;
	
	using reference_type = std::conditional_t<std::is_reference_v<Event>, Event, const Event&>;

	template <std::invocable<std::exception_ptr> ExceptionHandler = rethrow_t>
	void push(reference_type event, ExceptionHandler&& exception_handler = {})
	{
		scope_cleanup cleanup{ this };
		_current_event = &event;
		this->_resume_chain(static_cast<reference_type>(event), std::forward<ExceptionHandler>(exception_handler));
	}

	template <std::invocable<std::exception_ptr> ExceptionHandler = rethrow_t>
	void operator()(reference_type event, ExceptionHandler&& exception_handler = {})
	{
		scope_cleanup cleanup{ this };
		_current_event = &event;
		this->_resume_chain(static_cast<reference_type>(event), std::forward<ExceptionHandler>(exception_handler));
	}
	
	auto await_resume() const -> reference_type
	{
		this->_rethrow_any();
		return static_cast<reference_type>(*_current_event);
	}

private:
	using pointer_type = std::add_pointer_t<reference_type>;
	
	pointer_type _current_event{};
};

template <
	typename Allocator
>
class event_hook<void, Allocator> : public detail::event_hook::event_hook_base<
	detail::event_hook::basic_event_hook_node<void>,
	typename detail::event_hook::event_hook_allocator<Allocator>::type
>
{
	using base = detail::event_hook::event_hook_base<
		detail::event_hook::basic_event_hook_node<void>,
		typename detail::event_hook::event_hook_allocator<Allocator>::type
	>;

public:
	using base::base;
	using base::operator=;
	
	using reference_type = void;

	template <std::invocable<std::exception_ptr> ExceptionHandler = rethrow_t>
	auto push(ExceptionHandler&& exception_handler = {})
	{
		this->_resume_chain(std::forward<ExceptionHandler>(exception_handler));
	}

	template <std::invocable<std::exception_ptr> ExceptionHandler = rethrow_t>
	void operator()(ExceptionHandler&& exception_handler = {})
	{
		this->_resume_chain(std::forward<ExceptionHandler>(exception_handler));
	}
	
	auto await_resume() const -> void
	{
		this->_rethrow_any();
	}
};

}
*/
#endif
