#ifndef SHION_CONTAINERS_HIVE_H_
#define SHION_CONTAINERS_HIVE_H_

#include <type_traits>
#include <bit>
#include <list>

#include "../shion_essentials.hpp"
#include "../utility/optional.hpp"

namespace shion {

template <typename T>
class hive;

namespace detail {

inline constexpr ssize_t hive_page_target_size = 24 * 1024;

template <typename T>
inline constexpr ssize_t hive_page_num_skipfields = std::max(1_sst, lossless_cast<ssize_t>((hive_page_target_size / sizeof(T)) / 64));

template <typename T>
inline constexpr ssize_t hive_page_num_elements = hive_page_num_skipfields<T> * 64;

template <typename T>
union hive_storage {
	constexpr hive_storage() noexcept requires (std::is_trivially_default_constructible_v<T>) = default;
	constexpr hive_storage() noexcept(std::is_nothrow_constructible<T>) requires (!std::is_trivially_default_constructible_v<T>) : dummy{} {}
	constexpr hive_storage(hive_storage const&) noexcept requires (std::is_trivially_copy_constructible_v<T>) = default;
	constexpr hive_storage(hive_storage&&) noexcept requires (std::is_trivially_move_constructible_v<T>) = default;
	constexpr hive_storage(T const& rhs) requires (std::is_copy_constructible_v<T>) : value(rhs) {}
	constexpr hive_storage(T&& rhs) requires (std::is_move_constructible_v<T>) : value(std::move(rhs)) {}
	constexpr ~hive_storage() {}

	template <typename... Args>
	T& emplace(Args&&... args) {
		return *std::construct_at(std::addressof(value), std::forward<Args>(args)...);
	}

	void destroy() {
		std::destroy_at(std::addressof(value));
	}

	empty dummy;
	T value;
};

template <typename T>
requires (std::is_trivial_v<T>)
union hive_storage<T> {
	template <typename... Args>
	T& emplace(Args&&... args) {
		return *std::construct_at(&value, std::forward<Args>(args)...);
	}

	void destroy() const noexcept {}

	T value;
};

template <typename T>
class hive_page_base;

template <typename T>
requires (std::is_trivial_v<T>)
class hive_page_base<T> {
protected:
	friend class hive<T>;

	ssize_t                                                   global_index_of_first{};
	uint64                                                    _skipfields[hive_page_num_elements<T>];
	std::array<hive_storage<T>, hive_page_num_elements<T>> data{};
};

template <typename T>
class hive_page_base {
public:
	constexpr hive_page_base() noexcept = default;

	constexpr hive_page_base(const hive_page_base& rhs) noexcept(std::is_nothrow_copy_constructible_v<T>) requires (std::is_copy_constructible_v<T>) {
		for (size_t i = 0; i < hive_page_num_skipfields<T>; ++i) {
			auto rhs_field = rhs._skipfields[i];

			if (rhs_field == 0) {
				continue;
			}
			for (size_t j = 0; j < 64; ++j) {
				size_t flag = (1_st << j);
				size_t idx = (i << 6) | j;
				if ((rhs_field & flag) != 0) {
					data[idx].emplace(rhs.data[idx].value);
					if constexpr (!std::is_nothrow_copy_constructible_v<T>) {
						_skipfields[i] |= flag;
					}
				}
				if constexpr (std::is_nothrow_copy_constructible_v<T>) {
					_skipfields[i] = rhs_field;
				}
			}
		}
	}
	constexpr hive_page_base(hive_page_base&& rhs) noexcept(std::is_nothrow_move_constructible_v<T>) requires (std::is_move_constructible_v<T>) {
		for (size_t i = 0; i < hive_page_num_skipfields<T>; ++i) {
			auto rhs_field = rhs._skipfields[i];

			if (rhs_field == 0) {
				continue;
			}
			for (size_t j = 0; j < 64; ++j) {
				size_t flag = (1_st << j);
				size_t idx = (i << 6) | j;
				if ((rhs_field & flag) != 0) {
					data[idx].emplace(rhs.data[idx].value);
					if constexpr (!std::is_nothrow_move_constructible_v<T>) {
						_skipfields[i] |= flag;
					}
				}
				if constexpr (std::is_nothrow_move_constructible_v<T>) {
					_skipfields[i] = rhs_field;
				}
			}
		}
	}
	constexpr ~hive_page_base() noexcept requires (std::is_trivially_destructible_v<T>) = default;
	constexpr ~hive_page_base() noexcept(std::is_nothrow_destructible_v<T>) requires (!std::is_trivially_destructible_v<T>) {
		for (ssize_t i = 0; i < hive_page_num_skipfields<T>; ++i) {
			if (_skipfields[i] == 0) {
				continue;
			} else if (_skipfields[i] == ~0_u64) {
				for (ssize_t j = 0; j < 64; ++j) {
					data[(i << 6) | j].destroy();
				}
			} else {
				for (ssize_t j = 0; j < 64; ++j) {
					if ((_skipfields[i] & (1_u64 << j)) != 0) {
						data[(i << 6) | j].destroy();
					}
				}
			}
		}
	}

