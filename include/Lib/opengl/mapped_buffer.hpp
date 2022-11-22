#pragma once

#include "core.hpp"
#include "uniform_block.hpp"
#include "storage_block.hpp"

namespace GL
{
	struct MappedBuffer : OpenGLObject
	{
		byte * map;

		CTOR(MappedBuffer, default);
		COPY(MappedBuffer, delete);
		MOVE(MappedBuffer, default);

		~MappedBuffer()
		{
			glDeleteBuffers(1, &id);
		}

//		struct Description
//		{
//			GLenum usage = GL_STATIC_DRAW;
//			span<byte> data = {};
//		};
//
//		void create(Description const & description)
//		{
//			glCreateBuffers(1, &id);
//			glNamedBufferData(
//				id,
//				description.data.size(),
//				description.data.data(),
//				description.usage
//			);
//		}
//
//		struct EmptyDescription
//		{
//			GLenum usage = GL_STATIC_DRAW;
//			usize size;
//		};
//
//		void create(EmptyDescription const & description)
//		{
//			glCreateBuffers(1, &id);
//			glNamedBufferData(
//				id,
//				description.size,
//				nullptr,
//				description.usage
//			);
//		}

		struct UniformBlockDescription
		{
			BufferStorageMask usage = GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT;
			BufferAccessMask access = GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT;
			UniformBlock const & uniform_block;
			u32 array_size = 1;
		};

		void create(UniformBlockDescription const & description)
		{
			glCreateBuffers(1, &id);
			glNamedBufferStorage(
				id,
				description.uniform_block.aligned_size * description.array_size,
				nullptr,
				description.usage
			);

			map = (byte *) glMapNamedBufferRange(
				id,
				0, description.uniform_block.aligned_size * description.array_size,
				description.access
			);
		}

//		struct StorageBlockDescription
//		{
//			GLenum usage = GL_DYNAMIC_DRAW;
//			GL::StorageBlock const & storage_block;
//			usize array_size = 1;
//		};
//
//		void create(StorageBlockDescription const & description)
//		{
//			glCreateBuffers(1, &id);
//			glNamedBufferData(
//				id,
//				description.storage_block.aligned_size * description.array_size,
//				nullptr,
//				description.usage
//			);
//		}
	};
}
