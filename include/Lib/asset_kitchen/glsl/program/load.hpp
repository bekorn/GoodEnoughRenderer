#pragma once
#pragma message("----Read ASSET/PROGRAM/load.Hpp----")

#include "Lib/core/core.hpp"
#include "Lib/opengl/core.hpp"

namespace GLSL::Program
{
	struct Stage
	{
		GL::GLenum type;
		std::string source;
	};

	struct Description
	{
		struct Stage
		{
			GL::GLenum stage;
			std::filesystem::path path;
		};
		vector<Stage> stages;
		// TODO(bekorn): The order will be important in a case in the future and this api will fail then :/
		vector<std::string_view> include_strings;
		vector<std::filesystem::path> include_paths;
	};

	struct LoadedData
	{
		vector<Stage> stages;
		vector<std::string> includes;
	};

	LoadedData Load(Description const & description);
}