	constexpr hive_page_base& operator=(const hive_page_base& rhs) noexcept(is_nothrow_copyable<T> && std::is_nothrow_destructible_v<T>) requires (is_copyable<T>) {
		constexpr auto nothrow = is_nothrow_copyable<T> && std::is_nothrow_destructible_v<T>;

		for (size_t i = 0; i < hive_page_num_skipfields<T>; ++i) {
			auto rhs_field = rhs._skipfields[i];
			auto &lhs_field = _skipfields[i];

			for (size_t j = 0; j < 64; ++j) {
				size_t flag = (1_st << j);
				size_t idx = (i << 6) | j;
				if ((lhs_field & flag) != 0) {
					if ((rhs_field & flag) != 0) {
						data[idx].value = rhs.data[idx].value;
					} else {
						data[idx].destroy();
						if constexpr (!nothrow) {
							lhs_field &= ~flag;
						}
					}
				} else {
					if ((rhs_field & flag) != 0) {
						data[idx].emplace(rhs.data[idx].value);
						if constexpr (!nothrow) {
							lhs_field |= flag;
						}
					}
				}
				if constexpr (nothrow) {
					_skipfields[i] = rhs_field;
				}
			}
		}
		return *this;
	}

	constexpr hive_page_base& operator=(hive_page_base&& rhs) noexcept(is_nothrow_moveable<T>) requires (is_moveable<T>) {
		constexpr auto nothrow = is_nothrow_copyable<T> && std::is_nothrow_destructible_v<T>;

		for (size_t i = 0; i < hive_page_num_skipfields<T>; ++i) {
			auto rhs_field = rhs._skipfields[i];
			auto &lhs_field = _skipfields[i];

			for (size_t j = 0; j < 64; ++j) {
				size_t flag = (1_st << j);
				size_t idx = (i << 6) | j;
				if ((lhs_field & flag) != 0) {
					if ((rhs_field & flag) != 0) {
						data[idx].value = std::move(rhs.data[idx].value);
					} else {
						data[idx].destroy();
						if constexpr (!nothrow) {
							lhs_field &= ~flag;
						}
					}
				} else {
					if ((rhs_field & flag) != 0) {
						data[idx].emplace(std::move(rhs.data[idx].value));
						if constexpr (!nothrow) {
							lhs_field |= flag;
						}
					}
				}
				if constexpr (nothrow) {
					_skipfields[i] = rhs_field;
				}
			}
		}
		return *this;
	}

protected:
	friend class hive<T>;

	ssize_t                                                 global_index_of_first{};
	uint64                                                  _skipfields[hive_page_num_skipfields<T>]{};
	std::array<hive_storage<T>, hive_page_num_elements<T>> data{};
};

template <typename T>
class hive_page : public hive_page_base<T> {
public:
	constexpr ssize_t find_last() const noexcept {
		for (auto i = hive_page_num_skipfields<T> - 1; i > 0; --i) {
			auto count = std::countr_zero(this->_skipfields[i]);
			if (count < 64) {
				return (i << 6) + count;
			}
		}
		return -1;
	}

	constexpr bool has(ssize_t index) const noexcept {
		return (this->_skipfields[index >> 6] & (1_u64 << (index & 63))) != 0;
	}

	constexpr void erase(ssize_t index) noexcept {
		SHION_ASSERT(has(index));

		this->data[index].destroy();
		this->_skipfields[index >> 6] &= ~(1_u64 << (index & 63));
	}

	template <typename... Args>
	constexpr ssize_t try_emplace(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) {
		for (auto i = 0; i < hive_page_num_skipfields<T>; ++i) {
			auto count = std::countr_one(this->_skipfields[i]);
			if (count < 64) {
				ssize_t index = (i << 6) | count;
				this->data[index].emplace(std::forward<Args>(args)...);
				this->_skipfields[i] |= (1_u64 << count);
				return index;
			}
		}
		return -1;
	}

