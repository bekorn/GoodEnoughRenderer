#pragma once

#include "Lib/core/core.hpp"

namespace GL
{
	// TODO(bekorn): move into a .cpp

	inline std::string GetContextInfo()
	{
		fmt::memory_buffer buffer;
		auto buffer_iter = std::back_inserter(buffer);

		fmt::format_to(buffer_iter, "Device: {}\n", (const char *) glGetString(GL_RENDERER));

		i32 major, minor;
		glGetIntegerv(GL_MAJOR_VERSION, &major);
		glGetIntegerv(GL_MINOR_VERSION, &minor);
		fmt::format_to(buffer_iter, "GL version: {}.{}\n", major, minor);

		char dot;
		std::istringstream((char const*) glGetString(GL_SHADING_LANGUAGE_VERSION)) >> major >> dot >> minor;
		fmt::format_to(buffer_iter, "GLSL version: {}.{}", major, minor);

		return fmt::to_string(buffer);
	}

	inline void DebugCallback(
		GLenum source,
		GLenum type, GLuint id, GLenum severity,
		GLsizei length, const GLchar* message,
		const void* userParam
	)
	{// @formatter:off
		// ignore non-significant error/warning codes
		if (id == 131169 || id == 131185 || id == 131218 || id == 131204)
			return;

		std::ostringstream out;

		out << "OpenGL Debug [";
		switch (source)
		{
		case GL_DEBUG_SOURCE_API:				out << "Source: API"; break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:		out << "Source: Window System"; break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER:	out << "Source: Shader Compiler"; break;
		case GL_DEBUG_SOURCE_THIRD_PARTY:		out << "Source: Third Party"; break;
		case GL_DEBUG_SOURCE_APPLICATION:		out << "Source: Application"; break;
		case GL_DEBUG_SOURCE_OTHER:				out << "Source: Other"; break;
		default: break;
		}
		out << ", ";
		switch (type)
		{
		case GL_DEBUG_TYPE_ERROR:				out << "Type: Error"; break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:	out << "Type: Deprecated Behaviour";break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:	out << "Type: Undefined Behaviour";	break;
		case GL_DEBUG_TYPE_PORTABILITY:			out << "Type: Portability";	break;
		case GL_DEBUG_TYPE_PERFORMANCE:			out << "Type: Performance";	break;
		case GL_DEBUG_TYPE_MARKER:				out << "Type: Marker";	break;
		case GL_DEBUG_TYPE_PUSH_GROUP:			out << "Type: Push Group";break;
		case GL_DEBUG_TYPE_POP_GROUP:			out << "Type: Pop Group";break;
		case GL_DEBUG_TYPE_OTHER:				out << "Type: Other";break;
		default: break;
		}
		out << ", ";
		switch (severity)
		{
		case GL_DEBUG_SEVERITY_HIGH: 			out << "Severity: high"; break;
		case GL_DEBUG_SEVERITY_MEDIUM: 			out << "Severity: medium"; break;
		case GL_DEBUG_SEVERITY_LOW: 			out << "Severity: low"; break;
		case GL_DEBUG_SEVERITY_NOTIFICATION:	out << "Severity: notification"; break;
		default: break;
		}
		out << "]\n";

		out << "Error " << id << ": " << message;
		if (message[length - 2] != '\n')
			out << '\n';

		fmt::print("{}", out.str());
	}// @formatter:on
}
