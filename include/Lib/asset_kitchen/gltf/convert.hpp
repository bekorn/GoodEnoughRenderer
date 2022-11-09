#pragma once
#pragma message("-- read ASSET/GLTF/convert.Hpp --")

#include "load.hpp"

#include "Lib/core/core.hpp"
#include "Lib/core/named.hpp"
#include "Lib/file_management/core.hpp"
#include "Lib/file_management/json_utils.hpp"
#include "Lib/opengl/texture_2d.hpp"
#include "Lib/render/material.hpp"
#include "Lib/render/mesh.hpp"
#include "Lib/scene/core.hpp"

namespace GLTF
{
	void Convert(
		LoadedData const & loaded,
		Managed<GL::Texture2D> & textures,
		Managed<unique_one<Render::IMaterial>> & materials,
		Managed<Geometry::Primitive> & primitives,
		Managed<Render::Mesh> & meshes,
		::Scene::Tree & scene_tree
	);

	std::pair<Name, Description> Parse(File::JSON::JSONObj o, std::filesystem::path const & root_dir);
}