	template <typename... Args>
	void emplace_at(std::size_t index, Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) {
		auto& skipfield = this->_skipfields[index >> 6];
		auto flag = (1_u64 << (index & 63));

		SHION_ASSERT((skipfield & flag) == 0);
		this->data[index].emplace(std::forward<Args>(args)...);
		skipfield |= flag;
	}

	constexpr ssize_t find_at_least(ssize_t idx) const noexcept {
		ssize_t skipfield_idx = idx >> 6;
		auto count = std::countr_zero(this->_skipfields[skipfield_idx] & (~0_u64 << (idx & 63)));
		if (count < 64)
			return (skipfield_idx << 6) | count;
		++skipfield_idx;
		while (skipfield_idx < hive_page_num_skipfields<T>) {
			count = std::countr_zero(this->_skipfields[skipfield_idx]);
			if (count < 64)
				return (skipfield_idx << 6) | count;
			++skipfield_idx;
		}
		return -1;
	}

	constexpr T& retrieve(ssize_t index) noexcept {
		SHION_ASSERT(has(index));

		return this->data[index].value;
	}

	constexpr T const& retrieve(ssize_t index) const noexcept {
		SHION_ASSERT(has(index));

		return this->data[index].value;
	}

	constexpr ssize_t get_from_address(std::add_const_t<T>* element) const noexcept {
		intptr_t addr = reinterpret_cast<intptr_t>(element);
		intptr_t start = reinterpret_cast<intptr_t>(this->data.data());
		intptr_t end = reinterpret_cast<intptr_t>(this->data.data() + this->data.size());

		if (addr < start || addr >= end)
			return -1;

		ssize_t hint = (addr - start) / sizeof(storage<T>);

		if (has(hint) && std::addressof(this->data[hint].value) == element) {
			return hint;
		}
		return -1;
	}
};

}

template <typename T>
class hive {
	using page_iterator = typename std::list<detail::hive_page<T>>::iterator;
	using page_riterator = typename std::list<detail::hive_page<T>>::reverse_iterator;

public:
	hive() = default;
	hive(hive const& rhs) requires (std::is_copy_constructible_v<T>) = default;
	hive(hive&& rhs) requires (std::is_move_constructible_v<T>) = default;
	~hive() = default;

	hive& operator=(hive const& rhs) requires (is_copyable<T>) = default;
	hive& operator=(hive&& rhs) requires (is_moveable<T>) = default;

	template <value_type ValueType>
	class basic_iterator {
	public:
		constexpr basic_iterator() = default;

		constexpr friend bool operator==(basic_iterator const& lhs, basic_iterator const& rhs) noexcept {
			if (rhs._hive == nullptr) {
				return lhs._hive == nullptr;
			}
			if (lhs._hive == nullptr) {
				return false;
			}
			return lhs._index == rhs._index;
		}

		constexpr basic_iterator operator++(int) const noexcept {
			SHION_ASSERT(_is_valid_hive_iterator());

			return _find_at_least(_hive, _current_page_it, _index + 1);
		}

		constexpr basic_iterator& operator++() noexcept {
			SHION_ASSERT(_is_valid_hive_iterator());

			*this = _find_at_least(_hive, _current_page_it, _index + 1);
			return {*this};
		}

		constexpr decltype(auto) operator*() const noexcept {
			return forward_like_type<ValueType>(_current_page_it->retrieve(_index - _current_page_it->global_index_of_first));
		}

		constexpr auto operator->() const noexcept {
			return std::addressof(operator*());
		}

		constexpr basic_iterator erase() const noexcept(std::is_nothrow_destructible_v<T>) {
			SHION_ASSERT(_is_valid_hive_iterator());

			return _hive->erase(*this);
		}

		constexpr ssize_t raw_index() const noexcept {
			return _index;
		}

	private:
		friend class hive;

