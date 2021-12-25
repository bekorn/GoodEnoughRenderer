#pragma once

#include "core.hpp"

template<typename C>
concept Container =
requires(C c)
{
	c.data();
	c.begin();
	c.end();
	std::next(c.begin());
};

template<Container C>
struct index_ptr
{
	C const & container;
	usize const index;

	index_ptr(C const & container, usize index) :
		container(container), index(index)
	{}

	index_ptr(C const & container, decltype(container.data()) item_ptr) :
		container(container), index(item_ptr - container.data())
	{}

	auto& operator*() const
	{
		return *std::next(container.begin(), index);
	}

	auto operator->() const
	{
		return std::next(container.begin(), index);
	}
};