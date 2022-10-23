#pragma once
#pragma message("-- read ASSET/BLOCK/convert.Hpp --")

#include "load.hpp"

#include "Lib/core/expected.hpp"
#include "Lib/file_management/core.hpp"
#include "Lib/file_management/json_utils.hpp"
#include "Lib/opengl/uniform_block.hpp"

namespace GLSL::UniformBlock
{
	Expected<GL::UniformBlock, std::string> Convert(LoadedData const & loaded);
	std::pair<Name, Description> Parse(File::JSON::JSONObj o, std::filesystem::path const & root_dir);
}