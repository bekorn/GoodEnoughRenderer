#pragma once

#include "Lib/core/named.hpp"
#include "Lib/file_management/core.hpp"
#include "Lib/file_management/json_utils.hpp"
#include "Lib/opengl/texture_cubemap.hpp"

namespace Cubemap
{
	GL::TextureCubemap Convert(LoadedData const & loaded);
	std::pair<Name, Description> Parse(File::JSON::JSONObj o, std::filesystem::path const & root_dir);
}