#pragma once

#include "Lib/geometry/core.hpp"

#include "core.hpp"
#include "utils.hpp"

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

		struct AttributeMapping
		{
			u32 location;
			GLenum glsl_type; // just for debug purposes
			bool per_patch;
			Geometry::Attribute::Key key;
		};
		vector<AttributeMapping> attribute_mappings;

		struct UniformMapping
		{
			u32 location;
			GLenum glsl_type; // just for debug purposes
			std::string key;
		};
		vector<UniformMapping> uniform_mappings;

		struct UniformBlockMapping
		{
			u32 location;
			u32 data_size;
			std::string key;

			struct Variable
			{
				u32 offset;
				GLenum glsl_type; // just for debug purposes
				std::string key;
			};
			vector<Variable> variables;
		};
		vector<UniformBlockMapping> uniform_block_mappings;

		struct StorageBlockMapping
		{
			u32 location;
			u32 data_size;
			std::string key;

			struct Variable
			{
				u32 offset;
				GLenum glsl_type; // just for debug purposes
				std::string key;
			};
			vector<Variable> variables;
		};
		vector<StorageBlockMapping> storage_block_mappings;
	};
}