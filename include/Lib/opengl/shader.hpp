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

	struct Desc
	{
		GLenum stage;
		vector<char const*> const & sources;
	};

	void init(Desc const & desc)
	{
		id = glCreateShader(desc.stage);

		glShaderSource(id, desc.sources.size(), desc.sources.data(), nullptr);
		glCompileShader(id);
	}
};

struct ShaderProgram : OpenGLObject
{
	Name vertex_layout_name;
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

	struct Desc
	{
		vector<ShaderStage const*> const & shader_stages;
	};

	void init(Desc const & desc)
	{
		id = glCreateProgram();

		for (auto & shader_stage: desc.shader_stages)
			glAttachShader(id, shader_stage->id);

		glLinkProgram(id);

		for (auto & shader_stage: desc.shader_stages)
			glDetachShader(id, shader_stage->id);
	}
};
}