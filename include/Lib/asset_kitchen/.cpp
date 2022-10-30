#include "assets.hpp"
#include "descriptions.hpp"
#include "glsl/program/convert.hpp"
#include "glsl/uniform_block/convert.hpp"
#include "gltf/convert.hpp"

void Descriptions::create(std::filesystem::path const & project_root)
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

bool Assets::load_glsl_program(Name const & name)
{
	auto const loaded_data = GLSL::Program::Load(descriptions.glsl.get(name));

	if (auto expected = GLSL::Program::Convert(loaded_data))
	{
		programs.get_or_generate(name) = expected.into_result();
		program_errors.get_or_generate(name) = {};
		return true;
	}
	else
	{
		auto error = expected.into_error();
		fmt::print(stderr, "program {} failed to load. Error: {}", name.string, error);
		programs.get_or_generate(name) = {};
		program_errors.get_or_generate(name) = error;
		return false;
	}
}

bool Assets::load_glsl_uniform_block(Name const & name)
{
	auto const loaded_data = GLSL::UniformBlock::Load(descriptions.uniform_block.get(name));

	if (auto expected = GLSL::UniformBlock::Convert(loaded_data))
	{
		uniform_blocks.get_or_generate(name) = expected.into_result();
		program_errors.get_or_generate(name) = {};
		return true;
	}
	else
	{
		auto error = expected.into_error();
		fmt::print(stderr, "uniform block {} failed to load. Error: {}", name.string, error);
		program_errors.get_or_generate(name) = error;
		return false;
	}
}

void Assets::load_gltf(Name const & name)
{
	auto const gltf_data = GLTF::Load(descriptions.gltf.get(name));
	GLTF::Convert(gltf_data, textures, materials, primitives, meshes, scene_tree);
}

bool Assets::reload_glsl_program(Name const & name)
{
	auto const loaded_data = GLSL::Program::Load(descriptions.glsl.get(name));

	if (auto expected = GLSL::Program::Convert(loaded_data))
	{
		auto new_program = expected.into_result();
		auto & old_program = programs.get_or_generate(name);

		// check if all the shader mappings are the same, prevent shader interface changes on runtime
		char const * missmatch = nullptr;
		if (old_program.attribute_mappings != new_program.attribute_mappings)
			missmatch = "attribute";
		else if (old_program.uniform_mappings != new_program.uniform_mappings)
			missmatch = "uniform";
		else if (old_program.uniform_block_mappings != new_program.uniform_block_mappings)
			missmatch = "uniform_block";
		else if (old_program.storage_block_mappings != new_program.storage_block_mappings)
			missmatch = "storage_block";

		if (missmatch != nullptr)
		{
			auto error = fmt::format("{} failed to reload. Shader {} interface mismatched\n", name.string, missmatch);
			// TODO(bekorn): logging only because the asset_kitchen is not stable enough, should be removed later
			fmt::print(stderr, "{}", error);
			program_errors.get_or_generate(name) = error;
			return false;
		}

		old_program = move(new_program);
		program_errors.get_or_generate(name) = {};
		return true;
	}
	else
	{
		auto error = expected.into_error();
		// TODO(bekorn): logging only because the asset_kitchen is not stable enough, should be removed later
		fmt::print(stderr, "{} failed to reload. Error: {}\n", name.string, error);
		program_errors.get_or_generate(name) = error;
		return false;
	}
}
