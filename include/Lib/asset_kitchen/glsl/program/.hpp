#pragma once
#pragma message("----Read ASSET/PROGRAM/.Hpp----")

#include "Lib/core/core.hpp"
#include "Lib/core/expected.hpp"
#include "Lib/file_management/core.hpp"
#include "Lib/file_management/json_utils.hpp"
#include "Lib/opengl/glsl.hpp"
#include "Lib/opengl/shader.hpp"

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

	Expected<GL::ShaderProgram, std::string> Convert(LoadedData const & loaded);

	std::pair<Name, Description> Parse(File::JSON::JSONObj o, std::filesystem::path const & root_dir);
}