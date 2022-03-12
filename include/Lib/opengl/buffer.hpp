#pragma once

#include "Lib/core/utils.hpp"

#include "core.hpp"
#include "uniform_block.hpp"

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

		struct UniformBlockDescription
		{
			GLenum usage = GL_DYNAMIC_DRAW;
			GL::UniformBlock const & uniform_block;
			u32 array_size = 1;
		};

		void create(UniformBlockDescription const & description)
		{
			i32 alignment;
			GL::glGetIntegerv(GL::GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &alignment);

			auto aligned_size = (description.uniform_block.data_size / alignment + 1) * alignment;

			glCreateBuffers(1, &id);
			glNamedBufferData(
				id,
				aligned_size * description.array_size,
				nullptr,
				description.usage
			);
		}
	};
}
