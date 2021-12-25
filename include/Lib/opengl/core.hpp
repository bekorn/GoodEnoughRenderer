#pragma once

#include "Lib/core/core.hpp"

#include ".pch.hpp"

namespace GL
{
	using namespace gl45core;

	auto constexpr VERSION_MAJOR = 4;
	auto constexpr VERSION_MINOR = 5;

	// following DSA API https://github.com/fendevel/Guide-to-Modern-OpenGL-Functions

	struct OpenGLObject
	{
		u32 id = 0;

		OpenGLObject() noexcept= default;

		OpenGLObject(OpenGLObject const &) noexcept = delete;
		OpenGLObject& operator=(OpenGLObject const &) noexcept = delete;

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