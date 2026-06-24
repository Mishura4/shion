#ifndef SHION_TOOLS_STORAGE_H_
#define SHION_TOOLS_STORAGE_H_

#include <shion/common/defines.hpp>

#if !SHION_BUILDING_MODULES
#    include <utility>
#    include <functional>
#    include <optional>
#    include <memory>
#    include <cassert>
#    include <ranges>
#    include <optional>
#endif

namespace SHION_NAMESPACE
{

SHION_EXPORT template <typename T>
class storage
{
public:
	using value_type = T;
	using reference = typename std::add_lvalue_reference<T>::type;
	using const_reference = typename std::add_lvalue_reference<typename std::add_const<T>::type>::type;
	using rvalue_reference = typename std::add_rvalue_reference<T>::type;
	using const_rvalue_reference = typename std::add_rvalue_reference<typename std::add_const<T>::type>::type;
	using pointer = typename std::add_pointer<T>::type;
	using const_pointer = typename std::add_pointer<typename std::add_const<T>::type>::type;
	
	template <typename... Args>
		requires (std::constructible_from<value_type, Args...>)
	constexpr storage(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) :
		_value(std::forward<Args>(args)...)
	{
	}

	template <typename U>
	constexpr auto operator=(U&& new_value) noexcept(std::is_nothrow_assignable_v<T&, U>) -> storage&
		requires (std::assignable_from<U, T&>)
	{
		_value = std::forward<T>(new_value);
		return *this;
	}
	
	constexpr friend auto operator<=>(const storage&, const storage&) = default;
	
	constexpr auto operator*() & noexcept -> reference
	{
		return _value;
	}
	
	constexpr auto operator*() && noexcept -> rvalue_reference
	{
		return std::move(_value);
	}
	
	constexpr auto operator*() const& noexcept -> const_reference
	{
		return _value;
	}
	
	constexpr auto operator*() const&& noexcept -> const_rvalue_reference
	{
		return std::move(_value);
	}
	
	constexpr auto operator->() noexcept -> pointer
	{
		return std::addressof(_value);
	}
	
	constexpr auto operator->() const noexcept -> const_pointer
	{
		return std::addressof(_value);
	}

private:
	value_type _value;
};

template <typename T>
requires (std::is_reference_v<T>)
class storage<T>
{
	
public:
	using value_type = typename std::remove_reference<T>::type;
	using reference = T;
	using const_reference = T;
	using rvalue_reference = T;
	using const_rvalue_reference = T;
	using pointer = typename std::add_pointer<value_type>::type;

	constexpr storage(SHION_LIFETIMEBOUND T&& object) noexcept : _ptr(std::addressof(object))
	{}
	
	template <typename U>
	constexpr auto operator=(U&& new_value) noexcept -> storage&
		requires (std::is_convertible_v<std::add_pointer_t<U>, pointer>)
	{
		_ptr = std::addressof(new_value);
		return *this;
	}
	
	constexpr friend auto operator<=>(const storage&, const storage&) = default;
	
	constexpr auto operator*() const noexcept -> reference
	{
		return static_cast<reference>(*_ptr);
	}

	constexpr auto operator->() const noexcept -> pointer
	{
		return _ptr;
	}
	
private:
	pointer _ptr;
};

template <>
class storage<void>
{
public:
	using value_type = void;
	using reference = void;
	using const_reference = void;
	using rvalue_reference = void;
	using const_rvalue_reference = void;
	using pointer = void;

	constexpr friend auto operator<=>(const storage&, const storage&) = default;
	
	constexpr void operator*() const {}
	constexpr void operator->() const = delete;
};

template <typename T>
storage(T value) -> storage<T>;

SHION_EXPORT template <typename Value, typename Ref = void>
class wrapper;

template <typename Value, typename Ref>
class wrapper : public storage<Value>
{
	using storage_base = storage<Value>;
	using as_const_value = typename std::add_const<Value>::type;
	
public:
	using storage_base::storage_base;
	using storage_base::operator=;
	using storage_base::operator->;
	
