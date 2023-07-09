#pragma once

#include <core/named.hpp>
#include <file_io/core.hpp>
#include <file_io/json_utils.hpp>
#include <opengl/texture_cubemap.hpp>

namespace Envmap
{
inline const char * const ASSET_NAME = "envmap";

std::pair<Name, Desc> Parse(File::JSON::ConstObj o, std::filesystem::path const & root_dir);
void Convert(LoadedData const & loaded, Name const & name, Managed <GL::TextureCubemap> & cubemaps);
}