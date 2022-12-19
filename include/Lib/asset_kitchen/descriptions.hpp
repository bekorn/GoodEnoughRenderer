#pragma once

#include "Lib/core/core.hpp"
#include "Lib/core/named.hpp"

#include "glsl/program/load.hpp"
#include "glsl/uniform_block/load.hpp"
#include "gltf/load.hpp"
#include "texture/load.hpp"
#include "cubemap/load.hpp"
#include "envmap/load.hpp"

struct Descriptions
{
	std::filesystem::path root;
	Managed<GLSL::Program::Description> glsl;
	Managed<GLSL::UniformBlock::Description> uniform_block;
	Managed<GLTF::Description> gltf;
	Managed<Texture::Description> texture;
	Managed<Cubemap::Description> cubemap;
	Managed<Envmap::Description> envmap;

	void create(std::filesystem::path const & project_root);
};