	using reference = std::conditional_t<requires (Value& v) { static_cast<Ref>(v); }, Ref, void>;
	using rvalue_reference = std::conditional_t<requires (Value&& v) { static_cast<Ref>(v); }, Ref, void>;
	using const_reference = std::conditional_t<requires (as_const_value& v) { static_cast<Ref>(v); }, Ref, void>;
	using const_rvalue_reference = std::conditional_t<requires (as_const_value&& v) { static_cast<Ref>(v); }, Ref, void>;

	constexpr friend auto operator<=>(const wrapper&, const wrapper&) = default;

	constexpr auto operator*() & noexcept(std::is_nothrow_constructible_v<reference, Value&>) -> reference
		requires (!std::is_void_v<reference>)
	{
		return static_cast<reference>(*static_cast<storage_base&>(*this));
	}

	constexpr auto operator*() && noexcept(std::is_nothrow_constructible_v<rvalue_reference, Value&&>) -> rvalue_reference
		requires (!std::is_void_v<rvalue_reference>)
	{
		return static_cast<rvalue_reference>(*static_cast<storage_base&&>(*this));
	}

	constexpr auto operator*() const& noexcept(std::is_nothrow_constructible_v<const_reference, as_const_value&>) -> const_reference
		requires (!std::is_void_v<const_reference>)
	{
		return static_cast<const_reference>(*static_cast<const storage_base&>(*this));
	}

	constexpr auto operator*() const&& noexcept(std::is_nothrow_constructible_v<const_rvalue_reference, as_const_value&&>) -> const_rvalue_reference
		requires (!std::is_void_v<const_rvalue_reference>)
	{
		return static_cast<const_rvalue_reference>(*static_cast<const storage_base&&>(*this));
	}
};

template <typename Value>
requires (!std::is_void_v<Value>)
class wrapper<Value, void> : public storage<Value>
{
	using storage_base = storage<Value>;
	using as_const_value = typename std::add_const<Value>::type;
	
public:
	using storage_base::storage_base;
	using storage_base::operator=;
	using storage_base::operator->;
	
	using reference = std::conditional_t<std::is_reference_v<Value>, Value, Value&>;
	using rvalue_reference = std::conditional_t<std::is_reference_v<Value>, Value, Value&&>;
	using const_reference = std::conditional_t<std::is_reference_v<Value>, Value, Value const&>;
	using const_rvalue_reference = std::conditional_t<std::is_reference_v<Value>, Value, Value const&&>;

	constexpr friend auto operator<=>(const wrapper&, const wrapper&) = default;

	constexpr auto operator*() & noexcept(std::is_nothrow_constructible_v<reference, Value&>) -> reference
		requires (!std::is_void_v<reference>)
	{
		return static_cast<reference>(*static_cast<storage_base&>(*this));
	}

	constexpr auto operator*() && noexcept(std::is_nothrow_constructible_v<rvalue_reference, Value&&>) -> rvalue_reference
		requires (!std::is_void_v<rvalue_reference>)
	{
		return static_cast<rvalue_reference>(*static_cast<storage_base&&>(*this));
	}

	constexpr auto operator*() const& noexcept(std::is_nothrow_constructible_v<const_reference, as_const_value&>) -> const_reference
		requires (!std::is_void_v<const_reference>)
	{
		return static_cast<const_reference>(*static_cast<const storage_base&>(*this));
	}

	constexpr auto operator*() const&& noexcept(std::is_nothrow_constructible_v<const_rvalue_reference, as_const_value&&>) -> const_rvalue_reference
		requires (!std::is_void_v<const_rvalue_reference>)
	{
		return static_cast<const_rvalue_reference>(*static_cast<const storage_base&&>(*this));
	}
};

template <typename Ref>
class wrapper<void, Ref> : public storage<void>
{
	using storage_base = storage<void>;
	
public:
	using storage_base::storage_base;
	using storage_base::operator=;
	using storage_base::operator->;

	constexpr friend auto operator<=>(const wrapper&, const wrapper&) = default;

