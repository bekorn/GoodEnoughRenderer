#pragma once

#include ".pch.hpp"
#include "core_types.hpp"

namespace GL
{
	struct Buffer : OpenGLObject
	{
		Buffer() noexcept = default;
		Buffer(Buffer&&) noexcept = default;

		~Buffer()
		{
			glDeleteBuffers(1, &id);
		}
		struct Description
		{
			GLenum usage = GL_STATIC_DRAW;
			span<byte> const & data;
		};

		void create(Description const & description)
		{
			glCreateBuffers(1, &id);
			glNamedBufferData(
				id,
				description.data.size(),
				description.data.data(),
				description.usage
			);
		}
	};

}
