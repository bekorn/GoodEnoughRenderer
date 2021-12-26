#pragma once

#include <string>
#include <regex>
#include <charconv>

#include "Lib/geometry/core.hpp"

#include "core.hpp"
#include "glue.hpp"

namespace GL
{
	// Pattern: String into Geometry::Attribute::Key
	Geometry::Attribute::Key IntoAttributeKey(std::string_view name)
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

		bool is_compiled() const
		{
			GLsizei success;
			glGetShaderiv(id, GL_COMPILE_STATUS, &success);
			return success != 0;
		}

		std::string get_log() const
		{
			GLsizei log_size;
			glGetShaderiv(id, gl::GL_INFO_LOG_LENGTH, &log_size);

			std::string log(log_size, '\0');
			glGetShaderInfoLog(id, log_size, nullptr, log.data());

			return log;
		}

		std::string get_source() const
		{
			GLsizei source_size;
			glGetShaderiv(id, GL_SHADER_SOURCE_LENGTH, &source_size);

			std::string shader_source(source_size, '\0');
			glGetShaderSource(id, source_size, &source_size, shader_source.data());

			return shader_source;
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

		struct UniformMapping
		{
			u32 location;
			GLenum glsl_type; // just for debug purposes
			std::string key;
		};
		vector<UniformMapping> uniform_mappings;

		struct AttributeMapping
		{
			u32 location;
			GLenum glsl_type; // just for debug purposes
			Geometry::Attribute::Key key;
		};
		vector<AttributeMapping> attribute_mappings;

		void update_interface_mapping()
		{
			// returns UniformMapping as a common denominator
			static auto const query_interface_mapping = [this](GLenum const interface) -> vector<UniformMapping>
			{
				vector<UniformMapping> mappings;

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
						UniformMapping{
							.location = static_cast<u32>(query_results[0]),
							.glsl_type = static_cast<GLenum>(query_results[1]),
							.key = name_buffer.substr(0, name_size),
						}
					);
				}

				return mappings;
			};

			uniform_mappings.clear();
			for (auto && mapping : query_interface_mapping(GL_UNIFORM))
				uniform_mappings.emplace_back(move(mapping));

			attribute_mappings.clear();
			for (auto && mapping : query_interface_mapping(GL_PROGRAM_INPUT))
				attribute_mappings.emplace_back(AttributeMapping{
					.location = mapping.location,
					.glsl_type = mapping.glsl_type,
					.key = IntoAttributeKey(mapping.key),
				});
		}
	};
}