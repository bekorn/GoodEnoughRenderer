#pragma once
#pragma message("-- read ASSET/BLOCK/load.Hpp --")

#include "Lib/core/core.hpp"

namespace GLSL::UniformBlock
{
struct LoadedData
{
	std::string source;
};

struct Desc
{
	std::filesystem::path path;
};

LoadedData Load(Desc const & desc);
}