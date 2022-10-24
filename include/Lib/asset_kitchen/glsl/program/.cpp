#pragma message("-- read ASSET/PROGRAM/.Cpp --")

#include "load.hpp"
#include "convert.hpp"
#include "../_helpers.hpp"

namespace GLSL::Program
{
	LoadedData Load(Description const & description)
	{
		LoadedData loaded;

		loaded.stages.reserve(description.stages.size());
		for (auto const & [stage, path]: description.stages)
			loaded.stages.emplace_back(Stage{
				.type = stage,
				.source = File::LoadAsString(path),
			});

		loaded.includes.reserve(description.include_paths.size() + description.include_strings.size());
		for (auto const & sv: description.include_strings)
			loaded.includes.emplace_back(sv);
		for (auto const & path: description.include_paths)
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

			stage.create(
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


		vector<GL::ShaderStage const*> stages_description;
		stages_description.reserve(stages.size());
		for(auto & s: stages)
			stages_description.emplace_back(&s);

		GL::ShaderProgram program;
		program.create(
			{
				.shader_stages = stages_description
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

	std::pair<Name, Description> Parse(File::JSON::JSONObj o, std::filesystem::path const & root_dir)
	{
		using namespace Helpers;

		auto name = o.FindMember("name")->value.GetString();
		GLSL::Program::Description description;

		for (auto & item : o.FindMember("stages")->value.GetObject())
			description.stages.push_back({
				.stage = ToGLenum(item.name.GetString()),
				.path = root_dir / item.value.GetString(),
			});

		if (auto member = o.FindMember("include_paths"); member != o.MemberEnd())
			for (auto & item: member->value.GetArray())
				description.include_paths.push_back(root_dir / item.GetString());

		description.include_strings.push_back(GL::GLSL_VERSION_MACRO);
		if (auto member = o.FindMember("include_strings"); member != o.MemberEnd())
			for (auto & item: member->value.GetArray())
				description.include_strings.emplace_back(item.GetString());

		return {name, description};
	}
}