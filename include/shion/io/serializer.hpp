#ifndef SHION_IO_SERIALIZER_H_
#define SHION_IO_SERIALIZER_H_

#include <shion/common/defines.hpp>

#if !SHION_BUILDING_MODULES
#  include <shion/utility/tuple.hpp>
#  include <shion/common/common.hpp>

#  include <tuple>
#  include <ranges>
#  include <concepts>
#endif

namespace SHION_NAMESPACE
{

inline namespace io
{

struct unknown_buffer_type
{
};

template <typename T>
inline constexpr auto buffer_size = unknown_buffer_type{};

template <std::ranges::sized_range T>
inline constexpr auto buffer_size<T> = [](const T& buffer) {
	return std::ranges::size(buffer);
};

// size() member function
template <typename T>
requires requires (T buffer) { { buffer.size() } -> std::convertible_to<size_t>;  }
inline constexpr auto buffer_size<T> = [](const T& buffer) {
	using size_type = decltype(buffer.size());

	if constexpr (std::is_signed_v<size_type>)
	{
		return (std::max<size_type>)(0, buffer.size());
	}
	else
	{
		return buffer.size();
	}
};


// size(meow) ADL function
template <typename T>
requires requires (T buffer) { { size(buffer) } -> std::convertible_to<size_t>; }
inline constexpr auto buffer_size<T> = [](const T& buffer) {
	// size(meow) ADL function
	using size_type = decltype(size(buffer));

	if constexpr (std::is_signed_v<size_type>)
	{
		return (std::max<size_type>)(0, size(buffer));
	}
	else
	{
		return size(buffer);
	}
};

template <typename T>
inline constexpr auto buffer_capacity = unknown_buffer_type{};

// capacity() member function
template <typename T>
requires requires (T buffer) { { buffer.capacity() } -> std::convertible_to<size_t>;  }
inline constexpr auto buffer_capacity<T> = [](const T& buffer) constexpr noexcept {
	using size_type = decltype(buffer.capacity());

	if constexpr (std::is_signed_v<size_type>)
	{
		return (std::max<size_type>)(0, buffer.capacity());
	}
	else
	{
		return buffer.capacity();
	}
};

// capacity(meow) ADL function
template <typename T>
requires requires (T buffer) { { capacity(buffer) } -> std::convertible_to<size_t>; }
inline constexpr auto buffer_capacity<T> = [](const T& buffer) constexpr noexcept {
	using size_type = decltype(capacity(buffer));

	if constexpr (std::is_signed_v<size_type>)
	{
		return (std::max<size_type>)(0, capacity(buffer));
	}
	else
	{
		return capacity(buffer);
	}
};

// byte[N]
template <storage_byte T, size_t N>
inline constexpr auto buffer_capacity<T[N]> = [](const T (&)[N]) constexpr noexcept {
	return N;
};

template <typename T>
concept sized_buffer = storage_buffer<T> && requires { { buffer_capacity<T>(std::declval<T&>()) } -> std::convertible_to<size_t>; };

template <typename T>
concept expandable_buffer = storage_buffer<T> && (
								requires (T t, size_t n) { t.resize(n); }
								|| requires (T t, std::ranges::range_value_t<T> v) { t.push_back(std::move(v)); }
							);
}

inline namespace io
{

SHION_EXPORT template <typename Buffer, typename Tag = void>
class serializer;

/**
 * @brief Helper for serializing a type into a buffer.
 * The helper can implement `read` and `write`, see the documentation for these member functions.
 *
 * @see read
 * @see write
 */
SHION_EXPORT template <typename T, typename Tag = void>
struct serializer_helper {
	/**
	 * @brief Extracts a value from a buffer and returns it in-place.
	 *
	 * @param bytes The current amount of bytes available.
	 * @param endian The endianness to read the data as.
	 *
	 * @return Returns the total amount of bytes we need.
	 * @retval If 0, the deserialization shall abort.
	 * @retval If > bytes.size(), the deserializer may obtain more bytes and call the function again, if possible.
	 * @retval If != 0 && <= bytes.size(), the behavior is undefined.
	 */
	constexpr auto construct(std::span<const std::byte> bytes, std::endian endian = std::endian::native) -> T = delete;

	/**
	 * @brief Extracts a value from a buffer.
	 *
	 * @param bytes The current amount of bytes available.
	 * @param value The value to fill. If nullptr, this is a dry run.
	 * @param endian The endianness to read the data as.
	 *
	 * @note This function can return a shion::generator<size_t> instead.
	 *
	 * @return Returns the total amount of bytes we need or have read.
	 * @retval If 0, the deserialization shall abort.
	 * @retval If == bytes.size(), write the value if non-null.
	 * @retval If > bytes.size(), the deserializer may obtain more bytes and call the function again, if possible.
	 * @retval If != 0 && <= bytes.size(), the behavior is undefined.
	 */
	constexpr auto read(std::span<const std::byte> bytes, T* value, std::endian endian = std::endian::native) -> size_t = delete;

	/**
	 * @brief Write a value to the buffer.
	 *
	 * @param bytes The current amount of bytes available.
	 * @param value The value to write.
	 * @param endian The endianness to write the data as.
	 *
	 * @return Returns the total amount of bytes we need.
	 * @retval If 0, the deserialization shall abort.
	 * @retval If > bytes.size(), the serializer may obtain more bytes and call the function again, if possible.
	 * @retval If != 0 && <= bytes.size(), the behavior is undefined.
	 */
	constexpr auto write(std::span<std::byte> bytes, const T& value, std::endian endian = std::endian::native) -> size_t = delete;
};

SHION_EXPORT template <typename T>
struct buffer_traits;

SHION_EXPORT template <storage_buffer T>
struct buffer_traits<T>
{
	constexpr static size_t capacity(const T& buffer) noexcept requires (sized_buffer<T>)
	{
		return buffer_capacity<T>(buffer);
	}
};

SHION_EXPORT struct reader
{
	template <typename T, size_t N>
	constexpr auto operator()(std::span<const std::byte, N> bytes, T* value) requires requires (serializer_helper<T> s) { s.read(bytes, value); }
	{
		return serializer_helper<T>{}.read(bytes, value);
	}

	template <typename T, size_t N>
	constexpr auto operator()(std::span<const std::byte, N> bytes, T* value, bool construct) requires requires (serializer_helper<T> s) { s.read(bytes, value, construct); }
	{
		return serializer_helper<T>{}.read(bytes, value, construct);
	}

	template <typename T, size_t N>
	constexpr auto operator()(std::span<const std::byte, N> bytes, T* value, std::endian endian, bool construct) requires requires (serializer_helper<T> s) { s.read(bytes, value, endian, construct); }
	{
		return serializer_helper<T>{}.read(bytes, value, endian, construct);
	}
};

SHION_EXPORT struct writer
{
	template <typename T, size_t N>
	constexpr auto operator()(std::span<std::byte, N> bytes, T* value) requires requires (serializer_helper<T> s) { s.read(bytes, value); }
	{
		return serializer_helper<T>{}.read(bytes, value);
	}

	template <typename T, size_t N>
	constexpr auto operator()(std::span<std::byte, N> bytes, T* value, std::endian endian) requires requires (serializer_helper<T> s) { s.read(bytes, value, endian); }
	{
		return serializer_helper<T>{}.read(bytes, value, endian);
	}
};

SHION_EXPORT inline constexpr auto read = reader{};
SHION_EXPORT inline constexpr auto write = writer{};

SHION_EXPORT template <typename T>
concept serializable = requires (std::span<std::byte> bytes, T val) { serializer_helper<T>{}.write(bytes, val); };

SHION_EXPORT template <typename T>
concept deserializable = requires (std::span<const std::byte> bytes, T val) { serializer_helper<T>{}.read(bytes, &val); };

SHION_EXPORT template <typename T>
concept deserialize_constructible = requires (std::span<const std::byte> bytes) { serializer_helper<T>{}.construct(bytes); };

}

namespace detail::serializer
{

template <typename>
struct scalar_serializer {};

template <typename T>
requires (std::is_scalar_v<T>)
struct scalar_serializer<T>
{
	constexpr T byteswap(T val [[maybe_unused]])
	{
		if constexpr (std::integral<T>)
		{
			val = std::byteswap(val);
		}
		else if constexpr (std::is_enum_v<T>)
		{
			auto underlying = std::to_underlying(val);
			val = static_cast<T>(std::byteswap(underlying));
		}
		else if constexpr (std::is_pointer_v<T>)
		{
			SHION_ASSERT(!std::is_pointer_v<T>);
		}
		else if constexpr (std::is_floating_point_v<T>)
		{
			// Don't
		}
		return val;
	}
	
private:
	constexpr auto _read(std::span<std::byte const> bytes, T* value) noexcept -> size_t
	{
		if (!std::is_constant_evaluated())
			std::memcpy(value, bytes.data(), sizeof(T));
		else
		{
			std::array<std::byte, sizeof(T)> arr;
			std::ranges::copy_n(bytes.data(), sizeof(T), arr.begin());
			*value = std::bit_cast<T>(arr);
		}
		return sizeof(T);
	}