		constexpr static basic_iterator _find_at_least(hive *h, page_iterator page_start, ssize_t index) noexcept {
			if (index > h->_last) {
				return {};
			}
			page_iterator         page_it = page_start;

			while (true) {
				SHION_ASSERT(page_it != h->_pages.end()); // Should have been caught by `_index > h->_last` -- `h->_last` is wrong -- race condition?
				ssize_t idx_in_page = index - page_it->global_index_of_first;
				if (idx_in_page < detail::hive_page_num_elements<T>) {
					if (idx_in_page < 0) {
						idx_in_page = 0;
					}
					idx_in_page = page_it->find_at_least(idx_in_page);
					if (idx_in_page >= 0) {
						return {h, page_it, idx_in_page + page_it->global_index_of_first};
					}
				}
				++page_it;
			}
			return {};
		}

		constexpr bool _is_valid_hive_iterator() const noexcept {
			return _hive && _index >= 0;
		}

		constexpr basic_iterator(std::nullptr_t) = delete;

		constexpr basic_iterator(hive* h) noexcept :
			basic_iterator(_find_at_least(h, h->_pages.begin(), 0)) {
		}

		constexpr basic_iterator(hive* h, page_iterator l, ssize_t i) noexcept :
			_index{i},
			_hive{h},
			_current_page_it{l} {
		}

		ssize_t       _index{-1};
		hive*         _hive{nullptr};
		page_iterator _current_page_it{};
	};

	using iterator = basic_iterator<value_type::lvalue_reference>;
	using const_iterator = basic_iterator<value_type::const_lvalue_reference>;

	constexpr auto begin() & noexcept -> basic_iterator<value_type::lvalue_reference> {
		return size() == 0 ? end() : basic_iterator<value_type::lvalue_reference>{this};
	}

	constexpr auto end() & noexcept -> basic_iterator<value_type::lvalue_reference> {
		return {};
	}

	constexpr auto begin() const& noexcept -> basic_iterator<value_type::const_lvalue_reference> {
		return {this};
	}

	constexpr auto end() const& noexcept -> basic_iterator<value_type::const_lvalue_reference> {
		return {};
	}

	constexpr auto begin() && noexcept -> basic_iterator<value_type::rvalue_reference> {
		return {this};
	}

	constexpr auto end() && noexcept -> basic_iterator<value_type::rvalue_reference> {
		return {};
	}

	constexpr auto begin() const&& noexcept -> basic_iterator<value_type::const_rvalue_reference> {
		return {this};
	}

	constexpr auto end() const&& noexcept -> basic_iterator<value_type::const_rvalue_reference> {
		return {};
	}

	template <typename... Args>
	requires (std::constructible_from<T, Args...>)
	constexpr iterator emplace(Args&&... args) {
		iterator result{};
		page_iterator                          page_it;
		ssize_t first_in_page = 0;

		for (page_it = _pages.begin(); page_it != _pages.end(); ++page_it) {
			first_in_page = page_it->global_index_of_first;
			if (auto idx = page_it->try_emplace(std::forward<Args>(args)...); idx >= 0) {
				auto global_idx = first_in_page + idx;
				result = {this, page_it, global_idx};
				++_size;
				if (global_idx > _last)
					_last = global_idx;
				return result;
			}
			first_in_page += detail::hive_page_num_elements<T>;
		}

		page_it = _pages.emplace(_pages.end());
		page_it->global_index_of_first = first_in_page;
		page_it->emplace_at(0, std::forward<Args>(args)...);
		result = {this, page_it, first_in_page};
		++_size;
		_last = first_in_page;
		return result;
	}

	template <typename... Args>
	requires (std::constructible_from<T, Args...>)
	constexpr std::pair<iterator, bool> try_emplace(ssize_t idx, Args&&... args) {
		SHION_ASSERT(idx >= 0);

		auto it = _pages.begin();
		ssize_t page_end = 0;
		ssize_t index_in_page = idx;
		while (it != _pages.end()) {
			if (idx < it->global_index_of_first) {
				// idx is after previous page, before this page
				break;
			}
			page_end = it->global_index_of_first + detail::hive_page_num_elements<T>;

			if (idx >= page_end) {
				++it;
				continue;
			}
			// idx is in this page
			index_in_page = idx - it->global_index_of_first;
			if (it->has(index_in_page)) {
				return {
					iterator{this, it, idx},
					false
				};
			} else {
				it->emplace_at(index_in_page, std::forward<Args>(args)...);
				++_size;
				if (idx > _last)
					_last = idx;
				return {
					iterator{this, it, idx},
					true
				};
			}
		}
		if (idx >= page_end + detail::hive_page_num_elements<T>) {
				index_in_page = idx % detail::hive_page_num_elements<T>;
				page_end = idx - index_in_page;
		}
		it = _pages.emplace(it);
		it->global_index_of_first = page_end;
		it->emplace_at(index_in_page, std::forward<Args>(args)...);
		++_size;
		if (idx > _last)
			_last = idx;
		return {
			iterator{this, it, idx},
			true
		};
	}

