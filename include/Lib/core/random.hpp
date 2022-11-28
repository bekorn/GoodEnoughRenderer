#pragma once

#include "core.hpp"

struct Random
{
	template<typename T>
	requires std::integral<T> || std::floating_point<T>
	static T next(T const & min, T const & max)
	{
		static thread_local std::mt19937_64 generator(
			DEBUG ? 1337 : std::chrono::steady_clock::now().time_since_epoch().count()
		);

		if constexpr (std::is_integral_v<T>)
			return std::uniform_int_distribution<T>(min, max)(generator);
		else
			return std::uniform_real_distribution<T>(min, max)(generator);
	}

	template<typename Vec>
	static Vec next(Vec::value_type const & min, Vec::value_type const & max)
	{
		Vec result;
		for (auto i = 0; i < Vec::length(); ++i)
			result[i] = next(min, max);
		return result;
	}

	template<glm::length_t L, typename T, glm::qualifier Q>
	static glm::vec<L, T, Q> next(glm::vec<L, T, Q> const & min, glm::vec<L, T, Q> const & max)
	{
		glm::vec<L, T, Q> result;
		for (auto i = 0; i < L; ++i)
			result[i] = next(min[i], max[i]);
		return result;
	}

	static f32x3 next_in_unit_cube()
	{
		return next<f32x3>(-1, 1);
	}

	// for the next 2 functions see https://datagenetics.com/blog/january32020/index.html
	static f32x3 next_in_unit_sphere()
	{
		// TODO(bekorn): compare with the discarding method

		auto theta = next(f32(0), glm::two_pi<f32>());
		auto phi = acos(next<f32>(-1, 1));
		auto r = pow(next<f32>(0, 1), 1.f / 3.f);
		auto x = r * sin(phi) * cos(theta);
		auto y = r * sin(phi) * sin(theta);
		auto z = r * cos(phi);
		return {x, y, z};
	}

	static f32x3 next_on_unit_sphere()
	{
		// return normalize(next<f32x3>(-1, 1));
		return normalize(next_in_unit_sphere());
	}
};
