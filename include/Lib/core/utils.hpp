#pragma once

#include <chrono>
#include <iostream>
#include <iomanip>

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

	void timeit(std::string_view tag)
	{
		auto cpu_now = cpu_clock::now();
		auto cpu_diff = std::chrono::duration_cast<std::chrono::microseconds>(cpu_now - cpu_last);
		auto wall_now = wall_clock::now();
		auto wall_diff = std::chrono::duration_cast<std::chrono::microseconds>(wall_now - wall_last);

		std::cout << std::right
				  << std::setw(32) << tag
				  << " | CPU:" << std::setw(16) << cpu_diff
				  << " | Wall:" << std::setw(16) << wall_diff
				  << '\n' << std::left;

		cpu_last = cpu_now;
		wall_last = wall_now;
	};
};