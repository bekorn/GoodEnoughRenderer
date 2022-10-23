#pragma once

#include "Lib/geometry/core.hpp"

#include "core.hpp"
#include "utils.hpp"
#include "shader_mappings.hpp"

namespace GL
{
	struct ShaderStage : OpenGLObject
	{
		CTOR(ShaderStage, default)
		COPY(ShaderStage, delete)
		MOVE(ShaderStage, default)

		~ShaderStage()
		{
			glDeleteShader(id);
		}

		struct Description
		{
			GLenum stage;
			vector<char const*> const & sources;
		};

		void create(Description const & description)
		{
			id = glCreateShader(description.stage);

			glShaderSource(id, description.sources.size(), description.sources.data(), nullptr);
			glCompileShader(id);
		}
	};

	struct ShaderProgram : OpenGLObject
	{
		vector<AttributeMapping> attribute_mappings;
		vector<UniformMapping> uniform_mappings;
		vector<UniformBlockMapping> uniform_block_mappings;
		vector<StorageBlockMapping> storage_block_mappings;

		CTOR(ShaderProgram, default)
		COPY(ShaderProgram, delete)
		MOVE(ShaderProgram, default)

		~ShaderProgram()
		{
			glDeleteProgram(id);
		}

		struct Description
		{
			vector<ShaderStage const*> const & shader_stages;
		};

		void create(Description const & description)
		{
			id = glCreateProgram();

			for (auto & shader_stage: description.shader_stages)
				glAttachShader(id, shader_stage->id);

			glLinkProgram(id);

			for (auto & shader_stage: description.shader_stages)
				glDetachShader(id, shader_stage->id);
		}
	};
}