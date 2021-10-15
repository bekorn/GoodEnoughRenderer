#pragma once

#include ".pch.hpp"
#include "convention.hpp"

#include <sstream>

namespace GL
{
	// TODO(bekorn): this and the ImGui init one should be the same thing
	auto const GLSL_VERSION_MACRO = "#version 430\n";

	std::string const & GET_GLSL_ATTRIBUTE_LOCATION_MACROS()
	{
		// TODO(bekorn): utilize constexpr string
		static bool computed = false;
		static std::string macros;

		if (computed)
			return macros;


		std::stringstream out;
		for (auto i = 0; i < ATTRIBUTE_LOCATION::SIZE; ++i)
			out << "#define ATTRIBUTE_LOCATION_" << ATTRIBUTE_LOCATION::ToString(i) << ' ' << i << '\n';
		macros = out.str();

		return macros;
	}


}
