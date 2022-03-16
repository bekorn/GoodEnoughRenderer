#pragma once

#include "Lib/core/core.hpp"
#include "Lib/opengl/core.hpp"

namespace GLSL::Program
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