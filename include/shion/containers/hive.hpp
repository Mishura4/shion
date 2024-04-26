#ifndef SHION_CONTAINERS_HIVE_H_
#define SHION_CONTAINERS_HIVE_H_

#include <type_traits>
#include <bit>

#include "../shion_essentials.hpp"
#include "../utility/optional.hpp"

namespace shion {

template <typename T>
struct hive_emplace_result {
	constexpr explicit operator bool() const noexcept {
		return inserted;
	}

	T*      inserted{nullptr};
	ssize_t index{};
};


namespace detail {

inline constexpr bool valid_hive_block_idx(ssize_t idx) noexcept {
	return (idx & ~ssize_t{63}) == 0;
}

template <typename T>
class hive_block {
public:
	template<value_type ValueType>
	class iterator {
	public:
		constexpr friend bool operator==(iterator const& lhs, iterator const& rhs) noexcept {
			if (rhs._array == nullptr) {
				return lhs._array == nullptr;
			}
			if (lhs._array == nullptr) {
				return false;
			}
			return lhs._index == rhs._index;
		}

		constexpr iterator operator++(int) const noexcept {
			for (ssize_t idx = _index; idx < 63;) {
				++idx;
				if (_presence_flags & (1_u64 << idx)) {
					return {_array, _presence_flags, idx};
				}
			}
			return {nullptr, 0, 64};
		}

		constexpr iterator& operator++(int) noexcept {
			while (_index < 63) {
				++_index;
				if (_presence_flags & (1_u64 << _index)) {
					return {_array, _presence_flags, _index};
				}
			}
			return {nullptr, 0, 64};
		}

		constexpr decltype(auto) operator*() const noexcept {
			SHION_ASSERT(detail::valid_hive_block_idx(_index) && (_presence_flags & (1_u64 << _index)) == 0);

			return forward_like_type<ValueType>(_array[_index].get());
		}

	private:
		storage<T>*_array{nullptr};
		uint64     _presence_flags{0};
		ssize_t    _index{0};
	};

	constexpr hive_block() noexcept = default;

	constexpr hive_block(const hive_block &rhs) noexcept (std::is_nothrow_copy_constructible_v<T>)
	requires (std::is_copy_constructible_v<T>) {
		this->_copy(rhs, std::make_index_sequence<64>());
		if constexpr (std::is_nothrow_copy_constructible_v<T>)
			_presence_flags = rhs._presence_flags;
	}

	constexpr hive_block(hive_block &&rhs) noexcept (std::is_nothrow_move_constructible_v<T>)
	requires (std::is_move_constructible_v<T>) {
		this->_move(std::move(rhs), std::make_index_sequence<64>());
		if constexpr (std::is_nothrow_move_constructible_v<T>)
			_presence_flags = rhs._presence_flags;
	}

	constexpr ~hive_block() {
		this->_destroy(std::make_index_sequence<64>());
	}

	constexpr auto begin() & noexcept -> iterator<value_type::lvalue_reference> {
		return {_storage, _presence_flags, 0};
	}

	constexpr auto end() & noexcept -> iterator<value_type::lvalue_reference> {
		return {nullptr, 0, 0};
	}

	constexpr auto begin() const& noexcept -> iterator<value_type::const_lvalue_reference> {
		return {_storage, _presence_flags, 0};
	}

	constexpr auto end() const& noexcept -> iterator<value_type::const_lvalue_reference> {
		return {nullptr, 0, 0};
	}

	constexpr auto begin() && noexcept -> iterator<value_type::rvalue_reference> {
		return {_storage, _presence_flags, 0};
	}

	constexpr auto end() && noexcept -> iterator<value_type::rvalue_reference> {
		return {nullptr, 0, 0};
	}

	constexpr auto begin() const&& noexcept -> iterator<value_type::const_rvalue_reference> {
		return {_storage, _presence_flags, 0};
	}

	constexpr auto end() const&& noexcept -> iterator<value_type::const_rvalue_reference> {
		return {nullptr, 0, 0};
	}

	constexpr bool has(ssize_t idx) const noexcept {
		SHION_ASSERT(detail::valid_hive_block_idx(idx));

		return (_presence_flags & (1_u64 << idx)) != 0;
	}

	constexpr hive_block& operator=(const hive_block &rhs) noexcept (std::is_nothrow_copy_constructible_v<T>)
	requires (std::is_copy_constructible_v<T>) {
		this->_destroy(std::make_index_sequence<64>());
		this->_copy(rhs, std::make_index_sequence<64>());
		if constexpr (std::is_nothrow_copy_constructible_v<T>)
			_presence_flags = rhs._presence_flags;
	}

