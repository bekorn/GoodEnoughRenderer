#pragma once

#include "Lib/core/core.hpp"
#include "Lib/core/named.hpp"

#include "glsl/program/load.hpp"
#include "glsl/uniform_block/load.hpp"
#include "gltf/load.hpp"
#include "texture/load.hpp"
#include "cubemap/load.hpp"

struct Descriptions
{
	Managed<GLSL::Program::Description> glsl;
	Managed<GLSL::UniformBlock::Description> uniform_block;
	Managed<GLTF::Description> gltf;
	Managed<Texture::Description> texture;
	Managed<Cubemap::Description> cubemap;

	void create(std::filesystem::path const & project_root);
};