#pragma once

#include "core.hpp"
#include "uniform_block.hpp"
#include "storage_block.hpp"

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

	struct Desc
	{
		GLenum usage = GL_STATIC_DRAW;
		span<byte> data = {};
	};

	void init(Desc const & desc)
	{
		glCreateBuffers(1, &id);
		glNamedBufferData(
			id,
			desc.data.size(),
			desc.data.data(),
			desc.usage
		);
	}

	struct EmptyDesc
	{
		GLenum usage = GL_STATIC_DRAW;
		usize size;
	};

	void init(EmptyDesc const & desc)
	{
		glCreateBuffers(1, &id);
		glNamedBufferData(
			id,
			desc.size,
			nullptr,
			desc.usage
		);
	}

	struct UniformBlockDesc
	{
		GLenum usage = GL_DYNAMIC_DRAW;
		GL::UniformBlock const & uniform_block;
		u32 array_size = 1;
	};

	void init(UniformBlockDesc const & desc)
	{
		glCreateBuffers(1, &id);
		glNamedBufferData(
			id,
			desc.uniform_block.aligned_size * desc.array_size,
			nullptr,
			desc.usage
		);
	}

	struct StorageBlockDesc
	{
		GLenum usage = GL_DYNAMIC_DRAW;
		GL::StorageBlock const & storage_block;
		usize array_size = 1;
	};

	void init(StorageBlockDesc const & desc)
	{
		glCreateBuffers(1, &id);
		glNamedBufferData(
			id,
			desc.storage_block.aligned_size * desc.array_size,
			nullptr,
			desc.usage
		);
	}
};
}
