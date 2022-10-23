#pragma once

// TODO(bekorn): this supposed to be inside glsl/program/.cpp but glsl/uniform_buffer/.cpp also needs it

#include "Lib/opengl/shader.hpp"

namespace GLSL::Program::Helpers
{
	inline std::string_view IntoString(GL::GLenum shader_stage)
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

	// Pattern: String into Geometry::Attribute::Key
	inline Geometry::Attribute::Key IntoAttributeKey(std::string_view name)
	{
		using namespace Geometry::Attribute;

		static auto const IntoCommon = [](std::string_view attribute_name) -> optional<Key::Common>
		{
			using enum Key::Common;
			if (attribute_name == "position") return POSITION;
			if (attribute_name == "normal") return NORMAL;
			if (attribute_name == "tangent") return TANGENT;
			if (attribute_name == "texcoord") return TEXCOORD;
			if (attribute_name == "color") return COLOR;
			return nullopt;
		};

		static std::regex const attribute_pattern("(_)?(.*?)(_\\d+)?");

		std::cmatch match;
		regex_match(
			name.data(), name.data() + name.size(),
			match, attribute_pattern
		);

		Key key;

		if (match[1].matched) // is custom
		{
			key.name = match[2].str();
		}
		else
		{
			auto common_name = IntoCommon(std::string_view(match[2].first, match[2].second));
			if (common_name.has_value())
				key.name = common_name.value();
			else
				key.name = match[2].str();
		}

		key.layer = 0;
		if (match[3].matched) // has a layer
			std::from_chars(match[3].first + 1, match[3].second, key.layer);

		return key;
	}


	// for GL::ShaderStage

	inline bool is_compiled(GL::ShaderStage const & program)
	{
		GL::GLsizei success;
		glGetShaderiv(program.id, GL::GL_COMPILE_STATUS, &success);
		return success != 0;
	}

	inline std::string get_log(GL::ShaderStage const & program)
	{
		GL::GLsizei log_size;
		glGetShaderiv(program.id, gl::GL_INFO_LOG_LENGTH, &log_size);

		std::string log(log_size, '\0');
		GL::glGetShaderInfoLog(program.id, log_size, nullptr, log.data());

		return log;
	}

	inline std::string get_source(GL::ShaderStage const & program)
	{
		GL::GLsizei source_size;
		glGetShaderiv(program.id, GL::GL_SHADER_SOURCE_LENGTH, &source_size);

		std::string shader_source(source_size, '\0');
		GL::glGetShaderSource(program.id, source_size, &source_size, shader_source.data());

		return shader_source;
	}


	// for GL::ShaderProgram

	inline bool is_linked(GL::ShaderProgram const & program)
	{
		i32 success;
		glGetProgramiv(program.id, GL::GL_LINK_STATUS, &success);
		return success;
	}

	inline std::string get_log(GL::ShaderProgram const & program)
	{
		i32 log_size;
		glGetProgramiv(program.id, gl::GL_INFO_LOG_LENGTH, &log_size);

		std::string log(log_size, '\0');
		GL::glGetProgramInfoLog(program.id, log_size, nullptr, log.data());

		return log;
	}

	// https://www.khronos.org/opengl/wiki/Program_Introspection#Interface_query

	inline void set_attribute_mapping(GL::ShaderProgram & program)
	{
		using namespace GL;

		auto const interface = GL_PROGRAM_INPUT;

		array const query_props{
			GL_LOCATION,
			GL_TYPE,
			GL_IS_PER_PATCH,
		};
		array<i32, query_props.size()> query_results;

		i32 max_name_size;
		glGetProgramInterfaceiv(program.id, interface, GL_MAX_NAME_LENGTH, &max_name_size);
		std::string name_buffer(max_name_size, 0);

		i32 resource_size;
		glGetProgramInterfaceiv(program.id, interface, GL_ACTIVE_RESOURCES, &resource_size);

		program.attribute_mappings.clear();
		program.attribute_mappings.reserve(resource_size);
		for (auto i = 0; i < resource_size; ++i)
		{
			glGetProgramResourceiv(
				program.id, interface, i,
				query_props.size(), query_props.data(),
				query_props.size(), nullptr, query_results.data()
			);

			i32 name_size;
			glGetProgramResourceName(
				program.id, interface, i,
				name_buffer.size(), &name_size, name_buffer.data()
			);

			program.attribute_mappings.emplace_back(
				GL::AttributeMapping{
					.location = static_cast<u32>(query_results[0]),
					.glsl_type = static_cast<GLenum>(query_results[1]),
					.per_patch = static_cast<bool>(query_results[2]),
					.key = IntoAttributeKey(std::string_view(name_buffer.data(), name_size)),
				}
			);
		}
	}

