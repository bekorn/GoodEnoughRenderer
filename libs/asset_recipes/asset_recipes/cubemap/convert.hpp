#pragma once

#include <core/named.hpp>
#include <file_io/core.hpp>
#include <file_io/json_utils.hpp>
#include <opengl/texture_cubemap.hpp>

namespace Cubemap
{
inline const char * const ASSET_NAME = "cubemap";

GL::TextureCubemap Convert(LoadedData const & loaded);
std::pair<Name, Desc> Parse(File::JSON::ConstObj o, std::filesystem::path const & root_dir);
}