	constexpr iterator get_iterator(std::add_const_t<T>* element) noexcept {
		// We cast all pointers to intptr_t because comparing incompatible pointers is undefined behavior
		intptr_t as_int = reinterpret_cast<intptr_t>(element);

		for (page_iterator page_it = _pages.begin(); page_it != _pages.end(); ++page_it) {
			ssize_t index_of_page = page_it->global_index_of_first;
			intptr_t begin_as_int = reinterpret_cast<intptr_t>(&(*page_it->data.begin()));
			intptr_t end_as_int = reinterpret_cast<intptr_t>(&(*page_it->data.rbegin())) + 1;

			if (as_int >= begin_as_int && as_int < end_as_int) {
				if (auto idx = page_it->get_from_address(element); idx >= 0) {
					return {this, page_it, page_it->global_index_of_first + idx};
				}
			}
		}
		return {};
	}

	constexpr iterator at_raw_index(ssize_t index) noexcept {
		if (size() == 0 || index > _last)
			return end();

		auto page_it = _get_page(index);
		if (page_it == _pages.end() || !page_it->has(index - page_it->global_index_of_first)) {
			return end();
		}
		return {this, page_it, index};
	}

	constexpr auto erase(basic_iterator<value_type::lvalue_reference> it) noexcept(std::is_nothrow_destructible_v<T>) -> basic_iterator<value_type::lvalue_reference> {
		return _erase(it);
	}

	constexpr auto erase(basic_iterator<value_type::rvalue_reference> it) noexcept(std::is_nothrow_destructible_v<T>) -> basic_iterator<value_type::rvalue_reference> {
		return _erase(it);
	}

	constexpr void clear() noexcept(std::is_nothrow_destructible_v<T>) {
		_pages.clear();
	}

	constexpr ssize_t capacity() const noexcept {
		return shion::lossless_cast<ssize_t>(detail::hive_page_num_elements<T> * _pages.size());
	}

	constexpr ssize_t size() const noexcept {
		return _size;
	}

	constexpr ssize_t size_bytes() const noexcept {
		return pages() * page_size_bytes();
	}

	constexpr ssize_t last_raw_index() const noexcept {
		return _last;
	}

	constexpr ssize_t pages() const noexcept {
		return _pages.size();
	}

	constexpr ssize_t page_size_bytes() const noexcept {
		return sizeof(detail::hive_page<T>);
	}

	constexpr ssize_t page_capacity() const noexcept {
		return detail::hive_page_num_elements<T>;
	}

private:
	constexpr auto _get_page(ssize_t idx) const noexcept {
		auto page_it = _pages.begin();
		while (page_it != _pages.end()) {
			if (idx - page_it->global_index_of_first < detail::hive_page_num_elements<T>) {
				return page_it;
			}
			++page_it;
		}
		return _pages.end();
	}

	constexpr auto _get_page(ssize_t idx) noexcept {
		auto page_it = _pages.begin();
		while (page_it != _pages.end()) {
			if (idx - page_it->global_index_of_first < detail::hive_page_num_elements<T>) {
				return page_it;
			}
			++page_it;
		}
		return _pages.end();
	}

	template <value_type ValueType>
	basic_iterator<ValueType> _erase(basic_iterator<ValueType> it) noexcept(std::is_nothrow_destructible_v<T>) {
		SHION_ASSERT(it._is_valid_hive_iterator());

		page_riterator         owning_page = page_riterator{std::next(it._current_page_it)};

		owning_page->erase(it._index - owning_page->global_index_of_first);
		if (--_size == 0) {
			_last = -1;
			return {};
		}

		if (it._index != _last) {
			return it++;
		}

		while (owning_page != _pages.rend()) {
			if (auto last = owning_page->find_last(); last >= 0) {
				_last = last + owning_page->global_index_of_first;
				return {};
			}
			++owning_page;
		}
		_last = -1;
		return {};
	}

	ssize_t         _size{0};
	ssize_t         _last{-1};
	std::list<detail::hive_page<T>> _pages{};
};

}

#endif /* SHION_CONTAINERS_HIVE_H_ */
