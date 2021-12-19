#pragma once

#include <string>

#include "core.hpp"
#include "glue.hpp"

namespace GL
{
	struct ShaderStage : OpenGLObject
	{
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

		bool is_compiled() const
		{
			i32 success;
			glGetShaderiv(id, GL_COMPILE_STATUS, &success);
			return success;
		}

		std::string get_log() const
		{
			i32 log_size;
			glGetShaderiv(id, gl::GL_INFO_LOG_LENGTH, &log_size);

			std::string log(log_size, '\0');
			glGetShaderInfoLog(id, log_size, nullptr, log.data());

			return log;
		}
	};

	struct ShaderProgram : OpenGLObject
	{
		ShaderProgram() noexcept = default;
		ShaderProgram(ShaderProgram &&) noexcept = default;
		ShaderProgram & operator=(ShaderProgram &&) noexcept = default;

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

		bool is_linked() const
		{
			i32 success;
			glGetProgramiv(id, GL_LINK_STATUS, &success);
			return success;
		}

		std::string get_log() const
		{
			i32 log_size;
			glGetProgramiv(id, gl::GL_INFO_LOG_LENGTH, &log_size);

			std::string log(log_size, '\0');
			glGetProgramInfoLog(id, log_size, nullptr, log.data());

			return log;
		}

		// https://www.khronos.org/opengl/wiki/Program_Introspection#Interface_query

		struct GLSLMapping
		{
			u32 location;
			GLenum glsl_type; // just for debug purposes
			std::string name;
		};
		vector<GLSLMapping> uniform_mappings;
		vector<GLSLMapping> attribute_mappings;

		vector<GLSLMapping> query_interface_mapping(GLenum const interface)
		{
			vector<GLSLMapping> mappings;

			array const query_props{
				GL_LOCATION,
				GL_TYPE,
			};
			array<i32, query_props.size()> query_results;

			i32 max_name_size;
			glGetProgramInterfaceiv(id, interface, GL_MAX_NAME_LENGTH, &max_name_size);
			std::string name_buffer(max_name_size, 0);

			i32 attribute_size;
			glGetProgramInterfaceiv(id, interface, GL_ACTIVE_RESOURCES, &attribute_size);

			mappings.reserve(attribute_size);
			for (auto i = 0; i < attribute_size; ++i)
			{
				glGetProgramResourceiv(
					id, interface, i,
					query_props.size(), query_props.data(),
					query_props.size(), nullptr, query_results.data()
				);

				i32 name_size;
				glGetProgramResourceName(
					id, interface, i,
					name_buffer.size(), &name_size, name_buffer.data()
				);

				mappings.emplace_back(
					GLSLMapping{
						.location = static_cast<u32>(query_results[0]),
						.glsl_type = static_cast<GLenum>(query_results[1]),
						.name = name_buffer.substr(0, name_size),
					}
				);
			}

			return mappings;
		}

		void update_interface_mapping()
		{
			uniform_mappings = query_interface_mapping(GL_UNIFORM);
			attribute_mappings = query_interface_mapping(GL_PROGRAM_INPUT);
		}
	};
}