	inline void set_uniform_mapping(GL::ShaderProgram & program)
	{
		using namespace GL;

		auto const interface = GL_UNIFORM;

		array const query_props{
			GL_LOCATION,
			GL_TYPE,
		};
		array<i32, query_props.size()> query_results;

		i32 max_name_size;
		glGetProgramInterfaceiv(program.id, interface, GL_MAX_NAME_LENGTH, &max_name_size);
		std::string name_buffer(max_name_size, 0);

		i32 resource_size;
		glGetProgramInterfaceiv(program.id, interface, GL_ACTIVE_RESOURCES, &resource_size);

		program.uniform_mappings.reserve(resource_size);
		for (auto i = 0; i < resource_size; ++i)
		{
			glGetProgramResourceiv(
				program.id, interface, i,
				query_props.size(), query_props.data(),
				query_props.size(), nullptr, query_results.data()
			);

			// Skip block uniform
			if (query_results[0] == -1)
				continue;

			i32 name_size;
			glGetProgramResourceName(
				program.id, interface, i,
				name_buffer.size(), &name_size, name_buffer.data()
			);

			program.uniform_mappings.emplace_back(
				GL::UniformMapping{
					.location = static_cast<u32>(query_results[0]),
					.glsl_type = static_cast<GLenum>(query_results[1]),
					.key = name_buffer.substr(0, name_size),
				}
			);
		}
	}

	inline vector<GL::UniformBlockMapping::Variable>
	query_uniform_block_variables(GL::ShaderProgram const & program, u32 block_idx, u32 variable_count)
	{
		using namespace GL;

		vector<i32> variable_indices;
		variable_indices.resize(variable_count);

		glGetProgramResourceiv(
			program.id, GL_UNIFORM_BLOCK, block_idx,
			1, &GL_ACTIVE_VARIABLES,
			variable_count, nullptr, variable_indices.data()
		);

		auto const interface = GL_UNIFORM;
		using Mapping = GL::UniformBlockMapping;

		array const query_props{
			GL_OFFSET,
			GL_TYPE,
		};
		array<i32, query_props.size()> query_results;

		i32 max_name_size;
		glGetProgramInterfaceiv(program.id, interface, GL_MAX_NAME_LENGTH, &max_name_size);
		std::string name_buffer(max_name_size, 0);

		vector<Mapping::Variable> variables;
		variables.reserve(variable_count);

		for (auto i: variable_indices)
		{
			glGetProgramResourceiv(
				program.id, interface, i,
				query_props.size(), query_props.data(),
				query_props.size(), nullptr, query_results.data()
			);

			i32 name_size;
			glGetProgramResourceName(
				program.id, interface, i,
				name_buffer.size(), &name_size, name_buffer.data()
			);

			variables.emplace_back(Mapping::Variable{
				.offset = static_cast<u32>(query_results[0]),
				.glsl_type = static_cast<GLenum>(query_results[1]),
				.key = name_buffer.substr(0, name_size),
			});
		}

		std::ranges::sort(variables, std::ranges::less{}, &Mapping::Variable::offset);

		return variables;
	}

