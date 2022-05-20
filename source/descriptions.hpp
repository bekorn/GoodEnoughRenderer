#pragma once

#include "Lib/core/core.hpp"
#include "Lib/core/named.hpp"
#include "Lib/opengl/core.hpp"
#include "Lib/opengl/glsl.hpp"
#include "Lib/asset_kitchen/glsl/program/loader.hpp"
#include "Lib/asset_kitchen/glsl/uniform_block/.hpp"
#include "Lib/asset_kitchen/gltf/loader.hpp"

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
			{
				for (auto const & item: member->value.GetArray())
				{
					auto name = item.FindMember("name")->value.GetString();
					GLSL::UniformBlock::Description decription{
						.path = project_root / item.FindMember("path")->value.GetString(),
					};

					uniform_block.generate(name, decription);
				}
			}

			if (auto const member = document.FindMember("glsl_program"); member != document.MemberEnd())
			{
				static auto const StringToGLEnum = [](std::string_view stage)
				{
					if (stage == "vert") return GL::GL_VERTEX_SHADER;
					if (stage == "frag") return GL::GL_FRAGMENT_SHADER;
					if (stage == "geom") return GL::GL_GEOMETRY_SHADER;
				};

				for (auto const & item: member->value.GetArray())
				{
					auto name = item.FindMember("name")->value.GetString();
					GLSL::Program::Description description;

					for (auto & item : item.FindMember("stages")->value.GetObject())
						description.stages.push_back({
							.stage = StringToGLEnum(item.name.GetString()),
							.path = project_root / item.value.GetString(),
						});

					for (auto & item : item.FindMember("include_paths")->value.GetArray())
						description.include_paths.push_back(project_root / item.GetString());

					description.include_strings.push_back(GL::GLSL_VERSION_MACRO);
					for (auto & item : item.FindMember("include_strings")->value.GetArray())
						description.include_strings.emplace_back(item.GetString());

					glsl.generate(GLTF::pbrMetallicRoughness_program_name, description);
				}
			}

			if (auto const member = document.FindMember("gltf"); member != document.MemberEnd())
			{
				for (auto const & item: member->value.GetArray())
				{
					auto name = item.FindMember("name")->value.GetString();
					GLTF::Description description{
						.name = name,
						.path = project_root / item.FindMember("path")->value.GetString(),
					};

					gltf.generate(name, description);
				}
			}
		}
	}
};