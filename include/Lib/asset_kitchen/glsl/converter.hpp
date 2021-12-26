#pragma once

#include "Lib/core/expected.hpp"
#include "Lib/opengl/shader.hpp"

#include "core.hpp"

namespace GLSL
{
	namespace Helpers
	{
		std::string_view IntoString(GL::GLenum shader_stage)
		{
			// https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glCreateShader.xhtml
			switch (shader_stage)
			{
			case GL::GL_COMPUTE_SHADER: return "COMPUTE Shader";
			case GL::GL_VERTEX_SHADER: return "VERTEX Shader";
			case GL::GL_TESS_CONTROL_SHADER: return "TESS_CONTROL Shader";
			case GL::GL_TESS_EVALUATION_SHADER: return "TESS_EVALUATION Shader";
			case GL::GL_GEOMETRY_SHADER: return "GEOMETRY Shader";
			case GL::GL_FRAGMENT_SHADER: return "FRAGMENT Shader";
			default: return "UNKNOWN";
			}
		}
	}

	Expected<GL::ShaderProgram, std::string> Convert(LoadedData const & loaded)
	{
		using namespace Helpers;

		std::stringstream error_log;

		vector<const char*> includes;
		includes.resize(loaded.includes.size() + 1); // +1 for the stage source
		for (auto i = 0; i < loaded.includes.size(); ++i)
			includes[i] = loaded.includes[i].data();

		bool all_stages_compiled = true;

		vector<GL::ShaderStage> stages;
		stages.resize(loaded.stages.size());
		for (auto i = 0; i < stages.size(); ++i)
		{
			auto & stage = stages[i];
			auto const & data_stage = loaded.stages[i];

			includes.back() = data_stage.source.data(); // put shader source to the last one

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

		return {move(program)};
	}
}