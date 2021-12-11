#pragma once

#include "Lib/core/core_types.hpp"

namespace GL
{
	// TODO(bekorn): is this the place for config?
	auto constexpr VERSION_MAJOR = 4;
	auto constexpr VERSION_MINOR = 5;

	// following DSA API https://github.com/fendevel/Guide-to-Modern-OpenGL-Functions

	struct OpenGLObject
	{
		u32 id = 0;

		OpenGLObject() noexcept= default;

		OpenGLObject(OpenGLObject const &) noexcept = delete;
		OpenGLObject& operator=(OpenGLObject const &) noexcept = delete;

		OpenGLObject(OpenGLObject && other)  noexcept
		{
			id = other.id;
			other.id = 0;
		}
		OpenGLObject& operator=(OpenGLObject && other) noexcept
		{
			id = other.id;
			other.id = 0;
			return *this;
		};
	};
}