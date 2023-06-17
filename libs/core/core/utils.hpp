#pragma once

#include "core.hpp"

template<typename Duration = std::chrono::microseconds>
struct Timer
{
	// https://en.cppreference.com/w/cpp/chrono/high_resolution_clock see Notes paragraph 2
	using cpu_clock = std::chrono::steady_clock;
	using wall_clock = std::chrono::system_clock;

	cpu_clock::time_point cpu_last;
	wall_clock::time_point wall_last;

	Timer() :
		cpu_last(cpu_clock::now()),
		wall_last(wall_clock::now())
	{}

	struct TimeElapsed
	{
		Duration cpu;
		Duration wall;
	};

	TimeElapsed timeit()
	{
		auto cpu_now = cpu_clock::now();
		auto cpu_diff = std::chrono::duration_cast<Duration>(cpu_now - cpu_last);

		auto wall_now = wall_clock::now();
		auto wall_diff = std::chrono::duration_cast<Duration>(wall_now - wall_last);

		cpu_last = cpu_now;
		wall_last = wall_now;

		return {cpu_diff, wall_diff};
	};

	void timeit(FILE * out, std::string_view tag)
	{
		auto time_elapsed = timeit();

		fmt::print(
			out,
			"{:>32} | CPU: {:>16} | Wall: {:>16}\n",
			tag, time_elapsed.cpu, time_elapsed.wall
		);
	}
};


// use this iterator to iterate an array of pointers as references
template<typename T>
struct PointerIterator
{
	T ** current;

	void operator++()
	{ current++; }

	T & operator*()
	{ return **current; }

	bool operator!=(PointerIterator const & other)
	{ return current != other.current; }
};

// TODO(bekorn): this is demonstrative but still much better than depending on macros
inline void log(std::string_view message, std::source_location const location = std::source_location::current())
{
	fmt::print(
		"LOG {} ({}) {}:{}\n",
		location.file_name(), location.line(), location.function_name(), message
	);
}
