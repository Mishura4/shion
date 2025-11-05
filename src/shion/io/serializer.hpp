#ifndef SHION_IO_SERIALIZER_H_
#define SHION_IO_SERIALIZER_H_

#include <shion/common/defines.hpp>

#if !SHION_BUILDING_MODULES
#	include <shion/utility/tuple.hpp>
#	include <shion/common/common.hpp>

#	include <tuple>
#	include <ranges>
#	include <concepts>
#endif

namespace SHION_NAMESPACE
{

inline namespace io
{

SHION_EXPORT template <typename T, typename Tag = void>
struct serializer_helper;

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
SHION_EXPORT template <typename T, typename Tag>
struct serializer_helper {
	/**
	 * @brief Extracts a value from a buffer and returns it in-place.
	 *
	 * @param bytes The current amount of bytes available.
	 * @param endian The endianness to read the data as.
	 *
	 * @return Returns a new object constructed from the buffer.
	 */
	constexpr auto construct(std::span<const std::byte>& bytes, std::endian endian = std::endian::native) -> T = delete;

	/**
	 * @brief Evaluate the size required to extract a value from the buffer.
	 *
	 * @param bytes The current amount of bytes available.
	 * @param endian The endianness to read the data as.
	 *
	 * @return Returns the total amount of bytes we need.
	 * @retval If 0, the deserialization shall abort.
	 * @retval If < 0, the size is expected to be incomplete and this function shall be called again.
	 * @retval If > 0, the size is complete and can be extracted.
	 */
	constexpr auto size(std::span<const std::byte> bytes, std::endian endian = std::endian::native) -> ptrdiff_t = delete;

	/**
	 * @brief Extracts a value from a buffer.
	 *
	 * @param bytes The current amount of bytes available.
	 * @param value The value to fill. If nullptr, this is a dry run.
	 * @param endian The endianness to read the data as.
	 *
	 * @return Returns the total amount of bytes we need or have read.
	 * @retval If 0, the deserialization shall abort.
	 * @retval If != 0 && <= bytes.size(), the behavior is undefined.
	 */
	constexpr auto read(std::span<const std::byte> bytes, T& value, std::endian endian = std::endian::native) -> ptrdiff_t = delete;

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
	constexpr auto write(std::span<std::byte> bytes, const T& value, std::endian endian = std::endian::native) -> ptrdiff_t = delete;
};

SHION_EXPORT template <typename T>
struct buffer_traits;

template <storage_buffer T>
struct buffer_traits<T>
{
	constexpr static size_t capacity(const T& buffer) noexcept requires (sized_buffer<T>)
	{
		return buffer_capacity<T>(buffer);
	}
};

SHION_EXPORT template <typename T>
concept serializable = requires (std::span<std::byte> bytes, T val) { serializer_helper<T>{}.write(bytes, val); };

SHION_EXPORT template <typename T>
concept deserialize_readable = requires (std::span<const std::byte> bytes) { serializer_helper<T>{}.size(bytes); };

SHION_EXPORT template <typename T>
concept deserialize_constructible = requires (std::span<const std::byte> bytes) { serializer_helper<T>{}.construct(bytes); };

SHION_EXPORT template <typename T>
concept deserializable = deserialize_readable<T> || deserialize_constructible<T>;

SHION_EXPORT struct reader
{
	template <typename T, size_t N>
	constexpr auto operator()(std::span<const std::byte, N> bytes, T& value) requires requires (serializer_helper<T> s) { s.read(bytes, value); }
	{
		return serializer_helper<T>{}.read(bytes, value);
	}

	template <typename T, size_t N>
	constexpr auto operator()(std::span<const std::byte, N> bytes, T& value, std::endian endian) requires requires (serializer_helper<T> s) { s.read(bytes, value, endian); }
	{
		return serializer_helper<T>{}.read(bytes, value, endian);
	}
};

SHION_EXPORT struct writer
{
	template <typename T, size_t N>
	constexpr auto operator()(std::span<std::byte, N> bytes, const T& value) requires requires (serializer_helper<T> s) { s.write(bytes, value); }
	{
		return serializer_helper<T>{}.write(bytes, value);
	}

	template <typename T, size_t N>
	constexpr auto operator()(std::span<std::byte, N> bytes, const T& value, std::endian endian) requires requires (serializer_helper<T> s) { s.write(bytes, value, endian); }
	{
		return serializer_helper<T>{}.write(bytes, value, endian);
	}
};

SHION_EXPORT inline constexpr auto read = reader{};
SHION_EXPORT inline constexpr auto write = writer{};

} // namespace io

namespace detail::serializer
{

template <typename T>
inline constexpr bool is_constant_size_serializer = false;

template <typename T, typename Tag>
	requires requires { serializer_helper<T, Tag>::constant_size; }
inline constexpr bool is_constant_size_serializer<serializer_helper<T, Tag>> = serializer_helper<T, Tag>::constant_size;
	
inline constexpr unsigned char size_shift = (CHAR_BIT - 1);
inline constexpr unsigned char size_bit = 1 << size_shift;
inline constexpr unsigned char num_bytes_for_size(auto sz)
{
//	auto highest_bit = ((sizeof(decltype(sz)) * CHAR_BIT) - std::countl_zero(sz));
//	return static_cast<unsigned char>((highest_bit >> 3) + 1);
	int count = 1;
	while (sz >= size_bit)
	{
		sz >>= 7;
		++count;
	}
	return static_cast<unsigned char>(count);
}

inline constexpr auto two_7 = 128;
inline constexpr auto two_14 = 16384;

static_assert(num_bytes_for_size(size_t{0}) == 1);
static_assert(num_bytes_for_size(size_t{two_7 - 1}) == 1);
static_assert(num_bytes_for_size(size_t{two_7}) == 2);
static_assert(num_bytes_for_size(size_t{two_14 - 1}) == 2);
static_assert(num_bytes_for_size(size_t{two_14}) == 3);
static_assert(num_bytes_for_size(size_t{0b10000000'00000000}) == 3);

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
	constexpr auto _read(std::span<std::byte const> bytes, T& value) noexcept -> ptrdiff_t
	{
		if (!std::is_constant_evaluated())
			std::memcpy(&value, bytes.data(), sizeof(T));
		else
		{
			std::array<std::byte, sizeof(T)> arr;
			std::ranges::copy_n(bytes.data(), sizeof(T), arr.begin());
			value = std::bit_cast<T>(arr);
		}
		return sizeof(T);
	}

	constexpr auto _write(std::span<std::byte> bytes, const T& value) noexcept -> ptrdiff_t
	{
		if (!std::is_constant_evaluated())
			std::memcpy(bytes.data(), &value, sizeof(T));
		else
			std::ranges::copy(std::bit_cast<std::array<std::byte, sizeof(T)>>(value), bytes.begin());
		return sizeof(T);
	}

public:
	static inline constexpr bool trivial = true;
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

	constexpr auto size(std::span<std::byte const> /* bytes */, std::endian /* endian */ = std::endian::native) -> ptrdiff_t
	{
		return sizeof(T);
	}

	constexpr auto read(std::span<std::byte const> bytes, T& value, std::endian endian = std::endian::native) noexcept -> ptrdiff_t
	{
		auto sz = this->_read(bytes, value);
		if (endian != std::endian::native)
			value = this->byteswap(value);
		return sz;
	}

	constexpr auto write(std::span<std::byte> bytes, const T& value, std::endian endian = std::endian::native) noexcept -> ptrdiff_t
	{
		constexpr ptrdiff_t sz = sizeof(T);
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

}

inline namespace io
{

template <std::integral T>
struct serializer_helper<T> : detail::serializer::scalar_serializer<T>
{
};

template <std::floating_point T>
struct serializer_helper<T> : detail::serializer::scalar_serializer<T>
{
};

template <typename T>
requires (std::is_enum_v<T>)
struct serializer_helper<T> : detail::serializer::scalar_serializer<T>
{
};

}

namespace detail::serializer
{

template <typename T, typename Tag, typename Ns>
struct tuple_serializer;

template <typename T, typename Tag, size_t... Ns>
struct tuple_serializer<T, Tag, std::index_sequence<Ns...>>
{
	template <size_t N>
	using proxy_t = serializer_helper<std::remove_cv_t<std::tuple_element_t<N, T>>, Tag>;
	using proxies_t = tuple<proxy_t<Ns>...>;
	
	static inline constexpr bool trivial = true;
	template <size_t N>
	static inline constexpr bool constant_size_n = (requires { requires proxy_t<N>::constant_size; });
	static inline constexpr bool constant_size = (constant_size_n<Ns> && ...);

	proxies_t proxies;

	constexpr auto construct(std::span<const byte>& bytes, std::endian endian = std::endian::native) -> T
#if !SHION_INTELLISENSE
	requires (deserialize_constructible<typename std::tuple_element<Ns, T>::type> && ...)
#endif
	{
		SHION_ASSERT(size(bytes, endian) <= static_cast<ptrdiff_t>(bytes.size()));
		return T{ get<Ns>(proxies).construct(bytes, endian)... };
	}

	constexpr auto size(std::span<const byte> bytes, std::endian endian = std::endian::native) -> ptrdiff_t
#if !SHION_INTELLISENSE
	requires (deserializable<typename std::tuple_element<Ns, T>::type> && ...)
#endif
	{
		ptrdiff_t sz = 0;
		bool failed = false;
		auto validate = [&sz, endian, bytes, &failed]<size_t N>(proxy_t<N>& proxy) mutable constexpr -> ptrdiff_t {
			ptrdiff_t size;
			if constexpr (detail::serializer::is_constant_size_serializer<proxy_t<N>>)
			{
				(void)bytes;
				(void)failed;
				size = proxy.size({}, endian);
				sz += size;
			}
			else
			{
				if (failed)
					return 0;

				size = proxy.size(bytes.subspan(sz), endian);
				sz += size * bool_to_sign(size > 0);
				failed = size < 0 || sz > static_cast<ptrdiff_t>(bytes.size());
			}
			return size;
		};
		std::initializer_list<ptrdiff_t> values [[maybe_unused]] = { validate.template operator()<Ns>(get<Ns>(proxies))... };
		return bool_to_sign(!failed) * sz;
	}

	constexpr auto read(std::span<const byte> bytes, T& value, std::endian endian = std::endian::native) -> ptrdiff_t
#if !SHION_INTELLISENSE
	requires (deserializable<typename std::tuple_element<Ns, T>::type> && ...)
#endif
	{
		SHION_ASSERT(static_cast<ptrdiff_t>(bytes.size()) >= size(bytes, endian));

		ptrdiff_t sz = 0;
		auto impl = [endian, bytes, &value, &sz]<size_t N>(proxy_t<N>& proxy) mutable constexpr {
			sz += proxy.read(bytes.subspan(sz), get<N>(value), endian);
			return empty{};
		};
		std::initializer_list<empty> values [[maybe_unused]] = { impl.template operator()<Ns>(get<Ns>(proxies))... };
		return sz;
	}

	constexpr auto write(std::span<byte> bytes, const T& value, std::endian endian = std::endian::native) -> ptrdiff_t
#if !SHION_INTELLISENSE
	requires (serializable<typename std::tuple_element<Ns, T>::type> && ...)
#endif
	{
		size_t sz = 0;
		auto validate = [&sz, &value, endian]<size_t N>(proxy_t<N>& proxy) mutable constexpr {
			size_t size = proxy.write({}, get<N>(value), endian);
			sz += size;
			return size;
		};
		[[maybe_unused]] size_t allSizes[sizeof...(Ns)] = { validate.template operator()<Ns>(get<Ns>(proxies))... };

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

template <typename T, typename Tag>
requires (deserialize_constructible<T> || std::is_default_constructible_v<T>)
struct serializer_input_iterator
{
	using value_type = T;
	using reference = T;
	using difference_type = ptrdiff_t;
	
	std::span<const byte>* bytes{};
	size_t cur_size{};
	mutable serializer_helper<T, Tag> serializer{};
	std::endian endian{};

	constexpr serializer_input_iterator() = default;
	
	constexpr serializer_input_iterator(SHION_LIFETIMEBOUND std::span<const byte>& bytes_, std::endian endian_) :
		bytes(&bytes_),
		cur_size(calc_size()),
		endian(endian_)
	{}

	constexpr auto calc_size() -> size_t {
		std::construct_at(&serializer);
		return serializer.size(*bytes, endian);
	}

	constexpr auto operator++() noexcept -> serializer_input_iterator& {
		*bytes = bytes->subspan(cur_size);
		std::destroy_at(&serializer);
		cur_size = calc_size();
		return *this;
	}

	constexpr auto operator++(int) noexcept -> void {
		++(*this);
	}

	constexpr T operator*() const
	{
		if constexpr (deserialize_constructible<T>)
		{
			auto b = *bytes;
			return serializer.construct(b, endian);
		}
		else
		{
			T t;
			serializer.read(*bytes, &t, endian);
			return t;
		}
	}

	friend constexpr bool operator==(serializer_input_iterator const& lhs, serializer_input_iterator const& rhs) noexcept {
		return (lhs == std::default_sentinel_t{} && rhs == std::default_sentinel_t{});
	}

	constexpr bool operator==(std::default_sentinel_t) const noexcept {
		return !bytes || bytes->size() < cur_size;
	}
};

} // namespace detail::serializer

namespace detail::serializer
{

inline constexpr auto read_compressed_range_size(std::span<const std::byte> bytes)
{
	struct result
	{
		ptrdiff_t idx_out;
		size_t range_size;
	};
	result ret { 0, 0 };
	unsigned char byte = size_bit;

	while ((byte & size_bit) != 0)
	{
		auto idx = ret.idx_out++;
		if (static_cast<ptrdiff_t>(bytes.size()) < ret.idx_out)
		{
			ret.idx_out *= -1;
			return ret;
		}
	
		byte = static_cast<unsigned char>(bytes[idx]);
		ret.range_size = (ret.range_size << size_shift) | (byte & (~size_bit));
	}
	return ret;
}

template <typename T, typename Tag>
struct container_constructor {};

template <tuple_range T, typename Tag>
requires (deserialize_constructible<std::ranges::range_value_t<T>>)
struct container_constructor<T, Tag> {
private:
	using value_t = std::ranges::range_value_t<T>;
	using iterator_t = serializer_input_iterator<value_t, Tag>;
	using proxy_t = serializer_helper<value_t, Tag>;

public:
	constexpr auto construct(std::span<const byte>& bytes, std::endian endian = std::endian::native) -> T
	{
		SHION_ASSERT(read(bytes, static_cast<value_t*>(nullptr), endian) <= bytes.size());
		iterator_t it;
		if constexpr (tuple_size_selector<T>::type::value < 128) {
			auto impl = [&bytes, endian]<size_t N>() {
				return proxy_t{}.construct(bytes, endian);
			};
			auto unfold = [&impl]<size_t... Ns>(std::index_sequence<Ns...>) {
				return T{ impl.template operator()<Ns>()... };
			};
			return unfold(std::make_index_sequence<tuple_size_selector<T>::type::value>{});
		} else if (std::is_default_constructible_v<value_t>) {
			T    ret;
			auto impl = [&bytes, endian] {
				return proxy_t{}.construct(bytes, endian);
			};
			std::ranges::generate_n(std::ranges::begin(ret), tuple_size_selector<T>::type::value, impl);
			return ret;
		} else {
			static_assert(std::is_default_constructible_v<value_t>, "Tuple too large & non-default-constructible, please specialize serializer_helper<T>.");
			SHION_ASSERT(false);
			unreachable();
		}
	}
};

template <std::ranges::sized_range T, typename Tag>
requires (!tuple_range<T> && deserialize_constructible<std::ranges::range_value_t<T>>)
struct container_constructor<T, Tag>
{
private:
	using value_t = std::ranges::range_value_t<T>;
	using iterator_t = serializer_input_iterator<value_t, Tag>;
	using proxy_t = serializer_helper<value_t, Tag>;
	using constructed_t = std::remove_cv_t<T>;

	constexpr auto _construct_default(std::span<const byte>& bytes, size_t n, std::endian endian) const -> T
	{
		T ret;
		if constexpr (resizable_container<T> && std::is_default_constructible_v<value_t>)
		{
			ret.resize(n);
			if constexpr (std::is_trivially_copyable_v<value_t> && requires { proxy_t::trivial; })
			{
				std::memcpy(ret.data(), bytes.data(), sizeof(value_t) * n);
				if constexpr (std::is_scalar_v<value_t> && sizeof(value_t) > 1)
				{
					if (endian != std::endian::native)
					{
						for (auto& v : ret)
							v = std::byteswap(v);
					}
				}
				bytes = bytes.subspan(sizeof(value_t) * n);
			}
			else
			{
				for (auto& value : ret)
				{
					size_t sz = serializer_helper<value_t, Tag>{}.read(bytes, value, endian);
					bytes = bytes.subspan(sz);
				}
			}
		}
		else
		{
			if constexpr (reservable_container<T>)
			{
				ret.reserve(n);
			}
			for (size_t i = 0; i < n; ++i)
			{
				if constexpr (requires { ret.push_back(std::declval<value_t>()); }) // std::vector-like
				{
					ret.push_back(proxy_t{}.construct(bytes, endian));
				}
				else if constexpr ( requires { ret.push(std::declval<value_t>()); } ) // std::queue-like
				{
					ret.push(proxy_t{}.construct(bytes, endian));
				}
				else // insanity
				{
					size_t sz = proxy_t{}.read(bytes, &ret.push_back(), endian);
					bytes = bytes.subspan(sz);
				}
			}
		}
		return ret;
	}

public:
	constexpr auto construct(std::span<const byte>& bytes, std::endian endian = std::endian::native) const -> T {
		using size_type = std::ranges::range_size_t<T>;
		size_type range_size = 0;
		using size_proxy = serializer_helper<size_type, Tag>;

		if constexpr (tuple_range<T>)
		{
			range_size = tuple_size_selector<T>::type::value;
		}
		else
		{
			if constexpr (requires { size_proxy::trivial; })
			{
				auto res = read_compressed_range_size(bytes);
				SHION_ASSERT(res.idx_out > 0);
				bytes = bytes.subspan(res.idx_out);
				range_size = static_cast<size_type>(res.range_size);
			}
			else
			{
				SHION_ASSERT(size_proxy{}.read(bytes, range_size, endian) > 0);
			}
		}

#if !(defined(_LIBCPP_VERSION) && _LIBCPP_VERSION < 200000)
		
#endif
		if constexpr (!std::is_const_v<T> && std::is_default_constructible_v<T>)
		{
			return _construct_default(bytes, range_size, endian);
		}
		else if constexpr (std::constructible_from<T, iterator_t, size_t>)
		{
			return T(iterator_t{ bytes, endian }, range_size);
		}
		else if constexpr (std::constructible_from<T, std::from_range_t, decltype(std::views::counted(std::declval<iterator_t>(), range_size))>)
		{
			return T(std::from_range, std::views::counted(iterator_t{ bytes, endian }, range_size));
		}
		else if constexpr (std::constructible_from<T, std::span<const byte>&, std::endian>)
		{
			return T(bytes, endian);
		}
		else
		{
			static_assert(std::is_default_constructible_v<T>, "Unrecognized container construction, please specialize serializer_helper<T>.");
			unreachable();
		}
	}
};

template <typename T, typename Tag>
struct container_reader {};

template <std::ranges::sized_range T, typename Tag>
struct container_reader<T, Tag>
{
	using value_t = typename T::value_type;
	using proxy_t = serializer_helper<value_t, Tag>;
	using size_type = std::ranges::range_size_t<T>;

	constexpr auto _fill_contiguous(std::span<const byte> bytes, T& value, size_t range_size, std::endian endian = std::endian::native) -> ptrdiff_t
	requires (std::ranges::contiguous_range<T>)
	{
		size_t start = 0;
		if constexpr (resizable_container<T>)
		{
			start = std::ranges::size(value);
			value.resize(start + range_size);
		}
		else
		{
			SHION_ASSERT(value.size() >= range_size);
		}
		ptrdiff_t size_in_bytes = 0;
		if constexpr (std::is_scalar_v<value_t> && requires { requires proxy_t::trivial; })
		{
			size_in_bytes = sizeof(value_t) * range_size;
			SHION_ASSERT(size_in_bytes <= static_cast<ptrdiff_t>(bytes.size()));
			std::memcpy(value.data() + start, bytes.data(), size_in_bytes);
			if constexpr (sizeof(value_t) > 1 && !std::is_floating_point_v<value_t>)
			{
				if (endian != std::endian::native)
				{
					for (size_t i = 0; i < range_size; ++i)
						value[start + i] = std::byteswap(value[start + i]);
				}
			}
		}
		else
		{
			for (size_t i = start; i < start + range_size; ++i)
			{
				ptrdiff_t sz = proxy_t{}.read(bytes.subspan(size_in_bytes), value[start + i], endian);
				size_in_bytes += sz;
				SHION_ASSERT(size_in_bytes <= static_cast<ptrdiff_t>(bytes.size()));
			}
		}
		return size_in_bytes;
	}

public:
	constexpr auto size(std::span<const byte> bytes, std::endian endian = std::endian::native) -> ptrdiff_t
	{
		size_type range_size = 0;
		ptrdiff_t size_in_bytes = 0;
		using size_proxy = serializer_helper<size_type, Tag>;

		if constexpr (tuple_range<T>)
		{
			range_size = tuple_size_selector<T>::type::value;
		}
		else
		{
			if constexpr (requires { size_proxy::trivial; })
			{
				auto res = read_compressed_range_size(bytes);
				if (res.idx_out <= 0)
					return res.idx_out;
				size_in_bytes = res.idx_out;
				range_size = res.range_size;
			}
			else
			{
				size_in_bytes += size_proxy{}.read(bytes, &range_size, endian);
			}
		}

		size_type i = 0;
		if constexpr (requires { requires proxy_t::constant_size; })
		{
			return size_in_bytes + range_size * proxy_t{}.size({}, endian);
		}
		else
		{
			while (i < range_size)
			{
				ptrdiff_t sz = proxy_t{}.size(bytes.subspan(size_in_bytes), endian);
				if (sz < 0)
					return (-1 * size_in_bytes) + sz;
				
				size_in_bytes += sz;
				++i;
			}
			return size_in_bytes;
		}
	}
	
	constexpr auto read(std::span<const byte> bytes, T& value, std::endian endian = std::endian::native) -> ptrdiff_t
	requires (deserializable<value_t>)
	{
		size_type range_size = 0;
		ptrdiff_t size_in_bytes = 0;
		using size_proxy = serializer_helper<size_type, Tag>;

		if constexpr (tuple_range<T>)
		{
			range_size = tuple_size_selector<T>::type::value;
		}
		else
		{
			if constexpr (requires { size_proxy::trivial; })
			{
				auto res = read_compressed_range_size(bytes);
				if (res.idx_out <= 0)
					return res.idx_out;
				size_in_bytes = res.idx_out;
				range_size = res.range_size;
			}
			else
			{
				size_in_bytes += size_proxy{}.read(bytes, size_in_bytes, endian);
			}
		}

		SHION_ASSERT(size_in_bytes <= static_cast<ptrdiff_t>(bytes.size()));
		bytes = bytes.subspan(size_in_bytes);
		if constexpr (std::ranges::contiguous_range<T>)
		{
			return size_in_bytes + _fill_contiguous(bytes, value, range_size, endian);
		}
		else
		{
			using iterator_t = serializer_input_iterator<value_t, Tag>;
				
			if constexpr (reservable_container<T>)
			{
				value.reserve(std::ranges::size(value) + range_size);
			}
			auto range = std::views::counted(iterator_t{ bytes, endian }, range_size);
			if constexpr (requires (value_t val) { value.push_back(std::move(val)); })
			{
				for (auto it = range.begin(); it != range.end(); ++it)
				{
					value.push_back(*it);
					size_in_bytes += it.base().cur_size;
				}
			}
			else if constexpr (requires (value_t val) { value.emplace(std::move(val)); })
			{
				for (auto it = range.begin(); it != range.end(); ++it)
				{
					value.emplace(*it);
					size_in_bytes += it.base().cur_size;
				}
			}
			else
			{
				static_assert(unsatisfyable<T>, "Not sure how to add to this container.");
				unreachable();
			}
			return size_in_bytes;
		}
	}
};

template <typename T, typename Tag>
struct container_writer {};

template <std::ranges::sized_range T, typename Tag>
struct container_writer<T, Tag>
{
	using value_t = std::ranges::range_value_t<T>;
	using proxy_t = serializer_helper<value_t, Tag>;

	constexpr auto write(std::span<std::byte> bytes, const T& value, std::endian endian = std::endian::native) -> ptrdiff_t
	requires (serializable<value_t>)
	{
		ptrdiff_t sz = 0;
		auto   size = std::ranges::size(value);
		if (bytes.empty())
		{
			if constexpr (!tuple_range<T>)
			{
				using size_proxy = serializer_helper<std::ranges::range_size_t<T>, Tag>;
				if constexpr (requires { requires size_proxy::constant_size; })
				{
					sz += num_bytes_for_size(size);
				}
				else
				{
					sz += size_proxy{}.write({}, size, endian);
				}
			}
			if constexpr (requires { requires proxy_t::constant_size; })
			{
				if (!std::ranges::empty(value))
					sz += size * proxy_t{}.write({}, *std::ranges::begin(value), endian);
			}
			else
			{
				for (const value_t& v : value)
				{
					sz += proxy_t{}.write({}, v, endian);
				}
			}
			return sz;
		}

		if constexpr (!tuple_range<T>)
		{
			using size_proxy = serializer_helper<std::ranges::range_size_t<T>, Tag>;
			auto range_size = std::ranges::size(value);
			if constexpr (requires { requires size_proxy::trivial; })
			{
				ptrdiff_t range_size_bytes = num_bytes_for_size(range_size);
				while (sz < range_size_bytes)
				{
					auto shifted = range_size >> (size_shift * (range_size_bytes - sz - 1));
					bytes[sz] = static_cast<std::byte>(shifted | size_bit);
					++sz;
				}
				bytes[sz - 1] &= static_cast<std::byte>(~size_bit);
			}
			else
			{
				sz += size_proxy{}.write(bytes.subspan(sz), range_size, endian);
			}
		}
		if constexpr (std::ranges::contiguous_range<T> && std::is_scalar_v<value_t> && requires { requires proxy_t::trivial; })
		{
			std::memcpy(bytes.data() + sz, value.data(), sizeof(value_t) * std::ranges::size(value));
			if constexpr (sizeof(value_t) > 1 && !std::is_floating_point_v<T>)
			{
				if (endian != std::endian::native)
				{
					std::byte* data = bytes.data() + sz;
					for (size_t i = 0; i < std::ranges::size(value); ++i)
					{
						for (size_t j = 0; j < sizeof(value_t) / 2; ++j)
						{
							auto& a = data[j];
							auto& b = data[sizeof(value_t) - j - 1];
							auto temp = data[j];
							a = b;
							b = temp;
						}
					}
				}
			}
			sz += size;
		}
		else
		{
			for (const value_t& v : value)
			{
				sz += proxy_t{}.write(bytes.subspan(sz), v, endian);
			}
		}
		return sz;
	}
};

}

inline namespace io
{

template <typename T, typename Tag>
	requires (!std::ranges::range<T> && tuple_like<T>)
struct serializer_helper<T, Tag> : detail::serializer::tuple_serializer<T, Tag, std::make_index_sequence<tuple_size_selector<T>::type::value>>
{
};

template <typename T, typename Tag>
	requires (std::ranges::range<T>)
struct serializer_helper<T, Tag> : detail::serializer::container_constructor<T, Tag>, detail::serializer::container_writer<T, Tag>, detail::serializer::container_reader<T, Tag>
{
};

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
