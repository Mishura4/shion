#ifndef SHION_VECTOR_H_
#define SHION_VECTOR_H_

#include <exception>
#include <new>
#include <utility>
#include <type_traits>
#include <cmath>

#include "../shion_essentials.hpp"

namespace shion {

inline namespace geometry {

/**
 * @brief Macro that defines operator[] and at() in a generic way for mathematic tuples
 */
#define SHION_DEFINE_TUPLE_ACCESS(N) \
	constexpr decltype(auto) operator[](int64 idx) & noexcept { \
		SHION_ASSERT(idx >= 0 && idx < (N)); \
 \
		return static_cast<T&>(*(std::launder<T*>(this) + idx)); \
	} \
 \
	constexpr decltype(auto) operator[](int64 idx) && noexcept { \
		SHION_ASSERT(idx >= 0 && idx < (N)); \
 \
		return static_cast<T&&>(*(std::launder<T*>(this) + idx)); \
	} \
 \
	constexpr decltype(auto) operator[](int64 idx) const& noexcept { \
		SHION_ASSERT(idx >= 0 && idx < (N)); \
 \
		return static_cast<T const&>(*(std::launder<T const*>(this) + idx)); \
	} \
 \
	constexpr decltype(auto) at(int64 idx) & { \
		if (!(idx >= 0 && idx < (N))) \
			throw std::out_of_range{"vector index out of range"}; \
 \
		return static_cast<T&>(*(std::launder<T*>(this) + idx)); \
	} \
	\
	constexpr decltype(auto) at(int64 idx) && { \
		if (!(idx >= 0 && idx < (N))) \
			throw std::out_of_range{"vector index out of range"}; \
 \
		return static_cast<T&&>(*(std::launder<T*>(this) + idx)); \
	} \
	\
	constexpr decltype(auto) at(int64 idx) const& { \
		if (!(idx >= 0 && idx < (N))) \
			throw std::out_of_range{"vector index out of range"}; \
 \
		return static_cast<T const&>(*(std::launder<T const*>(this) + idx)); \
	} \
	\
	static consteval int64 tuple_size() noexcept {\
		return N;\
	};\
	static_assert(true) /* force semicolon :) */

/**
 * @brief Mathematical 2D vector, i.e. the difference between 2 points.
 */
template <typename T>
struct vector2 {
	SHION_DEFINE_TUPLE_ACCESS(2);

	friend constexpr vector2 operator*(vector2 const& lhs, T factor) noexcept {
		return {lhs.x * factor, lhs.y * factor};
	}

	friend constexpr vector2 operator*(T factor, vector2 const& rhs) noexcept {
		return {factor * rhs.x, factor * rhs.y};
	}

	constexpr vector2& operator*=(T factor) noexcept {
		x *= factor;
		y *= factor;
		return *this;
	}

	friend constexpr vector2 operator/(vector2 const& lhs, T factor) noexcept {
		return {lhs.x / factor, lhs.y / factor};
	}

	friend constexpr vector2 operator/(T factor, vector2 const& rhs) noexcept {
		return {factor / rhs.x, factor / rhs.y,};
	}

	constexpr vector2& operator/=(T factor) noexcept {
		x /= factor;
		y /= factor;
		return *this;
	}

	/**
	 * @brief The X or "length" coordinate in 2D space.
	 */
	T x = {};

	/**
	 * @brief The Y or "height" coordinate in 2D space.
	 */
	T y = {};
};

/**
 * @brief Mathematical 3D vector, i.e. the difference between 2 points.
 */
template <typename T>
struct vector3 {
	SHION_DEFINE_TUPLE_ACCESS(3);

	friend constexpr vector3 operator*(vector3 const& lhs, T factor) noexcept {
		return {lhs.x * factor, lhs.y * factor, lhs.z * factor};
	}

	friend constexpr vector3 operator*(T factor, vector3 const& rhs) noexcept {
		return {factor * rhs.x, factor * rhs.y, factor * rhs.z};
	}

	constexpr vector3& operator*=(T factor) noexcept {
		x *= factor;
		y *= factor;
		z *= factor;
		return *this;
	}

	friend constexpr vector3 operator/(vector3 const& lhs, T factor) noexcept {
		return {lhs.x / factor, lhs.y / factor, lhs.z / factor};
	}

	friend constexpr vector3 operator/(T factor, vector3 const& rhs) noexcept {
		return {factor / rhs.x, factor / rhs.y, factor / rhs.z};
	}

	constexpr vector3& operator/=(T factor) noexcept {
		x /= factor;
		y /= factor;
		z /= factor;
		return *this;
	}

	/**
	 * @brief The X or "length" coordinate in 3D space.
	 */
	T x = {};

	/**
	 * @brief The Y or "height" coordinate in 3D space.
	 */
	T y = {};

	/**
	 * @brief The Z or "depth" coordinate in 3D space.
	 */
	T z = {};
};

/**
 * @brief Mathematical 3D vector, i.e. the difference between 2 points, with a rotation.
 */
template <typename T>
struct quaternion : vector3<T> {
	/**
	 * @brief Rotation. TODO write this because i have no idea
	 */
	T w = {};
};

/**
 * @brief Dimensions in 2D space.
 */
template <typename T>
struct dimensions2 {
	/**
	 * @brief Width, i.e. right x - left x.
	 */
	T width = {};

	/**
	 * @brief Height, i.e. bottom y - top x.
	 */
	T height = {};

	template <typename U>
	dimensions2 operator*(U factor) const noexcept {
		return {.width = width * factor, .height = height * factor};
	}

	template <typename U>
	dimensions2 &operator*(U factor) const {
		width *= factor;
		height *= factor;
		return *this;
	}
};

/**
 * @brief Dimensions in 3D space.
 */
template <typename T>
struct dimensions3 {
	/**
	 * @brief Width, i.e. right x - left x.
	 */
	T width = {};

	/**
	 * @brief Height, i.e. top y - bottom x.
	 */
	T height = {};

	/**
	 * @brief Depth, i.e. back z - front z.
	 */
	T depth = {};
};

/**
 * @brief Point in 2D space. (0, 0) is top left.
 */
template <typename T>
struct point2 {
	SHION_DEFINE_TUPLE_ACCESS(2);

	/**
	 * @brief Calculate the vector between from this point to another.
	 *
	 * @param other Point to get a vector to
	 * @return Vector2 to the other point
	 */
	constexpr auto to(const point2& other) const noexcept {
		if constexpr (std::is_unsigned_v<T>) {
			using as_signed = std::make_signed_t<T>;

			return vector2<std::make_signed_t<T>>{
				.x = (other.x > x ? static_cast<as_signed>(other.x - x) : -static_cast<as_signed>(x - other.x)),
				.y = (other.y > y ? static_cast<as_signed>(other.y - y) : -static_cast<as_signed>(y - other.y)),
			};
		} else {
			return vector2{other.x - x, other.y - y};
		}
	}

	/**
	 * @brief Calculate the vector between to this point from another.
	 *
	 * @param other Point to get a vector from
	 * @return Vector2 from the other point
	 */
	constexpr auto from(const point2& other) const noexcept {
		if constexpr (std::is_unsigned_v<T>) {
			using as_signed = std::make_signed_t<T>;

			return vector2<std::make_signed_t<T>>{
				.x = (other.x > x ? -static_cast<as_signed>(other.x - x) : static_cast<as_signed>(x - other.x)),
				.y = (other.y > y ? -static_cast<as_signed>(other.y - y) : static_cast<as_signed>(y - other.y)),
			};
		} else {
			return vector2{x - other.x, y - other.y};
		}
	}

	/**
	 * @brief Calculate the squared distance between this point and another.
	 *
	 * @param other Point to calculate the distance to
	 * @return Squared distance to the point
	 */
	template <typename U = T>
	constexpr U distance_squared(point2 const& other) const noexcept {
		if constexpr (std::is_unsigned_v<T> || std::is_unsigned_v<U>) {
			U dx;
			U dy;

			if (other.x > x) {
				dx = other.x - x;
			} else {
				dx = x - other.x;
			}

			if (other.y > y) {
				dy = other.y - y;
			} else {
				dy = y - other.y;
			}

			return dx * dx + dy * dy;
		} else {
			U dx = other.x - x;
			U dy = other.y - y;

			return dx * dx + dy * dy;
		}
	}

	/**
	 * @brief Calculate the distance between this point and another.
	 *
	 * May be performance-heavy from calculating the square root, consider using distance_squared instead.
	 *
	 * @see distance_squared
	 * @param other Point to calculate the distance to
	 * @return Squared distance to the point
	 */
	double distance(point2 const& other) const noexcept {
		return std::sqrt(distance_squared(other));
	}

	friend constexpr point2 operator+(point2 const& lhs, vector2<T> const& rhs) noexcept {
		return {lhs.x + rhs.x, lhs.y + rhs.y};
	}

	friend constexpr point2 operator+(vector2<T> const& lhs, point2 const& rhs) noexcept {
		return {lhs.x + rhs.x, lhs.y + rhs.y};
	}

	friend constexpr point2 operator-(point2 const& lhs, vector2<T> const& rhs) noexcept {
		return {lhs.x - rhs.x, lhs.y - rhs.y};
	}

	friend constexpr point2 operator-(vector2<T> const& lhs, point2 const& rhs) noexcept {
		return {lhs.x - rhs.x, lhs.y - rhs.y};
	}

	constexpr point2 &operator+=(vector2<T> const& rhs) noexcept {
		x += rhs.x;
		y += rhs.y;
		return *this;
	}

	constexpr point2 &operator-=(vector2<T> const& rhs) noexcept {
		x -= rhs.x;
		y -= rhs.y;
		return *this;
	}

	/**
	 * @brief The X or "length" coordinate in 3D space.
	 */
	T x = {};

	/**
	 * @brief The Y or "height" coordinate in 3D space.
	 */
	T y = {};
};

template <typename T>
struct point3 {
	SHION_DEFINE_TUPLE_ACCESS(3);

	/**
	 * @brief Calculate the vector between from this point to another.
	 *
	 * @param other Point to get a vector to
	 * @return Vector3 to the other point
	 */
	auto to(const point3& other) const noexcept {
		if constexpr (std::is_unsigned_v<T>) {
			using as_signed = std::make_signed_t<T>;

			return vector3<std::make_signed_t<T>>{
				.x = (other.x > x ? static_cast<as_signed>(other.x - x) : -static_cast<as_signed>(x - other.x)),
				.y = (other.y > y ? static_cast<as_signed>(other.y - y) : -static_cast<as_signed>(y - other.y)),
				.z = (other.z > z ? static_cast<as_signed>(other.z - z) : -static_cast<as_signed>(z - other.z)),
			};
		} else {
			return vector3{other.x - x, other.y - y, other.z - z};
		}
	}

	/**
	 * @brief Calculate the vector between to this point from another.
	 *
	 * @param other Point to get a vector from
	 * @return Vector2 from the other point
	 */
	auto from(const point3& other) const noexcept {
		if constexpr (std::is_unsigned_v<T>) {
			using as_signed = std::make_signed_t<T>;

			return vector3<std::make_signed_t<T>>{
				.x = (other.x > x ? -static_cast<as_signed>(other.x - x) : static_cast<as_signed>(x - other.x)),
				.y = (other.y > y ? -static_cast<as_signed>(other.y - y) : static_cast<as_signed>(y - other.y)),
				.z = (other.z > z ? -static_cast<as_signed>(other.z - z) : static_cast<as_signed>(z - other.z)),
			};
		} else {
			return vector3{x - other.x, y - other.y, z - other.z};
		}
	}

	/**
	 * @brief Calculate the squared distance between this point and another.
	 *
	 * @param other Point to calculate the distance to
	 * @return Squared distance to the point
	 */
	template <typename U = T>
	U distance_squared(point3 const& other) const noexcept {
		if constexpr (std::is_unsigned_v<T> || std::is_unsigned_v<U>) {
			U dx;
			U dy;
			U dz;

			if (other.x > x) {
				dx = other.x - x;
			} else {
				dx = x - other.x;
			}

			if (other.y > y) {
				dy = other.y - y;
			} else {
				dy = y - other.y;
			}

			if (other.z > z) {
				dz = other.z - z;
			} else {
				dz = z - other.z;
			}

			return dx * dx + dy * dy + dz * dz;
		} else {
			U dx = other.x - x;
			U dy = other.y - y;
			U dz = other.z - z;

			return dx * dx + dy * dy + dz * dz;
		}
	}

	/**
	 * @brief Calculate the distance between this point and another.
	 *
	 * May be performance-heavy from calculating the square root, consider using distance_squared instead.
	 *
	 * @see distance_squared
	 * @param other Point to calculate the distance to
	 * @return Squared distance to the point
	 */
	template <typename U = double>
	U distance(point3 const& other) const noexcept {
		return static_cast<U>(std::sqrt(distance_squared(other)));
	}

	friend constexpr point3 operator+(point3 const& lhs, vector3<T> const& rhs) noexcept {
		return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
	}

	friend constexpr point3 operator+(vector3<T> const& lhs, point3 const& rhs) noexcept {
		return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
	}

	friend constexpr point3 operator-(point3 const& lhs, vector3<T> const& rhs) noexcept {
		return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
	}

	friend constexpr point3 operator-(vector3<T> const& lhs, point3 const& rhs) noexcept {
		return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
	}

	constexpr point3 &operator+=(vector3<T> const& rhs) noexcept {
		x += rhs.x;
		y += rhs.y;
		z += rhs.z;
		return *this;
	}

	constexpr point3 &operator-=(vector3<T> const& rhs) noexcept {
		x -= rhs.x;
		y -= rhs.y;
		z -= rhs.z;
		return *this;
	}

	/**
	 * @brief The X or "length" coordinate in 3D space.
	 */
	T x = {};

	/**
	 * @brief The Y or "height" coordinate in 3D space.
	 */
	T y = {};

	/**
	 * @brief The Z or "depth" coordinate in 3D space.
	 */
	T z = {};
};

template <typename T>
struct rect {
	/**
	 * @brief Top left point.
	 */
	point2<T> top_left = {};

	/**
	 * @brief Bottom right point.
	 */
	point2<T> bottom_right = {};

	/**
	 * @brief Calculate the dimensions of the rectangle.
	 */
	dimensions2<T> get_dimensions() const noexcept {
		SHION_ASSERT(bottom_right.x > top_left.x);
		SHION_ASSERT(bottom_right.y > top_left.y);

		return {.width = bottom_right.x - top_left.x, .height = bottom_right.y - top_left.y};
	}

	rect shrink(double factor) const noexcept {
		dimensions2<T> dimensions = get_dimensions();
		dimensions2<T> after = dimensions * factor;
		T dxhalf = (after.width - dimensions.width) / 2;
		T dyhalf = (after.height - dimensions.height) / 2;

		return {
			.top_left = {.x = top_left.x + dxhalf, .y = top_left.y + dyhalf},
			.bottom_right = {.x = bottom_right.x - dxhalf, .y = bottom_right.y - dxhalf}
		};
	}
};

template <typename T>
struct box {
	/**
	 * @brief Top left front point.
	 */
	point3<T> top_left_front = {};

	/**
	 * @brief Bottom right behind point.
	 */
	point3<T> bottom_right_behind = {};

	/**
	 * @brief Calculate the dimensions of the box.
	 */
	dimensions3<T> get_dimensions() const noexcept {
		SHION_ASSERT(bottom_right_behind.x > top_left_front.x);
		SHION_ASSERT(bottom_right_behind.y < top_left_front.y);
		SHION_ASSERT(bottom_right_behind.z > top_left_front.z);

		return {.width = bottom_right_behind.x - top_left_front.x, .height = top_left_front.y - bottom_right_behind.y, .depth = bottom_right_behind.z - top_left_front.z};
	}
};

#undef SHION_DEFINE_TUPLE_ACCESS

using vector2s = vector2<short>;
using vector2i = vector2<int>;
using vector2l = vector2<long>;
using vector2ll = vector2<long long>;

using vector2us = vector2<ushort>;
using vector2u = vector2<uint>;
using vector2ul = vector2<ulong>;
using vector2ull = vector2<ull>;

using vector2i16 = vector2<int16>;
using vector2i32 = vector2<int32>;
using vector2i64 = vector2<int64>;
using vector2u16 = vector2<uint16>;
using vector2u32 = vector2<uint32>;
using vector2u64 = vector2<uint64>;

using vector2f = vector2<float>;
using vector2d = vector2<double>;

using vector3s = vector3<short>;
using vector3i = vector3<int>;
using vector3l = vector3<long>;
using vector3ll = vector3<long long>;

using vector3us = vector3<ushort>;
using vector3u = vector3<uint>;
using vector3ul = vector3<ulong>;
using vector3ull = vector3<ull>;

using vector3i16 = vector3<int16>;
using vector3i32 = vector3<int32>;
using vector3i64 = vector3<int64>;
using vector3u16 = vector3<uint16>;
using vector3u32 = vector3<uint32>;
using vector3u64 = vector3<uint64>;

using vector3f = vector3<float>;
using vector3d = vector3<double>;

}

}

#endif