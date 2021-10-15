#pragma once

#include ".pch.hpp"

#include <sstream>

namespace GL
{
	std::string GetContextInfo()
	{
		std::stringstream out;

		out << "Renderer: " << glGetString(GL_RENDERER) << '\n';

		i32 major, minor;
		glGetIntegerv(GL_MAJOR_VERSION, &major);
		glGetIntegerv(GL_MINOR_VERSION, &minor);
		out << "GL version: " << major << '.' << minor << '\n';

		char dot;
		std::istringstream((char const*) glGetString(GL_SHADING_LANGUAGE_VERSION)) >> major >> dot >> minor;
		out << "GLSL version: " << major << '.' << minor << '\n';

		return out.str();
	}
}
