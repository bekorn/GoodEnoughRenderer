#pragma once

#include "Lib/core/utils.hpp"

#include "core.hpp"

namespace GL
{
	struct Buffer : OpenGLObject
	{
		CTOR(Buffer, default);
		COPY(Buffer, delete);
		MOVE(Buffer, default);

		~Buffer()
		{
			glDeleteBuffers(1, &id);
		}

		struct Description
		{
			GLenum usage = GL_STATIC_DRAW;
			span<byte> data = {};
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
