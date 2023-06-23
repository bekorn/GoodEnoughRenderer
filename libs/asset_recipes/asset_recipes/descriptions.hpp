#pragma once

#include <core/core.hpp>
#include <core/named.hpp>
#include <core/geometry.hpp>

#include "glsl/program/load.hpp"
#include "glsl/uniform_block/load.hpp"
#include "gltf/load.hpp"
#include "texture/load.hpp"
#include "cubemap/load.hpp"
#include "envmap/load.hpp"

struct Descriptions
{
	std::filesystem::path root;

	Managed<Geometry::Layout> attrib_layout;
	Managed<GLSL::Program::Desc> glsl;
	Managed<GLSL::UniformBlock::Desc> uniform_block;
	Managed<GLTF::Desc> gltf;
	Managed<Texture::Desc> texture;
	Managed<Cubemap::Desc> cubemap;
	Managed<Envmap::Desc> envmap;

	void init(std::filesystem::path const & project_root);
};