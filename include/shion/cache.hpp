#ifndef SHION_CACHE_H_
#define SHION_CACHE_H_

#include <atomic>
#include <type_traits>
#include <mutex>
#include <list>
#include <cassert>
#include <optional>
#include <utility>

namespace shion {

template <typename Value>
class cached_resource;

namespace detail {

template <typename Value>
requires (!std::is_reference_v<Value> && !std::is_const_v<Value>)
class stored_cache_resource {
public:
	using value_t = Value;

	constexpr stored_cache_resource() {
	}

	void increment() {
		ref_count.fetch_add(1, std::memory_order_relaxed);
	}

	bool decrement() {
		if (ref_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
			_destroy();
			return true;
		}
		return false;
	}

	[[nodiscard]] Value &get() noexcept {
		return v;
	}

	constexpr explicit operator bool() const noexcept {
		return ref_count.load(std::memory_order_relaxed) > 0;
	}

	template <typename... Args>
	[[nodiscard]] cached_resource<Value> emplace(Args&&... args)
	noexcept(std::is_nothrow_constructible_v<std::remove_cvref_t<Value>, Args...>);

	~stored_cache_resource() {
		assert(ref_count == 0);
	}

private:
	void _destroy() {
		assert(ref_count == 0);
		std::destroy_at(&v);
	}

	std::atomic<intptr_t> ref_count{0};
	union {
		Value v;
	};
};

template <typename T>
using shared_cached_resource = stored_cache_resource<std::conditional_t<std::is_reference_v<T>, std::add_pointer_t<T>, std::remove_const_t<T>>>;

}

template <typename Value>
class cached_resource {
	template <typename, typename, typename, typename>
	friend class cache;

	using resource = detail::shared_cached_resource<Value>;

	friend resource;

	using value_t = typename resource::value_t;

	cached_resource(resource& res) noexcept :
		ptr{&res} {
		ptr->increment();
	}

public:
	friend class cached_resource<std::add_const_t<Value>>;

	cached_resource() noexcept = default;

	cached_resource(const cached_resource &other) noexcept :
		ptr{other.ptr} {
		if (ptr) {
			ptr->increment();
		}
	}

	cached_resource(const cached_resource<std::remove_const_t<Value>> &other) noexcept
		requires(std::is_const_v<Value>) :
		ptr{other.ptr} {
		if (ptr) {
			ptr->increment();
		}
	}

	cached_resource(cached_resource&& rhs) noexcept :
		ptr{std::exchange(rhs.ptr, nullptr)}
	{}

	~cached_resource() {
		if (ptr) {
			ptr->decrement();
		}
	}

	cached_resource &operator=(const cached_resource &other) noexcept {
		ptr = other.ptr;
		if (ptr) {
			ptr->increment();
		}
		return *this;
	}

	cached_resource &operator=(cached_resource &&other) noexcept {
		ptr = std::exchange(other.ptr, nullptr);
		return *this;
	}

	constexpr explicit operator bool() const noexcept {
		return ptr;
	}

	[[nodiscard]] auto operator*() const noexcept -> std::add_lvalue_reference_t<std::add_const_t<Value>> {
		assert(ptr);

		if constexpr (std::is_reference_v<Value>) {
			return *ptr->get();
		} else {
			return ptr->get();
		}
	}

	[[nodiscard]] auto operator*() noexcept -> std::add_lvalue_reference_t<Value>{
		assert(ptr);

		if constexpr (std::is_reference_v<Value>) {
			return *ptr->get();
		} else {
			return ptr->get();
		}
	}

	std::add_pointer_t<Value> operator->() noexcept {
		assert(ptr);

		return &ptr->get();
	}

	std::add_pointer_t<std::add_const_t<Value>> *operator->() const noexcept {
		assert(ptr);

		return &ptr->operator*();
	}

	std::add_const_t<value_t> &get() const {
		cached_resource const& self = *this;
		if (!self) {
			throw std::bad_optional_access{};
		}
		return self.operator*();
	}

	value_t& get() {
		cached_resource& self = *this;
		if (!self) {
			throw std::bad_optional_access{};
		}
		return self.operator*();
	}

	void release() noexcept {
		if (ptr) {
			ptr->decrement();
		}
		ptr = nullptr;
	}

private:
	resource* ptr = nullptr;
};

template <typename Value>
requires (!std::is_reference_v<Value> && !std::is_const_v<Value>)
template <typename... Args>
[[nodiscard]] cached_resource<Value> detail::stored_cache_resource<Value>::emplace(Args&&... args)
noexcept(std::is_nothrow_constructible_v<std::remove_cvref_t<Value>, Args...>) {
	assert(ref_count == 0);
	std::construct_at(
		&v,
		std::forward<Args>(args)...
	);
	return {*this};
}

template <typename Key, typename Value, typename Hasher, typename Equal>
class cache {
private:
public:
	constexpr cache() noexcept = default;
	cache(const cache&) = delete;
	cache(cache&&) = delete;
	cache &operator=(const cache&) = delete;
	cache &operator=(cache&&) = delete;

	using element_t = std::pair<Key, Value>;
	using value_t = cached_resource<Value>;

	template <typename T>
	static inline constexpr auto nothrow_hash = noexcept(Hasher{}(std::declval<T>()));

