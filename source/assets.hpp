#pragma once

#include "Lib/core/core.hpp"
#include "Lib/opengl/.hpp"
#include "Lib/render/.hpp"
#include "Lib/asset_kitchen/glsl/.hpp"
#include "Lib/asset_kitchen/gltf/.hpp"

#include "globals.hpp"

struct Assets
{
	// GL resources
	GL::ShaderProgram program; // will be a vector<shader> later
	vector<GL::Texture2D> textures;
	// Render resources
	vector<Geometry::Primitive> primitives;
	vector<unique_ptr<Render::IMaterial>> materials;
	vector<Render::Mesh> meshes;

	CTOR(Assets, default)
	COPY(Assets, delete)
	MOVE(Assets, delete)

	void create()
	{
		load_program();
		load_gltf_assets();
	}

	optional<std::string> load_program()
	{
		auto const glsl_data = GLSL::Load(
			{
				.stages = {
					{GL::GL_VERTEX_SHADER,   global_state.test_assets / "gltf_pbrMetallicRoughness.vert.glsl"},
					{GL::GL_FRAGMENT_SHADER, global_state.test_assets / "gltf_pbrMetallicRoughness.frag.glsl"},
				},
				.include_paths = {},
				.include_strings = {
					GL::GLSL_VERSION_MACRO,
				}
			}
		);

		if (auto expected = GLSL::Convert(glsl_data))
		{
			program = expected.into_result();
			program.update_interface_mapping();
			GL::glUseProgram(program.id);
			return nullopt;
		}
		else
		{
			return expected.into_error();
		}
	}

	void load_gltf_assets()
	{
		auto const gltf_data = GLTF::Load(global_state.test_assets / "helmet/DamagedHelmet.gltf");
//		auto const gltf_data = GLTF::Load(global_state.test_assets / "avocado/Avocado.gltf");
//		auto const gltf_data = GLTF::Load(global_state.test_assets / "electric_guitar_fender_strat_plus/model.gltf");
//		auto const gltf_data = GLTF::Load(global_state.test_assets / "sponza/Sponza.gltf");
//		auto const gltf_data = GLTF::Load(global_state.test_assets / "flight_helmet/FlightHelmet.gltf");

		GLTF::Convert(gltf_data, textures, materials, primitives, meshes);
	}
};