	constexpr auto _write(std::span<std::byte> bytes, const T& value) noexcept -> size_t
	{
		if (!std::is_constant_evaluated())
			std::memcpy(bytes.data(), &value, sizeof(T));
		else
			std::ranges::copy(std::bit_cast<std::array<std::byte, sizeof(T)>>(value), bytes.begin());
		return sizeof(T);
	}

public:
	static inline constexpr bool shion_provided = true;
	static inline constexpr bool constant_size = true;

	constexpr auto construct(std::span<std::byte const>& bytes, std::endian endian = std::endian::native) noexcept -> T
	{
		SHION_ASSERT(bytes.size() >= sizeof(T));
		T t;
			
		if (!std::is_constant_evaluated())
			std::memcpy(&t, bytes.data(), sizeof(T));
		else
		{
			std::array<std::byte, sizeof(T)> arr;
			std::ranges::copy_n(bytes.data(), sizeof(T), arr.begin());
			t = std::bit_cast<T>(arr);
		}
		if constexpr (!std::is_floating_point_v<T>)
		{
			if (endian != std::endian::native)
				t = this->byteswap(t);
		}
		bytes = bytes.subspan(sizeof(T));
		return t;
	}

	constexpr auto read(std::span<std::byte const> bytes, T* value, std::endian endian = std::endian::native) noexcept -> size_t
	{
		constexpr size_t sz = sizeof(T);
		if (value == nullptr || bytes.size() < sz)
			return sz;

		if (value != nullptr)
		{
			this->_read(bytes, value);
			if (endian != std::endian::native)
				*value = this->byteswap(*value);
		}
		return sz;
	}

