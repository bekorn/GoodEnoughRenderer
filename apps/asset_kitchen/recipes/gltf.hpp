#pragma once

#include <asset_recipes/gltf/load.hpp>
#include <asset_recipes/gltf/convert.hpp>

namespace GLTF
{
inline const char * const NAME = "gltf";

optional<std::string> Serve(LoadedData const & loaded_data, std::filesystem::path path);
};