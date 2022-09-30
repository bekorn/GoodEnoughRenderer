#pragma once

#include "Lib/core/core.hpp"
#include "Lib/core/named.hpp"
#include "Lib/opengl/core.hpp"
#include "Lib/opengl/glsl.hpp"
#include "Lib/asset_kitchen/glsl/program/.hpp"
#include "Lib/asset_kitchen/glsl/uniform_block/.hpp"
#include "Lib/asset_kitchen/gltf/.hpp"

struct Descriptions
{
	Managed<GLSL::Program::Description> glsl;
	Managed<GLSL::UniformBlock::Description> uniform_block;
	Managed<GLTF::Description> gltf;

	void create(std::filesystem::path const & project_root)
	{
		auto asset_decription_path = project_root / "assets.json";
		if (std::filesystem::exists(asset_decription_path))
		{
			using namespace rapidjson;
			using namespace File::JSON;

			Document document;
			document.Parse(File::LoadAsString(asset_decription_path).c_str());

			if (auto const member = document.FindMember("glsl_uniform_block"); member != document.MemberEnd())
				for (auto const & item: member->value.GetArray())
				{
					auto [name, description] = GLSL::UniformBlock::Parse(item.GetObject(), project_root);
					uniform_block.generate(name, description);
				}

			if (auto const member = document.FindMember("glsl_program"); member != document.MemberEnd())
				for (auto const & item: member->value.GetArray())
				{
					auto [name, description] = GLSL::Program::Parse(item.GetObject(), project_root);
					glsl.generate(name, description);
				}

			if (auto const member = document.FindMember("gltf"); member != document.MemberEnd())
				for (auto const & item: member->value.GetArray())
				{
					auto [name, description] = GLTF::Parse(item.GetObject(), project_root);
					gltf.generate(name, description);
				}
		}
	}
};