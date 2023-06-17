#pragma once

#include <core/named.hpp>
#include "Lib/file_management/core.hpp"
#include "Lib/file_management/json_utils.hpp"
#include "Lib/opengl/texture_2d.hpp"

namespace Texture
{
GL::Texture2D Convert(LoadedData const & loaded);
std::pair<Name, Desc> Parse(File::JSON::JSONObj o, std::filesystem::path const & root_dir);
}