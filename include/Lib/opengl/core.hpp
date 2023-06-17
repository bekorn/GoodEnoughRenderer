#pragma once

#include <core/core.hpp>

namespace GL
{
using namespace gl45core;
using namespace gl45ext;

auto constexpr VERSION_MAJOR = 4;
auto constexpr VERSION_MINOR = 5;

// following DSA API https://github.com/fendevel/Guide-to-Modern-OpenGL-Functions

struct OpenGLObject
{
	u32 id = 0;

	CTOR(OpenGLObject, default)
	COPY(OpenGLObject, delete)

	OpenGLObject(OpenGLObject && other) noexcept
	{
		std::swap(id, other.id);
	}
	OpenGLObject & operator=(OpenGLObject && other) noexcept
	{
		std::swap(id, other.id);
		return *this;
	};
};

inline void init()
{
	// configure NDC z to be [0, 1]
	glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);

	// dithering is not used and keeping it enabled produces warnings when using integer framebuffers
	glDisable(GL_DITHER);
}
}