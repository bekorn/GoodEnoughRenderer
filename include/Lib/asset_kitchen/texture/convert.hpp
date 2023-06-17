#pragma once

#include <core/named.hpp>
#include <file_io/core.hpp>
#include <file_io/json_utils.hpp>
#include <opengl/texture_2d.hpp>

namespace Texture
{
GL::Texture2D Convert(LoadedData const & loaded);
std::pair<Name, Desc> Parse(File::JSON::JSONObj o, std::filesystem::path const & root_dir);
}