#pragma once

#include "descriptions.hpp"

#include "Lib/core/core.hpp"
#include "Lib/core/named.hpp"
#include "Lib/opengl/texture.hpp"
#include "Lib/render/mesh.hpp"
#include "Lib/render/material.hpp"
#include "Lib/scene/core.hpp"

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
		auto all_uniform_blocks_laoded = true;
		for (auto const & [name, _] : descriptions.uniform_block)
			all_uniform_blocks_laoded &= load_glsl_uniform_block(name);
		if (not all_uniform_blocks_laoded)
			std::exit(1);

		for (auto const & [name, _] : descriptions.glsl)
			load_glsl_program(name);

		for (auto const & [name, _] : descriptions.gltf)
			load_gltf(name);
	}

	bool load_glsl_program(Name const & name);
	bool load_glsl_uniform_block(Name const & name);
	void load_gltf(Name const & name);
	// For rapid editing purposes
	bool reload_glsl_program(Name const & name);
};