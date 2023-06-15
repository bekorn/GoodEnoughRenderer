#pragma message("-- read ASSET/PROGRAM/.Cpp --")

#include "load.hpp"
#include "convert.hpp"
#include "../_helpers.hpp"

namespace GLSL::Program
{
LoadedData Load(Desc const & desc)
{
	LoadedData loaded;

	loaded.layout_name = desc.layout_name;

	loaded.stages.reserve(desc.stages.size());
	for (auto const & [stage, path]: desc.stages)
		loaded.stages.emplace_back(LoadedData::Stage{
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

namespace Helpers
{
const char * to_string(GL::GLenum stage)
{
	switch (stage)
	{
	case (GL::GL_VERTEX_SHADER): return "VERT";
	case (GL::GL_FRAGMENT_SHADER): return "FRAG";
	case (GL::GL_GEOMETRY_SHADER): return "GEOM";
	case (GL::GL_COMPUTE_SHADER): return "COMP";
	default: assert_case_not_handled();
	}
}

const char * to_string(Geometry::Attribute::Key const & key)
{
	auto const & name = key.name;
	if (holds_alternative<std::string>(name))
		return get<std::string>(name).data();

	using enum Geometry::Attribute::Key::Common;
	switch (get<Geometry::Attribute::Key::Common>(name))
	{
		case POSITION: return "position";
		case NORMAL: return "normal";
		case TANGENT: return "tangent";
		case TEXCOORD: return "texcoord";
		case COLOR: return "color";
		case JOINTS: return "joints";
		case WEIGHTS: return "weights";
	}
	assert_enum_out_of_range();
}

std::string generate_vertex_layout(Geometry::Layout const & layout)
{
	std::string buffer;
	fmt::format_to(back_inserter(buffer), "{}", "#ifdef ONLY_VERT\n");
	for (auto & [k, l] : layout)
		fmt::format_to(back_inserter(buffer), "layout(location = {}) in vec{} {};\n", l.location, l.type.dimension, to_string(k));
	fmt::format_to(back_inserter(buffer), "{}", "#endif\n\0");
	return buffer;
}
}

Expected<GL::ShaderProgram, std::string> Convert(LoadedData const & loaded, Managed<Geometry::Layout> const & vertex_layouts)
{
	using namespace Helpers;

	vector<const char*> sources; // = { language_config, stage_define, loaded.includes, vertex_layout, "#line 1", stage_source }
	sources.resize(1 + 1 + loaded.includes.size() + 1 + 1 + 1);

	sources[0] = GL::GLSL_LANGUAGE_CONFIG.data();

	for (auto i = 0; i < loaded.includes.size(); ++i)
		sources[i + 1 + 1] = loaded.includes[i].data();

	sources[sources.size() - 2] = "#line 1\n";

	std::string vertex_layout;
	if (not loaded.layout_name.string.empty())
		vertex_layout = generate_vertex_layout(vertex_layouts.get(loaded.layout_name));
	sources[sources.size() - 3] = vertex_layout.data();

	/// Compile stages
	vector<GL::ShaderStage> stages;
	stages.resize(loaded.stages.size());
	std::string error_log;
	for (auto i = 0; i < stages.size(); ++i)
	{
		auto & stage = stages[i];
		auto const & loaded_stage = loaded.stages[i];

		auto stage_define = fmt::format("#define ONLY_{}\n", to_string(loaded_stage.type));
		sources[1] = stage_define.data();

		sources[sources.size() - 1] = loaded_stage.source.data();

		stage.init({
			.stage = loaded_stage.type,
			.sources = sources,
		});
		if (not is_compiled(stage))
			fmt::format_to(back_inserter(error_log), "{}: {}", IntoString(loaded_stage.type), get_log(stage));
	}
	if (not error_log.empty())
		return {move(error_log)};


	/// Compile program
	vector<GL::ShaderStage const*> stages_descs;
	stages_descs.reserve(stages.size());
	for(auto & s: stages)
		stages_descs.emplace_back(&s);

	GL::ShaderProgram program;
	program.init({
		.shader_stages = stages_descs,
	});
	if (not is_linked(program))
		return {fmt::format("Compilation failed! Linking Error:\n{}", get_log(program))};

	program.vertex_layout_name = loaded.layout_name;
	set_uniform_mapping(program);
	set_uniform_block_mapping(program);
	set_storage_block_mapping(program);
	return {move(program)};
}

namespace Helpers
{
GL::GLenum to_glenum(std::string_view stage)
{
	if (stage == "vert") return GL::GL_VERTEX_SHADER;
	if (stage == "frag") return GL::GL_FRAGMENT_SHADER;
	if (stage == "geom") return GL::GL_GEOMETRY_SHADER;
	if (stage == "comp") return GL::GL_COMPUTE_SHADER;
	assert_case_not_handled();
}
}

std::pair<Name, Desc> Parse(File::JSON::JSONObj o, std::filesystem::path const & root_dir)
{
	using namespace Helpers;

	auto name = o.FindMember("name")->value.GetString();
	GLSL::Program::Desc desc;

	if (auto member = o.FindMember("layout"); member != o.MemberEnd())
		desc.layout_name = member->value.GetString();

	for (auto & item : o.FindMember("stages")->value.GetArray())
	{
		const char * const path = item.GetString();

		// assert path == <path>/<name>.<1char-separated-stages>.glsl
		auto stages = std::string_view(path);
		stages.remove_suffix(stages.size() - stages.rfind('.'));
		stages = stages.substr(stages.rfind('.') + 1);

		do
		{
			auto stage = stages.substr(0, 4); // all stages are char[4]
			stages.remove_prefix(glm::min(usize(5), stages.size()));

			desc.stages.push_back({
				.stage = to_glenum(stage),
				.path = root_dir / path,
			});
		}
		while (not stages.empty());
	}

	if (auto member = o.FindMember("include_paths"); member != o.MemberEnd())
		for (auto & item: member->value.GetArray())
			desc.include_paths.push_back(root_dir / item.GetString());

	if (auto member = o.FindMember("include_strings"); member != o.MemberEnd())
		for (auto & item: member->value.GetArray())
			desc.include_strings.emplace_back(item.GetString());

	return {name, desc};
}
}