//#pragma once
//
//
//
//struct ShaderProgram : OpenGLObject
//{
//	ShaderProgram() noexcept = default;
//
//	~ShaderProgram()
//	{
//		glDeleteProgram(id);
//	}
//
//	struct Description
//	{
//		std::string const & name;
//	};
//
//	bool create(Description const & description)
//	{
//		static auto const compile_shader = [](
//			std::string const & name, ShaderHandle::Stage stage, GLuint & shader
//		) -> bool
//		{
//			ShaderHandle handle(name, stage);
//			should(handle.is_file())
//
//			std::string source;
//			must(handle.load(source))
//
//			shader = glCreateShader(shader_stage_to_enum(handle.stage));
//			const char* const s = source.c_str();
//			glShaderSource(shader, 1, &s, nullptr);
//			glCompileShader(shader);
//
//			i32 success;
//			glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
//			should(success) // TODO(bekorn): must or should
//
//			return true;
//		};
//
//		std::vector<GLuint> shaders;
//		shaders.reserve(ShaderHandle::STAGE_COUNT);
//		for (ufast i = 0; i < ShaderHandle::STAGE_COUNT; ++i)
//			if (GLuint shader; compile_shader(description.name, (ShaderHandle::Stage) i, shader))
//				shaders.push_back(shader);
//
//		should(not shaders.empty())
//
//		id = glCreateProgram();
//
//		for (auto & shader : shaders)
//			glAttachShader(id, shader);
//		glLinkProgram(id);
//
//		for (auto & shader: shaders)
//			glDetachShader(id, shader), glDeleteShader(shader);
//
//		int success;
//		glGetProgramiv(id, GL_LINK_STATUS, &success);
//		should(success)
//
//		return true;
//	}
//
//	/*
//	void update_handles(const std::string & name)
//	{
//
//	}
//
//	bool compile()
//	{
//		glDeleteProgram(id);
//		id = glCreateProgram();
//
//		static auto const compile_shader = [](const ShaderHandle & handle, GLuint & shader) -> bool
//		{
//			should(handle.is_file())
//
//			std::string source;
//			must(handle.load(source))
//
//			shader = glCreateShader(stage_to_enum(handle.stage));
//			const char* const s = source.c_str();
//			glShaderSource(shader, 1, &s, nullptr);
//			glCompileShader(shader);
//
//			i32 success;
//			glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
//			should(success) // TODO(bekorn): must or should
//
//			return true;
//		};
//
//		std::vector<GLuint> shaders;
//		shaders.reserve(ShaderHandle::STAGE_COUNT);
//		for (auto & handle : handles)
//			if (GLuint shader; compile_shader(handle, shader))
//				shaders.push_back(shader);
//
//		should(not shaders.empty())
//
//		for (auto & shader : shaders)
//			glAttachShader(id, shader);
//		glLinkProgram(id);
//
//		for (auto & shader: shaders)
//			glDetachShader(id, shader), glDeleteShader(shader);
//
//		int success;
//		glGetProgramiv(id, GL_LINK_STATUS, &success);
//		should(success) // TODO(bekorn): must or should
//
//		return true;
//	}
//	 */
//};