	constexpr auto write(std::span<std::byte> bytes, const T& value, std::endian endian = std::endian::native) noexcept -> size_t
	{
		constexpr size_t sz = sizeof(T);
		if (bytes.empty())
			return sz;

		if constexpr (sz > 1 && !std::floating_point<T>)
		{
			if (endian != std::endian::native)
			{
				return this->_write(bytes, this->byteswap(value));
			}
			
		}
		return this->_write(bytes, value);
	}
};

template <typename T, typename Tag, typename Ns>
struct tuple_serializer;

template <typename T, typename Tag, size_t... Ns>
struct tuple_serializer<T, Tag, std::index_sequence<Ns...>>
{
	template <size_t N>
	using proxy_t = serializer_helper<std::tuple_element_t<N, T>, Tag>;
	using proxies_t = tuple<proxy_t<Ns>...>;
	
	static inline constexpr bool shion_provided = true;
	template <size_t N>
	static inline constexpr bool constant_size_n = (requires { requires proxy_t<N>::constant_size; });
	static inline constexpr bool constant_size = (constant_size_n<Ns> && ...);

	proxies_t proxies;

	constexpr auto construct(std::span<const byte>& bytes, std::endian endian = std::endian::native) -> T
	requires (deserialize_constructible<typename tuple_element_selector<Ns, T>::type::type> && ...)
	{
		SHION_ASSERT(read(bytes, nullptr, endian) <= bytes.size());
		return T{ get<Ns>(proxies).construct(bytes, endian)... };
	}

