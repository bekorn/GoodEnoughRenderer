#pragma once

#include <sstream>

#include "convention.hpp"

namespace GL
{
	auto const GLSL_VERSION_MACRO = [](){
		return "#version " + std::to_string(VERSION_MAJOR) + std::to_string(VERSION_MINOR) + "0\n";
	}();

	// TODO(bekorn): This looks better but still computed at runtime
	auto const GLSL_ATTRIBUTE_LOCATION_MACROS = []()
	{
		std::stringstream out;
		for (auto i = 0; i < ATTRIBUTE_LOCATION::SIZE; ++i)
			out << "#define ATTRIBUTE_LOCATION_" << ATTRIBUTE_LOCATION::ToString(i) << ' ' << i << '\n';
		return out.str();
	}();
}
