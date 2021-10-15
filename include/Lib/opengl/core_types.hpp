#pragma once

#include ".pch.hpp"

// TODO(bekorn): maybe rename to ogl or OGL
namespace GL
{
	// TODO(bekorn): is this the place for config?
	auto const VERSION_MAJOR = 4;
	auto const VERSION_MINOR = 3;

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