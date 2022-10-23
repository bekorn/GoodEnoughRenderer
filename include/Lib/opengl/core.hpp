#pragma once

#include "Lib/core/core.hpp"

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
		OpenGLObject& operator=(OpenGLObject && other) noexcept
		{
			std::swap(id, other.id);
			return *this;
		};
	};
}