	constexpr auto operator*() const noexcept(std::is_nothrow_constructible_v<Ref>) -> Ref
	{
		return Ref();
	}
};

template <typename T>
wrapper(T value) -> wrapper<T>;

namespace detail
{

struct result_get_functions
{
	template <size_t N, typename Self>
	friend constexpr auto get(Self&& self)  -> decltype(auto)
		requires requires (Self s) { s.template get<N>(); }
	{
		return static_cast<Self&&>(self).template get<N>();
	}

	template <size_t N, typename Self>
	friend constexpr auto get_unchecked(Self&& self) -> decltype(auto)
		requires requires (Self s) { s.template get_unchecked<N>(); }
	{
		return static_cast<Self&&>(self).template get_unchecked<N>();
	}
};

}

SHION_EXPORT template <typename... Ts>
class basic_result;

template <typename... Ts>
class basic_result : public detail::result_get_functions
{
protected:
	using variant_type = std::variant<std::monostate, std::exception_ptr, Ts...>;
	
	inline static constexpr size_t IDX_EMPTY = 0;
	inline static constexpr size_t IDX_EXCEPTION = 1;
	inline static constexpr size_t IDX_VALUE_BEGIN = 2;

public:
#if defined(__cpp_pack_indexing) && __cpp_pack_indexing >= 202311L && __cplusplus >= 202400L
	template <size_t N>
	using alternative_t = Ts...[N];
#else
	template <size_t N> requires (N < sizeof...(Ts))
	using alternative_t = std::variant_alternative_t<N + IDX_VALUE_BEGIN, variant_type>;
#endif

	constexpr basic_result() noexcept = default;
	constexpr basic_result(std::exception_ptr exception) noexcept : _value(std::in_place_index<1>, std::move(exception))
	{}

	constexpr basic_result(std::monostate) noexcept : _value()
	{}
	
	constexpr basic_result(std::monostate, std::exception_ptr exception) noexcept : _value(std::in_place_index<1>, std::move(exception))
	{}
	
	template <typename... Args>
	constexpr basic_result(Args&&... args)
		noexcept(std::is_nothrow_constructible_v<variant_type, Args...>)
		requires (std::constructible_from<variant_type, Args...>) :
		_value(std::forward<Args>(args)...)
	{}
	
	template <typename T, typename... Args>
	constexpr basic_result(std::in_place_type_t<T>, Args&&... args)
		noexcept(std::is_nothrow_constructible_v<variant_type, std::in_place_type_t<T>, Args...>)
		requires (std::constructible_from<variant_type, std::in_place_type_t<T>, Args...>) :
		_value(std::in_place_type_t<T>{}, std::forward<Args>(args)...)
	{}
	
	template <size_t N, typename... Args>
	constexpr basic_result(std::in_place_index_t<N>, Args&&... args)
		noexcept(std::is_nothrow_constructible_v<alternative_t<N>, Args...>)
		requires (std::constructible_from<std::variant_alternative_t<N + 2, variant_type>, Args...>) :
		_value(std::in_place_index_t<N + IDX_VALUE_BEGIN>{}, std::forward<Args>(args)...)
	{}
	
	constexpr auto operator=(std::exception_ptr ptr) noexcept -> basic_result&
	{
		set_exception(std::move(ptr));
		return *this;
	}
	
	constexpr void set_exception(std::exception_ptr ptr) noexcept
	{
		_value.template emplace<IDX_EXCEPTION>(std::move(ptr));
	}
	
	template <size_t N, typename... Args>
	constexpr auto emplace(Args&&... args) noexcept(std::is_nothrow_constructible_v<alternative_t<N>, Args...>) -> decltype(auto)
		requires (N < sizeof...(Ts) && std::constructible_from<alternative_t<N>, Args...>)
	{
		return _value.template emplace<N + IDX_VALUE_BEGIN>(std::forward<Args>(args)...);
	}
	
	constexpr void clear() noexcept(std::is_nothrow_destructible_v<Ts...>)
	{
		_value.template emplace<0>();
	}
	
	constexpr bool empty() const noexcept
	{
		return _value.index() == IDX_EMPTY;
	}
	
	constexpr bool has_exception() const noexcept
	{
		return _value.index() == IDX_EXCEPTION;
	}
	
