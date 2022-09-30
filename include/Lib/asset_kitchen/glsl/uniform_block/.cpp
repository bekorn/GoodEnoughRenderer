#pragma message("----Read ASSET/BLOCK/.Cpp----")

#include ".hpp"

#include "Lib/opengl/glsl.hpp"
#include "Lib/opengl/shader.hpp"

namespace GLSL::UniformBlock
{
	LoadedData Load(Description const & description)
	{
		return LoadedData{
			.source = File::LoadAsString(description.path),
		};
	}

	Expected<GL::UniformBlock, std::string> Convert(LoadedData const & loaded)
	{
		GL::ShaderStage vertex_stage;
		vertex_stage.create(
			{
				.stage = GL::GL_VERTEX_SHADER,
				.sources = {
					GL::GLSL_VERSION_MACRO.data(),
					"#line 1\n",
					loaded.source.data(),
					"void main(){}"
				},
			}
		);
		if (not vertex_stage.is_compiled())
			return {vertex_stage.get_log()};

		GL::ShaderProgram program;
		program.create(
			{
				.shader_stages = {&vertex_stage},
			}
		);
		if (not program.is_linked())
			return {program.get_log()};

		program.update_uniform_block_mapping();
		if (program.uniform_block_mappings.empty())
			return {"ShaderProgram has no uniform_blocks"};

		GL::UniformBlock uniform_block;
		uniform_block.create(
			{
				.layout = program.uniform_block_mappings.front(),
			}
		);
		return {move(uniform_block)};
	}

	std::pair<Name, Description> Parse(File::JSON::JSONObj o, std::filesystem::path const & root_dir)
	{
		return {
			o.FindMember("name")->value.GetString(),
			{.path = root_dir / o.FindMember("path")->value.GetString()}
		};
	}
}