	constexpr hive_block& operator=(hive_block &&rhs) noexcept (std::is_nothrow_move_constructible_v<T>)
	requires (std::is_move_constructible_v<T>) {
		this->_destroy(std::make_index_sequence<64>());
		this->_move(std::move(rhs), std::make_index_sequence<64>());
		if constexpr (std::is_nothrow_move_constructible_v<T>)
			_presence_flags = rhs._presence_flags;
	}

	constexpr optional<std::add_lvalue_reference_t<T>> operator[](ssize_t index) noexcept {
		if (has(index)) {
			return std::nullopt;
		}
		return optional<std::add_lvalue_reference_t<T>>{_storage[index]._data};
	}

	constexpr auto retrieve(ssize_t index) noexcept -> decltype(auto) {
		SHION_ASSERT(has(index));

		return _storage[index].get();
	}

	template <typename... Args>
	constexpr auto emplace(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) -> hive_emplace_result<std::remove_reference_t<std::add_pointer_t<T>>>
	requires (std::is_constructible_v<T, Args...>) {
		size_t count = std::countr_one(_presence_flags);

		if (count >= 64) {
			return {nullptr, std::numeric_limits<ssize_t>::max()};
		}
		return {std::addressof(emplace_at(count, std::forward<Args>(args)...)), count};
	}

	template <typename... Args>
	constexpr auto emplace_at(ssize_t idx, Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) -> decltype(auto)
	requires (std::is_constructible_v<T, Args...>) {
		SHION_ASSERT(!has(idx));

		_storage[idx].emplace(std::forward<Args&&>(args)...);
		_presence_flags |= (1_u64 << idx);
		return _storage[idx].get();
	}

	constexpr void erase(ssize_t idx) noexcept {
		SHION_ASSERT(has(idx));

		_storage[idx].destroy();
		_presence_flags &= ~(1_u64 << idx);
	}

	constexpr ssize_t first_free() const noexcept {
		return std::countr_one(_presence_flags);
	}

	constexpr ssize_t first_present() const noexcept {
		return std::countr_zero(_presence_flags);
	}

	constexpr ssize_t size() const noexcept {
		return std::popcount(_presence_flags);
	}

	constexpr uint64 presence() const noexcept {
		return _presence_flags;
	}

private:
	template <size_t... Ns>
	constexpr void _destroy(std::index_sequence<Ns...>) {
#ifdef __cpp_lib_is_implicit_lifetime
		if constexpr (std::is_implicit_lifetime<T>) {
			return;
		} else
#else
		{
			constexpr auto fun = []<size_t N>(uint64 flags, storage<T>* array) constexpr noexcept {
				if (flags & (1_u64 << N)) {
					array[N].destroy();
				}
			};
			(fun.template operator()<Ns>(_presence_flags, _storage), ...);
		}
#endif
	}

	template <size_t... Ns>
	constexpr void _copy(const hive_block &rhs, std::index_sequence<Ns...>) noexcept (std::is_nothrow_copy_constructible_v<T>)
	requires (std::is_copy_constructible_v<T>) {
		constexpr auto fun = []<size_t N>(uint64 flags, hive_block* me, storage<T> *other) constexpr noexcept (std::is_nothrow_copy_constructible_v<T>) {
			if (flags & (1_u64 << N)) {
				me->_storage[N].emplace(other[N].get());
				if constexpr (!std::is_nothrow_copy_constructible_v<T>)
					me->_presence_flags |= (1_u64 << N);
			}
		};

		(fun.template operator()<Ns>(_presence_flags, this, rhs._storage), ...);
	}

	template <size_t... Ns>
	constexpr void _move(hive_block &rhs, std::index_sequence<Ns...>) noexcept (std::is_nothrow_move_constructible_v<T>)
	requires (std::is_move_constructible_v<T>) {
		constexpr auto fun = []<size_t N>(uint64 flags, hive_block* me, storage<T> *other) constexpr noexcept (std::is_nothrow_move_constructible_v<T>) {
			if (flags & (1_u64 << N)) {
				me->_storage[N].emplace(std::move(other[N]).get());
				if constexpr (!std::is_nothrow_move_constructible_v<T>)
					me->_presence_flags |= (1_u64 << N);
			}
		};

		(fun.template operator()<Ns>(_presence_flags, this, rhs._storage), ...);
	}

