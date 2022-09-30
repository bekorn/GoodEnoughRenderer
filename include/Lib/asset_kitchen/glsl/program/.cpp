#include ".hpp"
#pragma message("----Read ASSET/PROGRAM/.Cpp----")

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

	namespace Helpers
	{
		std::string_view IntoString(GL::GLenum shader_stage)
		{
			using namespace std::string_view_literals;
			// https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glCreateShader.xhtml
			switch (shader_stage)
			{
			case GL::GL_COMPUTE_SHADER: return "COMPUTE Shader"sv;
			case GL::GL_VERTEX_SHADER: return "VERTEX Shader"sv;
			case GL::GL_TESS_CONTROL_SHADER: return "TESS_CONTROL Shader"sv;
			case GL::GL_TESS_EVALUATION_SHADER: return "TESS_EVALUATION Shader"sv;
			case GL::GL_GEOMETRY_SHADER: return "GEOMETRY Shader"sv;
			case GL::GL_FRAGMENT_SHADER: return "FRAGMENT Shader"sv;
			default: return "UNKNOWN"sv;
			}
		}
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

			if (not stage.is_compiled())
			{
				all_stages_compiled = false;
				error_log << IntoString(data_stage.type) << ": " << stage.get_log();
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

		if (not program.is_linked())
		{
			error_log << "Compilation failed! Linking Error:\n" << program.get_log();
			return {error_log.str()};
		}

		program.update_interface_mapping();
		return {move(program)};
	}

	namespace Helpers
	{
		GL::GLenum StringToGLEnum(std::string_view stage)
		{
			if (stage == "vert") return GL::GL_VERTEX_SHADER;
			if (stage == "frag") return GL::GL_FRAGMENT_SHADER;
			if (stage == "geom") return GL::GL_GEOMETRY_SHADER;
		}
	}

	std::pair<Name, Description> Parse(File::JSON::JSONObj o, std::filesystem::path const & root_dir)
	{
		using namespace Helpers;

		auto name = o.FindMember("name")->value.GetString();
		GLSL::Program::Description description;

		for (auto & item : o.FindMember("stages")->value.GetObject())
			description.stages.push_back({
				.stage = StringToGLEnum(item.name.GetString()),
				.path = root_dir / item.value.GetString(),
			});

		for (auto & item: o.FindMember("include_paths")->value.GetArray())
			description.include_paths.push_back(root_dir / item.GetString());

		description.include_strings.push_back(GL::GLSL_VERSION_MACRO);
		for (auto & item: o.FindMember("include_strings")->value.GetArray())
			description.include_strings.emplace_back(item.GetString());

		return {name, description};
	}
}