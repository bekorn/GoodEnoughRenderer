#pragma once

#include "core.hpp"
#include "Lib/geometry/core.hpp"

namespace GL
{
	std::string GetContextInfo();

	void DebugCallback(
		GLenum source,
		GLenum type, GLuint id, GLenum severity,
		GLsizei length, const GLchar * message,
		const void * userParam
	);

	std::string_view GLSLTypeToString(GLenum type);

	u32 ComponentTypeSize(GLenum type);

	GLenum IntoGLenum(Geometry::Attribute::Type::Value type);

	Geometry::Attribute::Type IntoAttributeType(GLenum type, bool is_normalized);
}