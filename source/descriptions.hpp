#pragma once

#include "Lib/core/core.hpp"
#include "Lib/core/named.hpp"
#include "Lib/opengl/core.hpp"
#include "Lib/opengl/glsl.hpp"
#include "Lib/asset_kitchen/glsl/loader.hpp"
#include "Lib/asset_kitchen/gltf/loader.hpp"

struct Desriptions
{
	Managed<GLSL::Description> glsl;
	Managed<GLTF::Description> gltf;

	void create(std::filesystem::path const & test_assets)
	{
		glsl.generate(GLTF::pbrMetallicRoughness_program_name).data = {
			.stages = {
				{GL::GL_VERTEX_SHADER,   test_assets / "gltf_pbrMetallicRoughness.vert.glsl"},
				{GL::GL_FRAGMENT_SHADER, test_assets / "gltf_pbrMetallicRoughness.frag.glsl"},
			},
			.include_paths = {},
			.include_strings = {
				GL::GLSL_VERSION_MACRO,
			}
		};

		gltf.generate("DamagedHelmet"_name).data = {
			.name = "DamagedHelmet", .path = test_assets / "damaged_helmet/DamagedHelmet.gltf"
		};
		gltf.generate("Avocado"_name).data = {
			.name = "Avocado", .path = test_assets / "avocado/Avocado.gltf"
		};
		gltf.generate("ElectricGuitar"_name).data = {
			.name = "ElectricGuitar", .path = test_assets / "electric_guitar_fender_strat_plus/model.gltf"
		};
		gltf.generate("Sponza"_name).data = {
			.name = "Sponza", .path = test_assets / "sponza/Sponza.gltf"
		};
		gltf.generate("FlightHelmet"_name).data = {
			.name = "FlightHelmet", .path = test_assets / "flight_helmet/FlightHelmet.gltf"
		};
		gltf.generate("OrientationTest"_name).data = {
			.name = "OrientationTest", .path = test_assets / "orientation test/OrientationTest.gltf"
		};
		gltf.generate("Three of Swords"_name).data = {
			.name = "Three of Swords", .path = test_assets / "Three of Swords/scene.gltf"
		};
	}
};