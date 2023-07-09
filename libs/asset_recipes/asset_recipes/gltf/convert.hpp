#pragma once
#pragma message("-- read ASSET/GLTF/convert.Hpp --")

#include "load.hpp"

#include <core/core.hpp>
#include <core/named.hpp>
#include <file_io/core.hpp>
#include <file_io/json_utils.hpp>
#include <opengl/texture_2d.hpp>
#include <render/material.hpp>
#include <render/mesh.hpp>
#include <render/scene.hpp>

namespace GLTF
{
inline const char * const ASSET_NAME = "gltf";

void Convert(
	LoadedData const & loaded,
	Managed<GL::Texture2D> & textures,
	Managed<unique_one<Render::IMaterial>> & materials,
	Managed<Render::Mesh> & meshes,
	::Scene::Tree & scene_tree,
	Managed<Geometry::Layout> const & attrib_layouts
);

std::pair<Name, Desc> Parse(File::JSON::ConstObj o, std::filesystem::path const & root_dir);
}