	template <typename T>
	static inline constexpr auto nothrow_equal = noexcept(Equal{}(std::declval<Key const&>(), std::declval<T>()));

	template <typename T>
	static inline constexpr auto nothrow_lookup = noexcept(nothrow_hash<T> && nothrow_equal<T>);

	template <typename T, typename... Args>
	static inline constexpr auto nothrow_emplace = std::is_nothrow_constructible_v<Key, T> && std::is_nothrow_constructible_v<Value, Args...>;

private:
	using resource = typename cached_resource<Value>::resource;

	struct node {
		size_t hash{0};
		std::pair<Key, resource> elem;
		cached_resource<Value> my_ref;
	};

	struct bucket {
		static constexpr size_t num_elements = std::max(32ull, (32ull * 1024 / (sizeof(node))));
		using storage = std::array<node, num_elements>;
		using storage_iterator = std::ranges::iterator_t<storage>;

		constexpr auto begin() {
			return data.begin();
		}

		constexpr auto begin() const {
			return data.begin();
		}

		constexpr auto end() {
			return data.end();
		}

		constexpr auto end() const {
			return data.end();
		}

		std::array<node, num_elements> data{};
	};

public:
	template <typename T>
	cached_resource<Value> find(const T& key) noexcept(nothrow_lookup<T>) {
		size_t hashed = hash(key);
		std::shared_lock lock{mutex};

		return _find_hash(key, hashed);
	}

	template <typename T>
	cached_resource<std::add_const_t<Value>> find(const T& key) const noexcept(nothrow_lookup<T>) {
		size_t hashed = hash(key);
		std::shared_lock lock{mutex};

		return _find_hash(key, hashed);
	}

	template <typename T>
	cached_resource<Value> find_hash(const T& key, size_t hash) noexcept(nothrow_equal<T>) {
		std::shared_lock lock{mutex};

		return _find_hash(key, hash);
	}

	template <typename T>
	cached_resource<std::add_const_t<Value>> find_hash(const T& key, size_t hash) const noexcept(nothrow_equal<T>) {
		std::shared_lock lock{mutex};

		return _find_hash(key, hash);
	}

	template <typename T, typename... Args>
	std::pair<cached_resource<Value>, bool> try_emplace(T&& key, Args&&... args) noexcept(nothrow_lookup<T> && nothrow_emplace<Args...>) {
		size_t hashed = hash(key);
		std::lock_guard lock{mutex};

		if (auto res = _find_hash(key, hashed); res) {
			return {res, false};
		}

		return {_emplace(std::forward<T>(key), hashed, std::forward<Args>(args)...), true};
	}

	template <typename T>
	requires (std::is_constructible_v<Key, T> && std::is_default_constructible_v<Value>)
	cached_resource<Value> operator[](T&& key) noexcept (nothrow_lookup<T> && nothrow_emplace<T>) {
		size_t hashed = hash(key);
		std::lock_guard lock{mutex};

		if (auto res = _find_hash(key, hashed); res) {
			return res;
		}

		return _emplace(std::forward<T>(key), hashed);
	}

	template <typename T>
	size_t hash(const T& key) const noexcept(nothrow_hash<const T&>) {
		return Hasher{}(key);
	}

private:
	template <typename T>
	cached_resource<Value> _find_hash(const T& key, size_t hash) noexcept(nothrow_equal<T>) {
#ifdef _MSC_VER
		if (_buckets.empty())
			return {};
#endif
		for (bucket &b : _buckets) {
			if (auto it = std::ranges::find(b, hash, &node::hash); it != b.end()) {
				if (auto ref = it->my_ref; ref) { // use the reference here because that means the resource can't be destroyed in-between
					if (it->elem.first == key) {
						return ref;
					}
				}
			}
		}
		return {};
	}

	template <typename T>
	cached_resource<std::add_const_t<Value>> _find_hash(const T& key, size_t hash) const noexcept(nothrow_equal<T>) {
#ifdef _MSC_VER
		if (_buckets.empty())
			return {};
#endif
		for (const bucket &b : _buckets) {
			if (auto it = std::ranges::find(b, hash, &node::hash); it != b.end()) {
				if (it->my_ref) { // use the reference here because that means the resource can't be destroyed in-between
					const auto& [elem_key, _] = *(it->my_ref);
					if (elem_key == key) {
						return it->my_ref;
					}
				}
			}
		}
		return {};
	}

	template <typename T, typename... Args>
	cached_resource<Value> _emplace(T&& key, size_t hash, Args&&... args) noexcept(nothrow_emplace<T, Args...>) {
		constexpr auto is_empty = [](const node &r) noexcept {
			return !r.elem.second;
		};

		for (bucket &b : _buckets) {
			if (auto it = std::ranges::find_if(b, is_empty); it != b.end()) {
				it->hash = hash;
				it->elem.first = std::forward<T>(key);
				it->my_ref = it->elem.second.emplace(std::forward<Args>(args)...);
				return it->my_ref;
			}
		}

		bucket &b = _buckets.emplace_back();
		b.data[0].hash = hash;
		b.data[0].elem.first = std::forward<T>(key);
		b.data[0].my_ref = b.data[0].elem.second.emplace(std::forward<Args>(args)...);
		return {b.data[0].my_ref};
	}

	mutable std::shared_mutex mutex;
	std::list<bucket> _buckets;
};

}

#endif SHION_CACHE_H_
