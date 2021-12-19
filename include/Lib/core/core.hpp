#pragma once

#include ".pch.hpp"

using byte = std::byte;

using i8 = glm::i8;
using i16 = glm::i16;
using i32 = glm::i32;
using i64 = glm::i64;
using u8 = glm::u8;
using u16 = glm::u16;
using u32 = glm::u32;
using u64 = glm::u64;

using i8x2 = glm::i8vec2;
using i16x2 = glm::i16vec2;
using i32x2 = glm::i32vec2;
using i64x2 = glm::i64vec2;
using u8x2 = glm::u8vec2;
using u16x2 = glm::u16vec2;
using u32x2 = glm::u32vec2;
using u64x2 = glm::u64vec2;

using i8x3 = glm::i8vec3;
using i16x3 = glm::i16vec3;
using i32x3 = glm::i32vec3;
using i64x3 = glm::i64vec3;
using u8x3 = glm::u8vec3;
using u16x3 = glm::u16vec3;
using u32x3 = glm::u32vec3;
using u64x3 = glm::u64vec3;

using i8x4 = glm::i8vec4;
using i16x4 = glm::i16vec4;
using i32x4 = glm::i32vec4;
using i64x4 = glm::i64vec4;
using u8x4 = glm::u8vec4;
using u16x4 = glm::u16vec4;
using u32x4 = glm::u32vec4;
using u64x4 = glm::u64vec4;

// x64 preferred sizes
#if defined(_M_X64) || defined(__x86_64__)
using usize = std::size_t;
using ifast = signed;
using ufast = unsigned;
#endif

using f32 = glm::f32;
using f64 = glm::f64;

using f32x2 = glm::f32vec2;
using f64x2 = glm::f64vec2;

using f32x3 = glm::f32vec3;
using f64x3 = glm::f64vec3;

using f32x4 = glm::f32vec4;
using f64x4 = glm::f64vec4;

using f32x4x4 = glm::f32mat4x4;
using f64x4x4 = glm::f64mat4x4;

using f32x3x3 = glm::f32mat3x3;
using f64x3x3 = glm::f64mat3x3;


// Handy std stuff
using std::move;
using std::vector, std::array, std::span;
using std::optional, std::nullopt;
using std::variant;
using std::unique_ptr, std::make_unique;


// Simple byte buffer
struct ByteBuffer
{
	unique_ptr<byte[]> data;
	usize size;

	ByteBuffer() = default;

	explicit ByteBuffer(usize size) :
		data(make_unique<byte[]>(size)),
		size(size)
	{}

	// move a pointer
	ByteBuffer(void* && pointer, usize size) :
		data(static_cast<byte*>(pointer)),
		size(size)
	{
		pointer = nullptr;
	}

	template<typename T>
	auto span_as() const
	{
		return span<T>(
			reinterpret_cast<T*>(data.get()),
			reinterpret_cast<T*>(data.get() + size)
		);
	}

	template<typename T>
	auto span_as(usize offset, usize size) const
	{
		assert(("Out of bounds access", offset + size <= this->size));
		return span<T>(
			reinterpret_cast<T*>(data.get() + offset),
			reinterpret_cast<T*>(data.get() + offset + size)
		);
	}

	template<typename T>
	auto data_as() const
	{
		return reinterpret_cast<T*>(data.get());
	}
};
