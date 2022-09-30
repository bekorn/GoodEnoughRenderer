#pragma once
#pragma message("----Read ASSET/BLOCK/.Hpp----")

#include "Lib/core/expected.hpp"
#include "Lib/file_management/core.hpp"
#include "Lib/file_management/json_utils.hpp"
#include "Lib/opengl/uniform_block.hpp"

namespace GLSL::UniformBlock
{
	struct LoadedData
	{
		std::string source;
	};

	struct Description
	{
		std::filesystem::path path;
	};

	LoadedData Load(Description const & description);
	Expected<GL::UniformBlock, std::string> Convert(LoadedData const & loaded);
	std::pair<Name, Description> Parse(File::JSON::JSONObj o, std::filesystem::path const & root_dir);
}