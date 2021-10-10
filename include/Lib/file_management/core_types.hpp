#pragma once

#include <span>

struct Buffer
{
	std::unique_ptr<byte> data;
	usize size;

	explicit Buffer(usize size) :
		data(new byte[size]),
		size(size)
	{}

	template<typename T>
	std::span<T> span_as() const
	{
		return {
			reinterpret_cast<T*>(data.get()),
			reinterpret_cast<T*>(data.get() + size)
		};
	}

	template<typename T>
	T* data_as() const
	{
		return reinterpret_cast<T*>(data.get());
	}
};