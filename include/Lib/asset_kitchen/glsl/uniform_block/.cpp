#pragma message("-- read ASSET/BLOCK/.Cpp --")

#include "load.hpp"
#include "convert.hpp"
#include "../_helpers.hpp"

#include <opengl/glsl.hpp>

namespace GLSL::UniformBlock
{
LoadedData Load(Desc const & desc)
{
	return LoadedData{
		.source = File::LoadAsString(desc.path),
	};
}

Expected<GL::UniformBlock, std::string> Convert(LoadedData const & loaded)
{
	using namespace GLSL::Program::Helpers;

	GL::ShaderStage vertex_stage;
	vertex_stage.init({
		.stage = GL::GL_VERTEX_SHADER,
		.sources = {
			GL::GLSL_LANGUAGE_CONFIG.data(),
			"#line 1\n",
			loaded.source.data(),
			"void main(){}"
		},
	});
	if (not is_compiled(vertex_stage))
		return {get_log(vertex_stage)};

	GL::ShaderProgram program;
	program.init({
		.shader_stages = {&vertex_stage},
	});
	if (not is_linked(program))
		return {get_log(program)};

	set_uniform_block_mapping(program);
	if (program.uniform_block_mappings.empty())
		return {"ShaderProgram has no uniform_blocks"};

	GL::UniformBlock uniform_block;
	uniform_block.init({
		.layout = program.uniform_block_mappings.front(),
	});
	return {move(uniform_block)};
}

std::pair<Name, Desc> Parse(File::JSON::JSONObj o, std::filesystem::path const & root_dir)
{
	return {
		o.FindMember("name")->value.GetString(),
		{.path = root_dir / o.FindMember("path")->value.GetString()}
	};
}
}