	uint64             _presence_flags{0};
	detail::storage<T> _storage[64];
};

template <typename T>
inline constexpr size_t hive_block_size = sizeof(hive_block<T>);

template <typename T>
inline constexpr size_t hive_blocks_per_page = sizeof(hive_block<T>) * 32;

template <typename T>
requires (sizeof(T) * 32 < 63 * 1024)
inline constexpr size_t hive_blocks_per_page<T> = ((63 * 1024) / hive_block_size<T>);

}

template <typename T>
class hive {
	inline constexpr static size_t blocks_per_page = detail::hive_blocks_per_page<T>;
	inline constexpr static ssize_t sblocks_per_page = shion::lossless_cast<ssize_t>(detail::hive_blocks_per_page<T>);

	struct page {
		ssize_t                                            global_index_of_first{};
		std::array<detail::hive_block<T>, blocks_per_page> data{};
	};

	using page_iterator = typename std::list<page>::iterator;
	using page_riterator = typename std::list<page>::reverse_iterator;

public:
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
			return forward_like_type<ValueType>(_current_page_it->data[_index >> 6].retrieve(_index & 63));
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
			detail::hive_block<T>*current_block;
			page_iterator         page_it = page_start;
			ssize_t               idx_in_page = index - page_it->global_index_of_first;

			while (page_it != h->_pages.end()) {
				while (idx_in_page < sblocks_per_page * 64) {
					current_block = &page_it->data[idx_in_page >> 6];
					auto to_end_of_block = 64 - (idx_in_page & 63);
					auto to_next_one = std::countr_zero(current_block->presence() >> (index & 63));

					if (to_next_one >= to_end_of_block) {
						index += to_end_of_block;
						idx_in_page += to_end_of_block;
					} else {
						return {h, page_it, index + to_next_one};
					}
				}
				++page_it;
			}
			SHION_ASSERT(false); // Should have been caught by `_index == _hive->_last` -- `_hive->_last` is wrong -- race condition?
			return {};
		}

		constexpr bool _is_valid_hive_iterator() const noexcept {
			return _hive && _index >= 0;
		}

		constexpr basic_iterator(nullptr_t) = delete;

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

	constexpr hive() = default;

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
		ssize_t                                block_idx;
		ssize_t                                free_idx;
		ssize_t                                first_in_page = 0;
		page_iterator                          page_it;

		for (page_it = _pages.begin(); page_it != _pages.end(); ++page_it) {
			for (block_idx = 0; block_idx < sblocks_per_page; ++block_idx) {
				detail::hive_block<T>& block = page_it->data[block_idx];
				if (free_idx = block.first_free(); detail::valid_hive_block_idx(free_idx)) {
					block.emplace_at(free_idx, std::forward<Args>(args)...);
					ssize_t global_idx = first_in_page + (block_idx << 6) | free_idx;
					result = {this, _pages.begin(), global_idx};
					++_size;
					if (global_idx > _last)
						_last = global_idx;
					return result;
				}
			}
			first_in_page += sblocks_per_page << 6;
		}