	constexpr auto read(std::span<const byte> bytes, T* value, std::endian endian = std::endian::native) -> size_t
	requires (deserializable<typename tuple_element_selector<Ns, T>::type::type> && ...)
	{
		size_t sz = 0;
		auto validate = [&sz, endian, bytes, failed = false]<size_t N>(proxy_t<N>& proxy) mutable constexpr {
			size_t size;
			if constexpr (requires { requires proxy_t<N>::constant_size; })
			{
				(void)bytes;
				(void)failed;
				size = proxy.read({}, {}, endian);
				sz += size;
			}
			else
			{
				if (failed)
					return;

				size = proxy.read(bytes.subspan(sz), {}, endian);
				sz += size;
				failed = sz > bytes.size();
			}
			return size;
		};
		size_t allSizes[sizeof...(Ns)] [[maybe_unused]] = { validate.template operator()<Ns>(get<Ns>(proxies))... };

		if (sz > bytes.size() || !value)
			return sz;

		auto impl = [this, endian, bytes, value, &allSizes, idx = size_t{}]<size_t N>(proxy_t<N>& proxy) mutable constexpr {
			size_t size = proxy.read(bytes.subspan(idx), &get<N>(*value), endian);
			SHION_ASSERT(size == allSizes[N] && "read must return the same size on both the dry run and the real run");
			idx += size;
		};
		(impl.template operator()<Ns>(get<Ns>(proxies)), ...);
		return sz;
	}

