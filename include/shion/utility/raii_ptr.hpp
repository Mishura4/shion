#ifndef SHION_RAII_PTR_H_
#define SHION_RAII_PTR_H_

#include <utility>

#include "../shion_essentials.hpp"

namespace shion {

template <typename Self, typename Ptr>
class raii_ptr {
public:
	constexpr raii_ptr() = default;
	explicit constexpr raii_ptr(Ptr* ptr) noexcept : _ptr{ptr} {}
	constexpr raii_ptr(raii_ptr const&) = delete;
	constexpr raii_ptr(raii_ptr&& other) noexcept : _ptr{std::exchange(other._ptr, nullptr)} {}
	constexpr ~raii_ptr() {
		this->_cleanup();
	}

	constexpr raii_ptr& operator=(raii_ptr const&) = delete;
	constexpr raii_ptr& operator=(raii_ptr&& other) noexcept {
		this->_cleanup();
		_ptr = std::exchange(other._ptr, nullptr);
		return *this;
	}

	constexpr void swap(raii_ptr& other) noexcept {
		_ptr = std::exchange(other._ptr, _ptr);
	}

	constexpr void reset(Ptr* ptr) noexcept {
		this->_cleanup();
		_ptr = ptr;
	}

	constexpr Ptr* release() noexcept {
		return std::exchange(_ptr, nullptr);
	}

	constexpr Ptr* get() noexcept {
		return _ptr;
	}

	constexpr explicit operator bool() const noexcept {
		return _ptr;
	}

	constexpr auto operator<=>(const raii_ptr& other) const = default;
	constexpr bool operator==(const raii_ptr& other) const = default;

private:
	void _cleanup() noexcept {
		static_cast<Self*>(this)->_delete();
	}

	Ptr *_ptr = nullptr;
};

}

#endif /* SHION_RAII_PTR_H_ */
