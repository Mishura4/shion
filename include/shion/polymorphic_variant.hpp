#ifndef SHION_POLYMORPHIC_VARIANT_H_
#define SHION_POLYMORPHIC_VARIANT_H_

#include <type_traits>
#include <cstddef>
#include <variant>

#include "type_list.h"

namespace shion {

template <typename Base, typename... Derived>
requires (std::is_base_of_v<Base, Derived> && ...)
class polymorphic_variant {
	template <size_t N>
	using type_at = pack_type<N, Base, Derived...>;

	using base_ref = std::add_lvalue_reference_t<Base>;
	using base_rval_ref = std::add_rvalue_reference_t<Base>;
	using base_const_ref = std::add_lvalue_reference_t<std::add_const_t<Base>>;
	using base_ptr = std::remove_reference_t<Base>*;
	using base_const_ptr = std::add_const_t<std::remove_reference_t<Base>>*;

public:
	static constexpr auto bad_index = std::numeric_limits<size_t>::max();
	static constexpr auto num_types = sizeof...(Derived) + 1;

	polymorphic_variant() = default;

	polymorphic_variant(const polymorphic_variant& other) noexcept (std::is_nothrow_copy_constructible_v<Base> && (std::is_nothrow_copy_constructible_v<Derived> && ...))
	requires (std::is_copy_constructible_v<Base> && (std::is_copy_constructible_v<Derived> && ...)) {
		_copy(other);
		_type_index = other._type_index;
	}

	polymorphic_variant(polymorphic_variant&& other) noexcept (std::is_nothrow_move_constructible_v<Base> && (std::is_nothrow_move_constructible_v<Derived> && ...))
	requires (std::is_move_constructible_v<Base> && (std::is_move_constructible_v<Derived> && ...)) {
		_move(other);
		_type_index = other._type_index;
	}

	template <typename T, typename... Args>
	requires (std::is_same_v<T, Base> || (std::is_same_v<T, Derived> || ...))
	explicit polymorphic_variant(std::in_place_type_t<T>, Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) :
		polymorphic_variant(std::in_place_index<index_of<T, Base, Derived...>>, std::forward<Args>(args)...)
	{}

	template <size_t N, typename... Args>
	requires (N < sizeof...(Derived) + 1)
	explicit polymorphic_variant(std::in_place_index_t<N>, Args&&... args) noexcept(std::is_nothrow_constructible_v<type_at<N>, Args...>) {
		std::construct_at(reinterpret_cast<type_at<N>*>(_storage), std::forward<Args>(args)...);
		_type_index = N;
	}

	template <typename T, typename... Args>
	requires (std::is_same_v<T, Base> || (std::is_same_v<T, Derived> || ...))
	T& emplace(std::in_place_type_t<T>, Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) {
		return emplace(std::in_place_index<index_of<T, Base, Derived...>>, std::forward<Args>(args)...);
	}

	template <size_t N, typename... Args>
	requires (N < sizeof...(Derived) + 1)
	type_at<N>& emplace(std::in_place_index_t<N>, Args&&... args) noexcept(std::is_nothrow_constructible_v<type_at<N>, Args...>) {
		_destroy_if_present();
		auto* ret = std::construct_at(reinterpret_cast<type_at<N>*>(_storage), std::forward<Args>(args)...);
		_type_index = N;
		return *ret;
	}

	polymorphic_variant &operator=(polymorphic_variant const& other) noexcept (std::is_nothrow_copy_constructible_v<Base> && (std::is_nothrow_copy_constructible_v<Derived> && ...))
	requires (std::is_copy_constructible_v<Base> && (std::is_copy_constructible_v<Derived> && ...)) {
		_destroy_if_present();
		_copy(other);
		_type_index = other._type_index;
		return *this;
	}

	polymorphic_variant &operator=(polymorphic_variant&& other) noexcept (std::is_nothrow_move_constructible_v<Base> && (std::is_nothrow_move_constructible_v<Derived> && ...))
	requires (std::is_move_constructible_v<Base> && (std::is_move_constructible_v<Derived> && ...)) {
		_destroy_if_present();
		_copy(other);
		_type_index = other._type_index;
		return *this;
	}

	explicit operator bool() const noexcept {
		return _type_index != bad_index;
	}

	auto operator*() & noexcept -> base_ref {
		return *_cast_unchecked<Base>();
	}

	auto operator*() const& noexcept -> base_const_ref {
		return *_cast_unchecked<Base>();
	}

	auto operator*() && noexcept -> base_rval_ref {
		return std::move(*_cast_unchecked<Base>());
	}

	auto operator->() noexcept -> base_ptr {
		return _cast_unchecked<Base>();
	}

	auto operator->() const noexcept -> base_const_ptr {
		return _cast_unchecked<Base>();
	}

