#pragma message("-- read ASSET/PROGRAM/.Cpp --")

#include "load.hpp"
#include "convert.hpp"
#include "../_helpers.hpp"

namespace GLSL::Program
{
LoadedData Load(Desc const & desc)
{
	LoadedData loaded;

	loaded.stages.reserve(desc.stages.size());
	for (auto const & [stage, path]: desc.stages)
		loaded.stages.emplace_back(Stage{
			.type = stage,
			.source = File::LoadAsString(path),
		});

	loaded.includes.reserve(desc.include_paths.size() + desc.include_strings.size());
	for (auto const & sv: desc.include_strings)
		loaded.includes.emplace_back(sv);
	for (auto const & path: desc.include_paths)
		loaded.includes.emplace_back(File::LoadAsString(path));

	return loaded;
}

Expected<GL::ShaderProgram, std::string> Convert(LoadedData const & loaded)
{
	using namespace Helpers;

	std::stringstream error_log;

	vector<const char*> includes;
	includes.resize(loaded.includes.size() + 2); // +1 for the #line, +1 for the stage source
	for (auto i = 0; i < loaded.includes.size(); ++i)
		includes[i] = loaded.includes[i].data();

	includes[includes.size() - 2] = "#line 1\n";

	bool all_stages_compiled = true;

	vector<GL::ShaderStage> stages;
	stages.resize(loaded.stages.size());
	for (auto i = 0; i < stages.size(); ++i)
	{
		auto & stage = stages[i];
		auto const & data_stage = loaded.stages[i];

		includes.back() = data_stage.source.data(); // put stage source as the last one

		stage.init(
			{
				.stage = data_stage.type,
				.sources = includes,
			}
		);

		if (not is_compiled(stage))
		{
			all_stages_compiled = false;
			error_log << IntoString(data_stage.type) << ": " << get_log(stage);
		}
	}

	if (not all_stages_compiled)
		return {error_log.str()};


	vector<GL::ShaderStage const*> stages_descs;
	stages_descs.reserve(stages.size());
	for(auto & s: stages)
		stages_descs.emplace_back(&s);

	GL::ShaderProgram program;
	program.init(
		{
			.shader_stages = stages_descs
		}
	);

	if (not is_linked(program))
	{
		error_log << "Compilation failed! Linking Error:\n" << get_log(program);
		return {error_log.str()};
	}

	set_attribute_mapping(program);
	set_uniform_mapping(program);
	set_uniform_block_mapping(program);
	set_storage_block_mapping(program);
	return {move(program)};
}

namespace Helpers
{
GL::GLenum ToGLenum(std::string_view stage)
{
	if (stage == "vert") return GL::GL_VERTEX_SHADER;
	if (stage == "frag") return GL::GL_FRAGMENT_SHADER;
	if (stage == "geom") return GL::GL_GEOMETRY_SHADER;
	if (stage == "comp") return GL::GL_COMPUTE_SHADER;
}
}

std::pair<Name, Desc> Parse(File::JSON::JSONObj o, std::filesystem::path const & root_dir)
{
	using namespace Helpers;

	auto name = o.FindMember("name")->value.GetString();
	GLSL::Program::Desc desc;

	for (auto & item : o.FindMember("stages")->value.GetArray())
	{
		const char * const path = item.GetString();

		// assert path == <path>/<name>.<stage>.glsl
		auto stage = std::string_view(path);
		stage.remove_suffix(stage.size() - stage.rfind('.'));
		stage = stage.substr(stage.rfind('.') + 1);

		desc.stages.push_back({
			.stage = ToGLenum(stage),
			.path = root_dir / path,
		});
	}

	if (auto member = o.FindMember("include_paths"); member != o.MemberEnd())
		for (auto & item: member->value.GetArray())
			desc.include_paths.push_back(root_dir / item.GetString());

	desc.include_strings.push_back(GL::GLSL_VERSION_MACRO);
	desc.include_strings.push_back(GL::GLSL_COMMON_EXTENSIONS);
	if (auto member = o.FindMember("include_strings"); member != o.MemberEnd())
		for (auto & item: member->value.GetArray())
			desc.include_strings.emplace_back(item.GetString());

	return {name, desc};
}
}