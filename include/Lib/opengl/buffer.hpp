#pragma once

#include ".pch.hpp"
#include "core_types.hpp"

namespace GL
{
	struct Buffer : OpenGLObject
	{
		GLenum type;

		Buffer() noexcept = default;

		~Buffer()
		{
			glDeleteBuffers(1, &id);
		}

		struct Description
		{
			GLenum type = GL_ARRAY_BUFFER;
			GLenum usage = GL_STATIC_DRAW;
			std::span<byte> const & data;
		};

		void create(Description const & description)
		{
			glGenBuffers(1, &id);

			glBindBuffer(description.type, id);
			glBufferData(
				description.type,
				description.data.size(),
				description.data.data(),
				description.usage
			);

			type = description.type;
		}
	};

}