	inline void set_uniform_block_mapping(GL::ShaderProgram & program)
	{
		using namespace GL;

		auto const interface = GL_UNIFORM_BLOCK;

		array const query_props{
			GL_BUFFER_BINDING,
			GL_BUFFER_DATA_SIZE,
			GL_NUM_ACTIVE_VARIABLES,
		};
		array<i32, query_props.size()> query_results;

		i32 max_name_size;
		glGetProgramInterfaceiv(program.id, interface, GL_MAX_NAME_LENGTH, &max_name_size);
		std::string name_buffer(max_name_size, 0);

		i32 resource_size;
		glGetProgramInterfaceiv(program.id, interface, GL_ACTIVE_RESOURCES, &resource_size);

		program.uniform_block_mappings.reserve(resource_size);
		for (auto i = 0; i < resource_size; ++i)
		{
			glGetProgramResourceiv(
				program.id, interface, i,
				query_props.size(), query_props.data(),
				query_props.size(), nullptr, query_results.data()
			);

			i32 name_size;
			glGetProgramResourceName(
				program.id, interface, i,
				name_buffer.size(), &name_size, name_buffer.data()
			);

			program.uniform_block_mappings.emplace_back(
				GL::UniformBlockMapping{
					.location = static_cast<u32>(query_results[0]),
					.data_size = static_cast<u32>(query_results[1]),
					.key = name_buffer.substr(0, name_size),
					.variables = query_uniform_block_variables(program, i, static_cast<u32>(query_results[2])),
				}
			);
		}
	}

	inline vector<GL::StorageBlockMapping::Variable>
	query_sotrage_block_variables(GL::ShaderProgram const & program, u32 block_idx, u32 variable_count)
	{
		using namespace GL;

		vector<i32> variable_indices;
		variable_indices.resize(variable_count);

		glGetProgramResourceiv(
			program.id, GL_SHADER_STORAGE_BLOCK, block_idx,
			1, &GL_ACTIVE_VARIABLES,
			variable_count, nullptr, variable_indices.data()
		);

		auto const interface = GL_BUFFER_VARIABLE;
		using Mapping = GL::StorageBlockMapping;

		array const query_props{
			GL_OFFSET,
			GL_TYPE,
		};
		array<i32, query_props.size()> query_results;

		i32 max_name_size;
		glGetProgramInterfaceiv(program.id, interface, GL_MAX_NAME_LENGTH, &max_name_size);
		std::string name_buffer(max_name_size, 0);

		vector<Mapping::Variable> variables;
		variables.reserve(variable_count);

		for (auto i: variable_indices)
		{
			glGetProgramResourceiv(
				program.id, interface, i,
				query_props.size(), query_props.data(),
				query_props.size(), nullptr, query_results.data()
			);

			i32 name_size;
			glGetProgramResourceName(
				program.id, interface, i,
				name_buffer.size(), &name_size, name_buffer.data()
			);

			variables.emplace_back(Mapping::Variable{
				.offset = static_cast<u32>(query_results[0]),
				.glsl_type = static_cast<GLenum>(query_results[1]),
				.key = name_buffer.substr(0, name_size),
			});
		}

		std::ranges::sort(variables, std::ranges::less{}, &Mapping::Variable::offset);

		return variables;
	}

	inline void set_storage_block_mapping(GL::ShaderProgram & program)
	{
		using namespace GL;

		auto const interface = GL_SHADER_STORAGE_BLOCK;

		array const query_props{
			GL_BUFFER_BINDING,
			GL_BUFFER_DATA_SIZE,
			GL_NUM_ACTIVE_VARIABLES,
		};
		array<i32, query_props.size()> query_results;

		i32 max_name_size;
		glGetProgramInterfaceiv(program.id, interface, GL_MAX_NAME_LENGTH, &max_name_size);
		std::string name_buffer(max_name_size, 0);

		i32 resource_size;
		glGetProgramInterfaceiv(program.id, interface, GL_ACTIVE_RESOURCES, &resource_size);

		program.storage_block_mappings.reserve(resource_size);
		for (auto i = 0; i < resource_size; ++i)
		{
			glGetProgramResourceiv(
				program.id, interface, i,
				query_props.size(), query_props.data(),
				query_props.size(), nullptr, query_results.data()
			);

			i32 name_size;
			glGetProgramResourceName(
				program.id, interface, i,
				name_buffer.size(), &name_size, name_buffer.data()
			);

			program.storage_block_mappings.emplace_back(
				GL::StorageBlockMapping{
					.location = static_cast<u32>(query_results[0]),
					.data_size = static_cast<u32>(query_results[1]),
					.key = name_buffer.substr(0, name_size),
					.variables = query_sotrage_block_variables(program, i, static_cast<u32>(query_results[2])),
				}
			);
		}
	}
}