	template <size_t N>
	requires (N < num_types)
	auto cast() & noexcept -> std::add_lvalue_reference_t<type_at<N>> {
		return *_cast_unchecked<N>();
	}

	template <size_t N>
	requires (N < num_types)
	auto cast() const& noexcept -> std::add_lvalue_reference_t<std::add_const_t<type_at<N>>> {
		return *_cast_unchecked<N>();
	}

	template <size_t N>
	requires (N < num_types)
	auto cast() && noexcept -> std::add_rvalue_reference_t<type_at<N>> {
		return std::move(*_cast_unchecked<N>());
	}

	template <typename T>
	auto cast() & noexcept -> std::add_lvalue_reference_t<T> {
		return cast<index_of<T>>();
	}

	template <typename T>
	auto cast() const& noexcept -> std::add_lvalue_reference_t<std::add_const_t<T>> {
		return cast<index_of<T>>();
	}

	template <typename T>
	auto cast() && noexcept -> std::add_rvalue_reference_t<T> {
		return std::move(*cast<index_of<T>>());
	}

	template <size_t N>
	requires (N < sizeof...(Derived) + 1)
	auto get() & -> std::add_lvalue_reference_t<type_at<N>> {
		if (_type_index != N) {
			throw std::bad_variant_access{};
		}
		return *reinterpret_cast<std::add_pointer_t<type_at<N>>>(_storage);
	}

	template <size_t N>
	requires (N < sizeof...(Derived) + 1)
	auto get() const& -> std::add_lvalue_reference_t<std::add_const_t<type_at<N>>> {
		if (_type_index != N) {
			throw std::bad_variant_access{};
		}
		return *reinterpret_cast<std::add_pointer_t<std::add_const_t<type_at<N>>>>(_storage);
	}

	template <size_t N>
	requires (N < sizeof...(Derived) + 1)
	auto get() && -> std::add_rvalue_reference_t<type_at<N>> {
		if (_type_index != N) {
			throw std::bad_variant_access{};
		}
		return std::move(*reinterpret_cast<std::add_pointer_t<type_at<N>>>(_storage));
	}

	template <typename T>
	auto get() & -> std::add_lvalue_reference_t<T> {
		return get<index_of<T>>();
	}

	template <typename T>
	auto get() const& -> std::add_lvalue_reference_t<std::add_const_t<T>> {
		return get<index_of<T>>();
	}

	template <typename T>
	auto get() && -> std::add_rvalue_reference_t<T> {
		return std::move(get<index_of<T>>());
	}

	template <size_t N>
	requires (N < sizeof...(Derived) + 1)
	auto get_if() -> std::add_pointer_t<type_at<N>> {
		if (_type_index != N) {
			throw std::bad_variant_access{};
		}
		return reinterpret_cast<std::add_pointer_t<type_at<N>>>(_storage);
	}

	template <size_t N>
	requires (N < sizeof...(Derived) + 1)
	auto get_if() const -> std::add_pointer_t<std::add_const_t<type_at<N>>> {
		if (_type_index != N) {
			throw std::bad_variant_access{};
		}
		return reinterpret_cast<std::add_pointer_t<std::add_const_t<type_at<N>>>>(_storage);
	}

	template <typename T>
	auto get_if() -> std::add_pointer_t<T> {
		return get_if<index_of<T>>();
	}

	template <typename T>
	auto get_if() const -> std::add_pointer_t<std::add_const_t<T>> {
		return get_if<index_of<T>>();
	}

	~polymorphic_variant() noexcept(std::is_nothrow_destructible_v<Base> && (std::is_nothrow_destructible_v<Derived> && ...)) {
		_destroy_if_present();
	}

private:
	template <size_t N>
	auto _cast_unchecked() {
		if constexpr (N == 0) {
			assert(_type_index != bad_index);
		} else {
			assert(_type_index == N);
		}

		return reinterpret_cast<type_at<N>*>(_storage);
	}
	template <size_t N>
	auto _cast_unchecked() const {
		if constexpr (N == 0) {
			assert(_type_index != bad_index);
		} else {
			assert(_type_index == N);
		}

		return reinterpret_cast<std::add_const_t<type_at<N>>*>(_storage);
	}

	template <size_t N>
	bool _copy_if_present(polymorphic_variant const& other) {
		if (_type_index == N) {
			std::construct_at(reinterpret_cast<type_at<N>*>(_storage), *reinterpret_cast<type_at<N> const*>(other._storage));
			return true;
		}
		return false;
	}

	void _copy(polymorphic_variant const& other) {
		if (_type_index == bad_index) {
			return;
		}
		[]<size_t... Ns>(polymorphic_variant *me, polymorphic_variant const& v, std::index_sequence<Ns...>){
			(me->_copy_if_present<Ns>(std::move(v)) || ...);
		}(this, other, std::index_sequence_for<Base, Derived...>{});
	}

