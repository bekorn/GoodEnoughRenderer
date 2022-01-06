#pragma once

#include "Lib/core/core.hpp"

namespace GLSL
{
	struct Stage
	{
		GL::GLenum type;
		std::string source;
	};

	struct LoadedData
	{
		vector<Stage> stages;
		vector<std::string> includes;
	};
}