		page_it = _pages.emplace(_pages.end(), first_in_page);
		page_it->data[0].emplace_at(0, std::forward<Args>(args)...);
		result = {this, page_it, first_in_page};
		++_size;
		_last = first_in_page;
		return result;
	}

	constexpr iterator get_iterator(std::add_const_t<T>* element) noexcept {
		// We cast all pointers to intptr_t because comparing invalid pointers is undefined behavior
		intptr_t as_int = reinterpret_cast<intptr_t>(element);

		for (page_iterator page_it = _pages.begin(); page_it != _pages.end(); ++page_it) {
			ssize_t index_of_page = page_it->global_index_of_first;
			intptr_t begin_as_int = reinterpret_cast<intptr_t>(&(*page_it->data.begin()));
			intptr_t end_as_int = reinterpret_cast<intptr_t>(&(*page_it->data.rbegin())) + 1;

			if (as_int >= begin_as_int && as_int < end_as_int) {
				ssize_t hint = (as_int - begin_as_int - sizeof(page::global_index_of_first));
				ssize_t block_idx = hint / sizeof(detail::hive_block<T>);
				detail::hive_block<T>* block = &page_it->data[block_idx];
				ssize_t block_addr = reinterpret_cast<intptr_t>(block);
				ssize_t elem_in_block_idx = ((as_int - block_addr - sizeof(block->presence())) / sizeof(T)) & 63;

				if (block->has(elem_in_block_idx) && &block->retrieve(elem_in_block_idx) == element) {
					return {this, page_it, index_of_page + (block_idx << 6) | elem_in_block_idx};
				}
			}
		}
		return {};
	}

	constexpr iterator at_raw_index(ssize_t index) noexcept {
		if (size() == 0 || index > _last)
			return end();

		auto page_it = _pages.begin();
		while (index > sblocks_per_page << 6) {
			index -= sblocks_per_page;
			++page_it;
		}

		SHION_ASSERT(page_it != _pages.end());
		auto &block = page_it->data[index >> 6];
		if (!block.has(index & 63)) {
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

	constexpr ssize_t capacity() const noexcept {
		return shion::lossless_cast<ssize_t>((blocks_per_page * _pages.size()) * 64);
	}

	constexpr ssize_t size() const noexcept {
		return _size;
	}

	constexpr ssize_t last_raw_index() const noexcept {
		return _last;
	}

	constexpr ssize_t page_size_bytes() const noexcept {
		return sizeof(page);
	}

	constexpr ssize_t page_capacity() const noexcept {
		return sblocks_per_page * 64;
	}

private:
	template <value_type ValueType>
	basic_iterator<ValueType> _erase(basic_iterator<ValueType> it) noexcept(std::is_nothrow_destructible_v<T>) {
		SHION_ASSERT(it._is_valid_hive_iterator());

		page_riterator         owning_page = page_riterator{std::next(it._current_page_it)};
		ssize_t                index_in_page = it._index - owning_page->global_index_of_first;
		ssize_t                block_idx = index_in_page >> 6;
		detail::hive_block<T>* block = &owning_page->data[block_idx];

		block->erase(index_in_page & 63);
		if (--_size == 0) {
			_last = -1;
			return {};
		}

		if (it._index != _last) {
			return it++;
		}

		while (owning_page != _pages.rend()) {
			while (block_idx >= 0) {
				block = &owning_page->data[block_idx];
				ssize_t present_in_block = block->first_present();
				if (detail::valid_hive_block_idx(present_in_block)) {
					_last = owning_page->global_index_of_first + (block_idx << 6) + present_in_block;
					return it++;
				}
				--block_idx;
			}
			++owning_page;
		}
		_last = -1;
		return {};
	}

	ssize_t         _size{0};
	ssize_t         _last{-1};
	std::list<page> _pages;
};

/* -- One of these days I'll write real tests

		using hive = shion::hive<std::unique_ptr<int>>;
		hive h;

		auto first = h.emplace(std::make_unique<int>(5));
		assert(**first == 5);

		auto second = h.emplace(std::make_unique<int>(42));
		assert(**second == 42);

		assert(h.at_raw_index(0) == first);
		assert(h.at_raw_index(1) == second);
		assert(h.erase(second) == h.end());
		assert(h.at_raw_index(1) == h.end());

		second = h.emplace(std::make_unique<int>(69));
		assert(**second == 69);
		assert(h.at_raw_index(1) == second);

		assert(h.erase(first) == second);
		assert(h.at_raw_index(0) == h.end());
		assert(h.at_raw_index(1) == second);
		first = h.emplace(std::make_unique<int>(42069));
		assert(first == h.begin());
		assert(h.at_raw_index(0) == first);
		assert(h.at_raw_index(1) == second);
		assert(h.erase(h.erase(first)) == h.end());
		assert(h.begin() == h.end());
		assert(h.at_raw_index(0) == h.end());
		assert(h.at_raw_index(1) == h.end());

		int i;
		for (i = 0; i < 69; ++i) {
			auto it = h.emplace(std::make_unique<int>(i));
		}
		for (auto it = h.begin(); it != h.end();) {
			it = it.erase();
		}
		assert(h.begin() == h.end() && h.size() == 0);

		for (i = 0; i < 420; ++i) {
			auto it = h.emplace(std::make_unique<int>(i));
		}
		for (auto it = h.begin(); it != h.end();) {
			it = it.erase();
		}
		assert(h.begin() == h.end() && h.size() == 0);

		for (i = 0; i < 420; ++i) {
			auto it = h.emplace(std::make_unique<int>(i));
		}
		hive::iterator at_63;
		hive::iterator at_128;
		i = 0;
		for (auto it = h.begin(); it != h.end();) {
			if (i == 63) {
				at_63 = it;
				++it;
			} else if (i > 63 && i < 128) {
				it = it.erase();
			} else if (i == 128) {
				at_128 = it;
				++it;
			}
			else
				++it;
			++i;
		}
		auto bad = std::unique_ptr<int>{};
		assert(at_63++ == at_128);
		assert(h.get_iterator(&(*at_63)) == at_63);
		assert(h.get_iterator(&(*at_128)) == at_128);
		assert(h.get_iterator(&bad) == h.end());

		auto size = h.size();
		for (i = 0; i < size; ++i) {
			h.erase(h.begin());
		}
		assert(h.begin() == h.end() && h.size() == 0);
*/

}

#endif /* SHION_CONTAINERS_HIVE_H_ */