	constexpr auto index() const noexcept -> size_t
	{
		return empty() ? std::numeric_limits<size_t>::max() : _value.index() - IDX_VALUE_BEGIN;
	}
	
	constexpr auto get_exception() const& noexcept -> std::exception_ptr
	{
		if (auto* exception = std::get_if<IDX_EXCEPTION>(&_value); exception != nullptr)
			return *exception;
		return std::exception_ptr();
	}
	
	constexpr auto get_exception() && noexcept -> std::exception_ptr
	{
		if (auto* exception = std::get_if<IDX_EXCEPTION>(&_value); exception != nullptr)
			return std::move(*exception);
		return std::exception_ptr();
	}
	
	template <size_t N, typename Self>
	requires (N < sizeof...(Ts))
	constexpr auto get_unchecked(this Self&& result) noexcept -> decltype(auto)
	{
		return std::forward_like<Self&&>(*get_if<N + 2>(&result._value));
	}
	
	template <size_t N, typename Self>
	requires (N < sizeof...(Ts))
	constexpr auto get(this Self&& result) -> decltype(auto)
	{
		using std::get;
		result._maybe_rethrow();
		result._check_alternative(N);
		return shion::get_unchecked<N>(static_cast<Self&&>(result));
	}
	
	template <size_t N>
	requires (N < sizeof...(Ts))
	friend constexpr auto get_if(basic_result* result) noexcept -> decltype(auto)
	{
		using std::get;
		return get_if<N + 2>(&result->_value);
	}
	
	template <size_t N>
	requires (N < sizeof...(Ts))
	friend constexpr auto get_if(const basic_result* result) noexcept -> decltype(auto)
	{
		using std::get;
		return get_if<N + 2>(&result->_value);
	}
	
	template <typename Self>
	constexpr auto get(this Self&& self) -> decltype(auto)
		requires (sizeof...(Ts) == 1)
	{
		return shion::get<0>(static_cast<Self&&>(self));
	}
	
	template <typename Self>
	constexpr auto operator*(this Self&& self) noexcept -> decltype(auto)
		requires (sizeof...(Ts) == 1)
	{
		return shion::get_unchecked<0>(static_cast<Self&&>(self));
	}
	
	constexpr auto operator->() noexcept -> decltype(auto)
		requires (sizeof...(Ts) == 1)
	{
		return get_if<2>(_value);
	}
	
	constexpr auto operator->() const noexcept -> decltype(auto)
		requires (sizeof...(Ts) == 1)
	{
		return get_if<2>(_value);
	}
	
	friend constexpr auto operator<=>(const basic_result&, const basic_result&) = default;
	
private:
	constexpr void _maybe_rethrow() const
	{
		if (auto* exception = std::get_if<IDX_EXCEPTION>(&_value); exception != nullptr)
			std::rethrow_exception(*exception);
	}
	
	constexpr void _check_alternative(size_t n) const
	{
		if (_value.index() != n + 2)
			throw std::bad_variant_access();
	}
	
	std::variant<std::monostate, std::exception_ptr, Ts...> _value;
};

template <typename T>
basic_result(T t) -> basic_result<T>;

basic_result() -> basic_result<void>;

template <typename... Ts>
class wrapped_result : public basic_result<Ts...>
{
	using base_result = basic_result<Ts...>;

public:
	template <size_t N, typename Self>
	requires (N < sizeof...(Ts))
	constexpr auto get_unchecked(this Self&& result) noexcept -> decltype(auto)
	{
		return *(static_cast<Self&&>(result).base_result::template get_unchecked<N>());
	}

	using base_result::base_result;
	using base_result::operator=;
	
	friend constexpr auto operator<=>(const wrapped_result&, const wrapped_result&) = default;
};

template <typename... Ts>
class result : wrapped_result<wrapper<Ts>...>
{
	using base_result = wrapped_result<wrapper<Ts>...>;

public:
	using base_result::base_result;
	using base_result::operator=;
	
	friend constexpr auto operator<=>(const result&, const result&) = default;
};

template <typename T>
result(T t) -> result<T>;

result() -> result<void>;

}

#endif // SHION_TOOLS_STORAGE_H_
