#pragma once

#include "Lib/core/core.hpp"
#include "Lib/core/named.hpp"
#include "Lib/asset_kitchen/glsl/program/load.hpp"
#include "Lib/asset_kitchen/glsl/uniform_block/load.hpp"
#include "Lib/asset_kitchen/gltf/load.hpp"

struct Descriptions
{
	Managed<GLSL::Program::Description> glsl;
	Managed<GLSL::UniformBlock::Description> uniform_block;
	Managed<GLTF::Description> gltf;

	void create(std::filesystem::path const & project_root);
};