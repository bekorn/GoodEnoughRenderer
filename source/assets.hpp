#pragma once

#include "Lib/core/core.hpp"
#include "Lib/opengl/.hpp"
#include "Lib/render/.hpp"
#include "Lib/scene/.hpp"
#include "Lib/asset_kitchen/glsl/.hpp"
#include "Lib/asset_kitchen/gltf/.hpp"

#include "descriptions.hpp"

struct Assets
{
	Desriptions const & desriptions;

	// GL resources
	Managed<GL::ShaderProgram> programs;
	Managed<std::string> program_errors;
	Managed<GL::Texture2D> textures;
	// Render resources
	Managed<Geometry::Primitive> primitives;
	Managed<unique_one<Render::IMaterial>> materials;
	Managed<Render::Mesh> meshes;
	// Scene resources
	Scene::Tree scene_tree;

	explicit Assets(Desriptions const & desriptions) :
		desriptions(desriptions)
	{}

	COPY(Assets, delete)
	MOVE(Assets, delete)

	void create()
	{
		load_glsl(GLTF::pbrMetallicRoughness_program_name);
		load_gltf("Sponza"_name);
	}

	void load_glsl(Name const & name)
	{
		auto const glsl_data = GLSL::Load(desriptions.glsl.get(name));

		if (auto expected = GLSL::Convert(glsl_data))
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

	void load_gltf(Name const & name)
	{
		auto const gltf_data = GLTF::Load(desriptions.gltf.get(name));
		GLTF::Convert(gltf_data, textures, materials, primitives, meshes, scene_tree);
	}
};