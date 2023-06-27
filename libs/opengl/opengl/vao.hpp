#pragma once

#include <core/core.hpp>
#include <core/geometry.hpp>

#include "core.hpp"
#include "buffer.hpp"
#include "shader.hpp"
#include "utils.hpp"

namespace GL
{
struct VertexArray : OpenGLObject
{
	Buffer vertex_buffer;
	Buffer element_buffer;
	GLsizei element_count;

	CTOR(VertexArray, default);
	COPY(VertexArray, delete);
	MOVE(VertexArray, default);

	~VertexArray()
	{
		glDeleteVertexArrays(1, &id);
	}

	struct Desc
	{
		Geometry::Primitive const & primitive;
		GLenum usage = GL_STATIC_DRAW;
	};

	void init(Desc const & desc)
	{
		glCreateVertexArrays(1, &id);

		vertex_buffer.init(Buffer::Desc{
			.usage = desc.usage,
			.data = desc.primitive.data.buffer,
		});

		auto buffer_bind_idx = 0;
		GLintptr buffer_offset = 0;
		for (auto i = 0; i < Geometry::ATTRIBUTE_COUNT; i++)
		{
			auto const & attrib = desc.primitive.layout->attributes[i];
			if (not attrib.is_used()) continue;

			glVertexArrayVertexBuffer(
				id,
				buffer_bind_idx,
				vertex_buffer.id, buffer_offset, attrib.vec.size()
			);
			glVertexArrayAttribFormat(
				id,
				attrib.location,
				attrib.vec.dimension, to_glenum(attrib.vec.type), attrib.vec.type.is_normalized(),
				0
			);
			glEnableVertexArrayAttrib(id, attrib.location);
			glVertexArrayAttribBinding(id, attrib.location, buffer_bind_idx);

			buffer_bind_idx++;
			buffer_offset += static_cast<GLintptr>(attrib.vec.size() * desc.primitive.data.vertex_count);
		}

		element_buffer.init(Buffer::Desc{
			.usage = desc.usage,
			.data = span((byte *) desc.primitive.indices.data(), desc.primitive.indices.size() * sizeof(u32)),
		});

		glVertexArrayElementBuffer(id, element_buffer.id);

		element_count = desc.primitive.indices.size();
	}

	struct EmptyDesc
	{
		i32 vertex_count;
		Geometry::Layout const & layout;
		i32 element_count;
		GLenum usage = GL_STATIC_DRAW;
	};

	void init(EmptyDesc const & desc)
	{
		glCreateVertexArrays(1, &id);

		/// Vertex Buffer
		usize vertex_buffer_size = 0;
		for (auto const & attrib: desc.layout)
			if (attrib.is_used())
				vertex_buffer_size += desc.vertex_count * attrib.vec.size();

		vertex_buffer.init(Buffer::EmptyDesc{
			.usage = desc.usage,
			.size = vertex_buffer_size,
		});

		auto buffer_bind_idx = 0;
		GLintptr buffer_offset = 0;
		for (auto const & attrib : desc.layout)
		{
			if (not attrib.is_used()) continue;

			glVertexArrayVertexBuffer(
				id,
				buffer_bind_idx,
				vertex_buffer.id, buffer_offset, attrib.vec.size()
			);
			glVertexArrayAttribFormat(
				id,
				attrib.location,
				attrib.vec.dimension, to_glenum(attrib.vec.type), attrib.vec.type.is_normalized(),
				0
			);
			glEnableVertexArrayAttrib(id, attrib.location);
			glVertexArrayAttribBinding(id, attrib.location, buffer_bind_idx);

			buffer_bind_idx++;
			buffer_offset += static_cast<GLintptr>(desc.vertex_count * attrib.vec.size());
		}

		/// Element Buffer
		element_buffer.init(Buffer::EmptyDesc{
			.usage = desc.usage,
			.size = desc.element_count * sizeof(u32),
		});

		glVertexArrayElementBuffer(id, element_buffer.id);

		element_count = desc.element_count;
	}

	void update(Geometry::Primitive const & primitive)
	{
		{ // check assertions
			assert(element_count == primitive.indices.size(), "Dynamic element buffer is not supported yet");

			i32 current_buffer_size;
			glGetNamedBufferParameteriv(vertex_buffer.id, GL_BUFFER_SIZE, &current_buffer_size);
			assert(primitive.data.buffer.size == current_buffer_size, "Dynamic vertex buffer is not supported yet");
		}

		glInvalidateBufferData(vertex_buffer.id);
		glInvalidateBufferData(element_buffer.id);

		glNamedBufferSubData(
			vertex_buffer.id,
			0, primitive.data.buffer.size, primitive.data.buffer.data.get()
		);

		glNamedBufferSubData(
			element_buffer.id,
			0, primitive.indices.size() * sizeof(u32), primitive.indices.data()
		);
	}
};
}