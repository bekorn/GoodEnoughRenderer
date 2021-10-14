#pragma once

#include ".pch.hpp"
#include "core_types.hpp"

#include <string>

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
			std::vector<char const*> const & sources;
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

	std::string GLTypeToString(GLenum type)
	{
		if (type == GL_FLOAT) return "FLOAT";
		if (type == GL_FLOAT_VEC2) return "FLOAT_VEC2";
		if (type == GL_FLOAT_VEC3) return "FLOAT_VEC3";
		if (type == GL_FLOAT_VEC4) return "FLOAT_VEC4";
		if (type == GL_FLOAT_MAT2) return "FLOAT_MAT2";
		if (type == GL_FLOAT_MAT3) return "FLOAT_MAT3";
		if (type == GL_FLOAT_MAT4) return "FLOAT_MAT4";
		if (type == GL_FLOAT_MAT2x3) return "FLOAT_MAT2x3";
		if (type == GL_FLOAT_MAT2x4) return "FLOAT_MAT2x4";
		if (type == GL_FLOAT_MAT3x2) return "FLOAT_MAT3x2";
		if (type == GL_FLOAT_MAT3x4) return "FLOAT_MAT3x4";
		if (type == GL_FLOAT_MAT4x2) return "FLOAT_MAT4x2";
		if (type == GL_FLOAT_MAT4x3) return "FLOAT_MAT4x3";
		if (type == GL_INT) return "INT";
		if (type == GL_INT_VEC2) return "INT_VEC2";
		if (type == GL_INT_VEC3) return "INT_VEC3";
		if (type == GL_INT_VEC4) return "INT_VEC4";
		if (type == GL_UNSIGNED_INT) return "UNSIGNED_INT";
		if (type == GL_UNSIGNED_INT_VEC2) return "UNSIGNED_INT_VEC2";
		if (type == GL_UNSIGNED_INT_VEC3) return "UNSIGNED_INT_VEC3";
		if (type == GL_UNSIGNED_INT_VEC4) return "UNSIGNED_INT_VEC4";
		if (type == GL_DOUBLE) return "DOUBLE";
		if (type == GL_DOUBLE_VEC2) return "DOUBLE_VEC2";
		if (type == GL_DOUBLE_VEC3) return "DOUBLE_VEC3";
		if (type == GL_DOUBLE_VEC4) return "DOUBLE_VEC4";
		if (type == GL_DOUBLE_MAT2) return "DOUBLE_MAT2";
		if (type == GL_DOUBLE_MAT3) return "DOUBLE_MAT3";
		if (type == GL_DOUBLE_MAT4) return "DOUBLE_MAT4";
		if (type == GL_DOUBLE_MAT2x3) return "DOUBLE_MAT2x3";
		if (type == GL_DOUBLE_MAT2x4) return "DOUBLE_MAT2x4";
		if (type == GL_DOUBLE_MAT3x2) return "DOUBLE_MAT3x2";
		if (type == GL_DOUBLE_MAT3x4) return "DOUBLE_MAT3x4";
		if (type == GL_DOUBLE_MAT4x2) return "DOUBLE_MAT4x2";
		/*if (type == GL_DOUBLE_MAT4x3)*/ return "DOUBLE_MAT4x3";
	}

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
			std::vector<ShaderStage const*> const & shader_stages;
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


		std::string get_active_uniforms_OLD() const
		{
			std::string result;

			i32 max_info_size;
			glGetProgramiv(id, GL_ACTIVE_UNIFORM_MAX_LENGTH, &max_info_size);
			std::string info(max_info_size, 0);

			i32 uniform_size;
			glGetProgramiv(id, GL_ACTIVE_UNIFORMS, &uniform_size);
			result += "Uniform size: " + std::to_string(uniform_size) + '\n';

			for (ifast i = 0; i < uniform_size; ++i)
			{
				GLenum uniform_type;
				i32 uniform_array_size;
				i32 info_size;
				glGetActiveUniform(
					id, i,
					max_info_size, &info_size,
					&uniform_array_size, &uniform_type, info.data()
				);

				result.append(info, 0, info_size);
				result += "[" + std::to_string(uniform_array_size) + "] ";
				result += GLTypeToString(uniform_type) + "\n";
			}

			return result;
		}

		std::string get_active_attributes_OLD() const
		{
			std::string result;

			i32 max_info_size;
			glGetProgramiv(id, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &max_info_size);
			std::string info(max_info_size, '\0');

			i32 attribute_size;
			glGetProgramiv(id, GL_ACTIVE_ATTRIBUTES, &attribute_size);
			result += "Attribute size: " + std::to_string(attribute_size) + '\n';

			for (ifast i = 0; i < attribute_size; ++i)
			{
				GLenum attribute_type;
				i32 attribute_array_size;
				i32 info_size;
				glGetActiveAttrib(
					id, i,
					max_info_size, &info_size,
					&attribute_array_size, &attribute_type, info.data()
				);

				result.append(info, 0, info_size);
				result += "[" + std::to_string(attribute_array_size) + "] ";
				result += GLTypeToString(attribute_type) + "\n";
			}

			return result;
		}

		// The new API
		// https://www.khronos.org/opengl/wiki/Program_Introspection#Interface_query

		std::string get_active_uniforms() const
		{
			std::string result;

			auto const interface = GL_UNIFORM;
			auto const prop_size = 3;
			std::array<GLenum, prop_size> const query_props{
				GL_TYPE,
				GL_ARRAY_SIZE,
//				GL_NAME_LENGTH,
				GL_LOCATION,
			};

			std::array<i32, prop_size> query_results;

			i32 max_name_size;
			glGetProgramInterfaceiv(id, interface, GL_MAX_NAME_LENGTH, &max_name_size);
			std::string name(max_name_size, 0);

			i32 uniform_size;
			glGetProgramInterfaceiv(id, interface, GL_ACTIVE_RESOURCES, &uniform_size);
			result += "Uniform size: " + std::to_string(uniform_size) + '\n';

			for (auto i = 0; i < uniform_size; ++i)
			{
				glGetProgramResourceiv(
					id, interface, i,
					prop_size, query_props.data(),
					prop_size, nullptr, query_results.data()
				);

				i32 name_size;
				glGetProgramResourceName(
					id, interface, i,
					name.size(), &name_size, name.data()
				);

				result += GLTypeToString(GLenum(query_results[0]));
				result += "[" + std::to_string(query_results[1]) + "] ";
				result.append(name, 0, name_size);
				result += " (location = " + std::to_string(query_results[2]) + ")\n";
			}

			return result;
		}

		std::string get_active_attributes() const
		{
			std::string result;

			auto const interface = GL_PROGRAM_INPUT;
			auto const prop_size = 3;
			std::array<GLenum, prop_size> const query_props{
				GL_TYPE,
				GL_ARRAY_SIZE,
				//				GL_NAME_LENGTH,
				GL_LOCATION,
			};

			std::array<i32, prop_size> query_results;

			i32 max_name_size;
			glGetProgramInterfaceiv(id, interface, GL_MAX_NAME_LENGTH, &max_name_size);
			std::string name(max_name_size, 0);

			i32 uniform_size;
			glGetProgramInterfaceiv(id, interface, GL_ACTIVE_RESOURCES, &uniform_size);
			result += "Attribute size: " + std::to_string(uniform_size) + '\n';

			for (auto i = 0; i < uniform_size; ++i)
			{
				glGetProgramResourceiv(
					id, interface, i,
					prop_size, query_props.data(),
					prop_size, nullptr, query_results.data()
				);

				i32 name_size;
				glGetProgramResourceName(
					id, interface, i,
					name.size(), &name_size, name.data()
				);

				result += GLTypeToString(GLenum(query_results[0]));
				result += "[" + std::to_string(query_results[1]) + "] ";
				result.append(name, 0, name_size);
				result += " (location = " + std::to_string(query_results[2]) + ")\n";
			}

			return result;
		}
	};
}