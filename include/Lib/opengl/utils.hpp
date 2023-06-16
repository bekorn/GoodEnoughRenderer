#pragma once

#include "core.hpp"
#include "Lib/geometry/core.hpp"

namespace GL
{
std::string get_context_info();

void debug_callback(
	GLenum source,
	GLenum type, GLuint id, GLenum severity,
	GLsizei length, const GLchar * message,
	const void * userParam
);

const char * glsl_uniform_type_to_string(GLenum type);

u32 gl_component_type_size(GLenum type);

GLenum to_glenum(Geometry::Type::Value type);


// shortcuts
// (using <original-function>; prevents name hiding)

using gl::glViewport;

inline void glViewport(i32x2 offset, i32x2 size)
{ glViewport(offset.x, offset.y, size.x, size.y); }

using gl::glScissor;

inline void glScissor(i32x2 offset, i32x2 size)
{ glScissor(offset.x, offset.y, size.x, size.y); }
}