	template <size_t N>
	bool _move_if_present(polymorphic_variant&& other) {
		if (_type_index == N) {
			std::construct_at(reinterpret_cast<type_at<N>*>(_storage), std::move(*reinterpret_cast<type_at<N> const*>(other._storage)));
			return true;
		}
		return false;
	}

	void _move(polymorphic_variant&& other) {
		if (_type_index == bad_index) {
			return;
		}
		[]<size_t... Ns>(polymorphic_variant *me, polymorphic_variant&& v, std::index_sequence<Ns...>){
			(me->_move_if_present<Ns>(std::move(v)) || ...);
		}(this, std::move(other), std::index_sequence_for<Base, Derived...>{});
	}

	template <size_t N>
	bool _destroy_if_present() {
		if (_type_index == N) {
			std::destroy_at(reinterpret_cast<type_at<N>*>(_storage));
			return true;
		}
		return false;
	}

	void _destroy_if_present() noexcept(std::is_nothrow_destructible_v<Base> && (std::is_nothrow_destructible_v<Derived> && ...)) {
		if (_type_index == bad_index) {
			return;
		}
		[]<size_t... Ns>(polymorphic_variant *me, std::index_sequence<Ns...>){
			(me->_destroy_if_present<Ns>() || ...);
		}(this, std::index_sequence_for<Base, Derived...>{});
	}

	std::byte _storage[(std::max)(sizeof(Base), sizeof(Derived)...)] alignas(Base) alignas(Derived...);
	size_t _type_index = bad_index;
};

template <typename T, typename Base, typename... Args>
requires (is_unique<T, Base, Args...>)
decltype(auto) get(polymorphic_variant<Base, Args...>& v) {
	return v.template get<T>();
}

template <typename T, typename Base, typename... Args>
requires (is_unique<T, Base, Args...>)
decltype(auto) get(polymorphic_variant<Base, Args...> const& v) {
	return v.template get<T>();
}

template <typename T, typename Base, typename... Args>
requires (is_unique<T, Base, Args...>)
decltype(auto) get(polymorphic_variant<Base, Args...>&& v) {
	return std::move(v).template get<T>();
}

template <size_t N, typename Base, typename... Args>
requires (N < sizeof...(Args) + 1)
decltype(auto) get(polymorphic_variant<Base, Args...>& v) {
	return v.template get<N>();
}

template <size_t N, typename Base, typename... Args>
requires (N < sizeof...(Args) + 1)
decltype(auto) get(polymorphic_variant<Base, Args...> const& v) {
	return v.template get<N>();
}

template <size_t N, typename Base, typename... Args>
requires (N < sizeof...(Args) + 1)
decltype(auto) get(polymorphic_variant<Base, Args...>&& v) {
	return std::move(v).template get<N>();
}


template <typename T, typename Base, typename... Args>
requires (is_unique<T, Base, Args...>)
decltype(auto) get_if(polymorphic_variant<Base, Args...>* v) {
	return v->template get_if<T>();
}

template <typename T, typename Base, typename... Args>
requires (is_unique<T, Base, Args...>)
decltype(auto) get_if(polymorphic_variant<Base, Args...> const* v) {
	return v->template get_if<T>();
}

template <size_t N, typename Base, typename... Args>
requires (N < sizeof...(Args) + 1)
decltype(auto) get_if(polymorphic_variant<Base, Args...>* v) {
	return v->template get_if<N>();
}

template <size_t N, typename Base, typename... Args>
requires (N < sizeof...(Args) + 1)
decltype(auto) get_if(polymorphic_variant<Base, Args...> const* v) {
	return v->template get_if<N>();
}

template <typename T, typename Base, typename... Args>
requires (is_unique<T, Base, Args...>)
decltype(auto) cast(polymorphic_variant<Base, Args...>& v) {
	return v.template cast<T>();
}

template <typename T, typename Base, typename... Args>
requires (is_unique<T, Base, Args...>)
decltype(auto) cast(polymorphic_variant<Base, Args...> const& v) {
	return v.template cast<T>();
}

template <typename T, typename Base, typename... Args>
requires (is_unique<T, Base, Args...>)
decltype(auto) cast(polymorphic_variant<Base, Args...>&& v) {
	return std::move(v).template cast<T>();
}

template <size_t N, typename Base, typename... Args>
requires (N < sizeof...(Args) + 1)
decltype(auto) cast(polymorphic_variant<Base, Args...>& v) {
	return v.template cast<N>();
}

template <size_t N, typename Base, typename... Args>
requires (N < sizeof...(Args) + 1)
decltype(auto) cast(polymorphic_variant<Base, Args...> const& v) {
	return v.template cast<N>();
}

template <size_t N, typename Base, typename... Args>
requires (N < sizeof...(Args) + 1)
decltype(auto) cast(polymorphic_variant<Base, Args...>&& v) {
	return std::move(v).template cast<N>();
}

}

#endif