	constexpr auto write(std::span<byte> bytes, const T& value, std::endian endian = std::endian::native) -> size_t
	requires (serializable<typename tuple_element_selector<Ns, T>::type::type> && ...)
	{
		size_t sz = 0;
		auto validate = [&sz, &value, endian]<size_t N>(proxy_t<N>& proxy) mutable constexpr {
			size_t size = proxy.write({}, get<N>(value), endian);
			sz += size;
			return size;
		};
		size_t allSizes[sizeof...(Ns)] [[maybe_unused]] = { validate.template operator()<Ns>(get<Ns>(proxies))... };

		if (sz > bytes.size())
			return sz;

		auto impl = [&value, &allSizes, bytes, endian, idx = size_t{}]<size_t N>(proxy_t<N>& proxy) mutable constexpr {
			size_t size = proxy.write(bytes.subspan(idx), get<N>(value), endian);
			SHION_ASSERT(size == allSizes[N] && "write must return the same size on both the dry run and the real run");
			idx += size;
		};
		(impl.template operator()<Ns>(get<Ns>(proxies)), ...);
		return sz;
	}
};

}

inline namespace io
{

SHION_EXPORT template <std::integral T>
struct serializer_helper<T> : detail::serializer::scalar_serializer<T>
{
};

SHION_EXPORT template <std::floating_point T>
struct serializer_helper<T> : detail::serializer::scalar_serializer<T>
{
};

SHION_EXPORT template <typename T>
requires (std::is_enum_v<T>)
struct serializer_helper<T> : detail::serializer::scalar_serializer<T>
{
};

SHION_EXPORT template <typename T, typename Tag>
	requires (!std::ranges::range<T> && requires { tuple_size_selector<T>::type::value; })
struct serializer_helper<T, Tag> : detail::serializer::tuple_serializer<T, Tag, std::make_index_sequence<tuple_size_selector<T>::type::value>>
{
};
/*
SHION_EXPORT template <std::ranges::sized_range T, typename Tag>
struct serializer_helper<T, Tag>
{
	static inline constexpr bool shion_provided = true;

private:
	using iterator_t = std::ranges::iterator_t<T>;
	using value_t = std::ranges::range_value_t<T>;
	using proxy_t = serializer_helper<value_t, Tag>;

	static inline constexpr bool has_tuple_size = requires { tuple_size_selector<T>::type::value; };
	static inline constexpr bool construct_in_place = std::is_default_constructible_v<value_t> && (has_tuple_size || requires { T(size_t{}); });

public:
	static inline constexpr bool constant_value_size = requires { requires proxy_t::constant_size; };
	static inline constexpr bool constant_size = has_tuple_size && constant_value_size;
	
	proxy_t proxy = {};
	
private:
	constexpr bool _construct(T* location, size_t elems) {
		if constexpr (construct_in_place)
		{
			if constexpr (has_tuple_size)
			{
				std::construct_at(location);
			}
			else
			{
				std::construct_at(location, elems);
			}
			return true;
		}
		else
		{
			std::construct_at(location);
			return false;
		}
	}

	constexpr auto _read(std::span<std::byte const> bytes, T* value, size_t n, size_t reqBytes, std::endian endian = std::endian::native, bool construct [[maybe_unused]] = false) -> size_t
	{
		if (reqBytes > bytes.size())
			return reqBytes;
		
		bool has_elements = requires { std::tuple_size<T>::value; };
		if (construct)
		{
			if constexpr (std::is_trivially_default_constructible_v<value_t>)
			{
				if constexpr (requires { std::tuple_size<T>::value; } )
				{
					std::construct_at(value);
					has_elements = true;
				}
				else if (requires { T(n, std::declval<value_t>()); })
				{
					std::construct_at(value, n, value_t{});
					has_elements = true;
				}
				else
				{
					std::construct_at(value);
					has_elements = false;
				}
			}
			else
			{
				if constexpr (requires { std::tuple_size<T>::value; } )
				{
					static_assert(unsatisfyable<T>, "Non-default constructible fixed size containers are not implemented");
				}
				std::construct_at(value);
				has_elements = false;
			}
		}
	
		if constexpr (
			std::ranges::contiguous_range<T>
			&& constant_value_size
			&& std::is_trivially_copyable_v<value_t>
			&& std::has_unique_object_representations_v<value_t>
			&& requires { typename proxy_t::shion_provided; }
		)
		{
			if (has_elements && !std::is_constant_evaluated() && endian == std::endian::native)
			{
				std::memcpy(std::ranges::data(*value), bytes.data(), sizeof(value_t) * n);
				return reqBytes;
			}
		}
		if (has_elements)
		{
			size_t totalBytes = 0;

			for (auto& elem : *value)
			{
				size_t readBytes = proxy.read(bytes.subspan(totalBytes), &elem, endian, false);
				SHION_ASSERT(totalBytes + readBytes <= bytes.size());
				totalBytes += readBytes;
			}
			return totalBytes;
		}
		else
		{
			if constexpr (requires { value->reserve(n); }) {
				value->reserve(n);
			}
			size_t totalBytes = 0;
			for (size_t i = 0; i < n; ++i)
			{
				size_t readBytes;
				if constexpr (std::is_trivially_default_constructible_v<value_t> && std::is_trivially_move_constructible_v<value_t> && requires { value->emplace_back(); })
				{
					auto& v = value->emplace_back();
					readBytes = proxy.read(bytes.subspan(totalBytes), &v, endian, false);
					SHION_ASSERT(totalBytes + readBytes <= bytes.size());
					totalBytes += readBytes;
				}
				else if constexpr ( requires { value->push_back(value_t{}); })
				{
					union storage_t {
						empty e = {};
						value_t v;
					};
					storage_t storage;
					
					readBytes = proxy.read(bytes.subspan(totalBytes), &storage.v, endian, true);
					value->push_back(std::move(storage.v));
					std::destroy_at(&storage.v);
					SHION_ASSERT(totalBytes + readBytes <= bytes.size());
					totalBytes += readBytes;
				}
				else
				{
					SHION_ASSERT(false);
				}
			}
			return totalBytes;
		}
	}

public:
	constexpr auto read(std::span<std::byte const> bytes, T* value, std::endian endian = std::endian::native, bool construct [[maybe_unused]] = false) -> size_t {
		std::span<std::byte const> data = bytes;
		size_t size;
		size_t idx = 0;

		if constexpr (requires { std::tuple_size<T>::value; })
		{
			size = std::tuple_size<T>::value;
		}
		else
		{
			using size_type = decltype(std::ranges::size(*value));
			auto sz = size_type{};
			auto sizeSize = serializer_helper<size_type, Tag>{}.read(bytes, &sz, endian);
			if (sizeSize > bytes.size() || sizeSize == 0)
				return sizeSize;

			idx += sizeSize;
			data = data.subspan(idx);
			size = static_cast<size_t>((std::max<size_type>)(0, sz));
		}

		if constexpr (proxy_t::constant_size)
		{
			auto requiredBytes = proxy_t{}.read({}, nullptr, endian, construct) * size;
			return idx + this->_read(data, value, size, requiredBytes, endian, construct);
		}
		else
		{
			size_t totalBytes = 0;
			for (size_t i = 0; i < size; ++i)
			{
				size_t requiredBytes = proxy_t{}.read(bytes.subspan(idx), nullptr, endian, construct);
				if (requiredBytes > bytes.size() - idx)
				{
					return idx + requiredBytes;
				}
				totalBytes += requiredBytes;
			}
			return this->_read(data, value, size, totalBytes, endian, construct);;
		}
	}
	
	constexpr auto write(std::span<std::byte> bytes, const T& value, std::endian endian = std::endian::native) noexcept -> size_t {
		auto n = std::ranges::size(value);
		size_t reqSize = 0;
		size_t idx = 0;

		if constexpr (!has_tuple_size) {
			reqSize += serializer_helper<decltype(n), Tag>{}.write({}, n, endian);
			idx += reqSize;
		}
		
		for (const auto& v : value) {
			size_t sz = proxy.write({}, v, endian);
			reqSize += sz;
		}

		if (bytes.size() < reqSize) {
			return reqSize;
		}
		
		if constexpr (!has_tuple_size) {
			serializer_helper<decltype(n), Tag>{}.write(bytes, n, endian);
		}
		
		if constexpr (
			std::ranges::contiguous_range<T>
			&& constant_value_size
			&& std::is_trivially_copyable_v<value_t>
			&& std::has_unique_object_representations_v<value_t>
			&& requires { typename proxy_t::shion_provided; }
		)
		{
			if (!std::is_constant_evaluated() && endian == std::endian::native)
			{
				std::memcpy(bytes.data(), std::ranges::data(value), proxy.write({}, value, endian) * n);
				return reqSize;
			}
		}
		
		for (const auto& v : value) {
			size_t written = proxy.write(bytes.subspan(idx), v, endian);
			idx += written;
		}
		return reqSize;
	}
};
*/
}

namespace detail
{

template <typename T>
class serializer_base
{
public:

protected:
	T* _buffer = nullptr;
};

}

inline namespace io
{

SHION_EXPORT template <typename T, typename Tag>
class serializer
{
public:
private:
};

}

}

#endif /* SHION_IO_SERIALIZER_H_ */
