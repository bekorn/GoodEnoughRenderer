#pragma once

#include <chrono>
#include <iostream>
#include <iomanip>
#include <source_location>

#include "core.hpp"

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
		std::chrono::microseconds cpu;
		std::chrono::microseconds wall;
	};

	TimeElapsed timeit()
	{
		auto cpu_now = cpu_clock::now();
		auto cpu_diff = std::chrono::duration_cast<std::chrono::microseconds>(cpu_now - cpu_last);

		auto wall_now = wall_clock::now();
		auto wall_diff = std::chrono::duration_cast<std::chrono::microseconds>(wall_now - wall_last);

		cpu_last = cpu_now;
		wall_last = wall_now;

		return {cpu_diff, wall_diff};
	};

	void timeit(std::ostream & out, std::string_view tag)
	{
		auto time_elapsed = timeit();

		out << std::right
			<< std::setw(32) << tag
			<< " | CPU:" << std::setw(16) << time_elapsed.cpu
			<< " | Wall:" << std::setw(16) << time_elapsed.wall
			<< '\n' << std::left;
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
void log(std::string_view message, std::source_location const location = std::source_location::current())
{
	std::cout
		<< "LOG "
		<< location.file_name() << "(" << location.line() << ") " << location.function_name() << ':'
		<< message
		<< '\n';
}
