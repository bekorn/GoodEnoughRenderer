#pragma once
#pragma message("----Read ASSET/BLOCK/load.Hpp----")

#include "Lib/core/core.hpp"

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
}