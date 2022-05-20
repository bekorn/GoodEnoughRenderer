#pragma once

#include "Lib/core/core.hpp"
#include "Lib/opengl/.hpp"
#include "Lib/render/.hpp"
#include "Lib/scene/.hpp"
#include "Lib/asset_kitchen/glsl/program/.hpp"
#include "Lib/asset_kitchen/gltf/.hpp"

#include "descriptions.hpp"

struct Assets
{
	Descriptions const & descriptions;

	// GL resources
	Managed<GL::UniformBlock> uniform_blocks;
	Managed<GL::ShaderProgram> programs;
	Managed<std::string> program_errors;
	Managed<GL::Texture2D> textures;
	// Render resources
	Managed<Geometry::Primitive> primitives;
	Managed<unique_one<Render::IMaterial>> materials;
	Managed<Render::Mesh> meshes;
	// Scene resources
	Scene::Tree scene_tree;

	explicit Assets(Descriptions const & desriptions) :
		descriptions(desriptions)
	{}

	COPY(Assets, delete)
	MOVE(Assets, delete)

	void create()
	{
		for (auto const & [name, _] : descriptions.uniform_block)
			load_glsl_uniform_block(name);

		for (auto const & [name, _] : descriptions.glsl)
			load_glsl_program(name);

		for (auto const & [name, _] : descriptions.gltf)
			load_gltf(name);
	}

	void load_glsl_program(Name const & name)
	{
		auto const loaded_data = GLSL::Program::Load(descriptions.glsl.get(name));

		if (auto expected = GLSL::Program::Convert(loaded_data))
		{
			programs.get_or_generate(name) = expected.into_result();
			program_errors.get_or_generate(name) = {};
		}
		else
		{
			programs.get_or_generate(name) = {};
			program_errors.get_or_generate(name) = expected.into_error();
		}
	}

	void load_glsl_uniform_block(Name const & name)
	{
		auto const loaded_data = GLSL::UniformBlock::Load(descriptions.uniform_block.get(name));

		if (auto expected = GLSL::UniformBlock::Convert(loaded_data))
		{
			uniform_blocks.get_or_generate(name) = expected.into_result();
			program_errors.get_or_generate(name) = {};
		}
		else
		{
			uniform_blocks.get_or_generate(name) = {};
			program_errors.get_or_generate(name) = expected.into_error();
		}
	}

	void load_gltf(Name const & name)
	{
		auto const gltf_data = GLTF::Load(descriptions.gltf.get(name));
		GLTF::Convert(gltf_data, textures, materials, primitives, meshes, scene_tree);
	}
};