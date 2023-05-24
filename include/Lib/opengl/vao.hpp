#pragma once

#include "Lib/core/core.hpp"
#include "Lib/geometry/core.hpp"

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
		Geometry::Primitive const & geometry;
		span<AttributeMapping const> attribute_mappings;
		GLenum usage = GL_STATIC_DRAW;
	};

	void init(Desc const & desc)
	{
		glCreateVertexArrays(1, &id);

		usize buffer_size = 0;
		for (auto const & attribute: desc.attribute_mappings)
		{
			auto const geo_attribute = desc.geometry.attributes.find(attribute.key);
			if (geo_attribute == desc.geometry.attributes.end())
				continue;

			auto const & [_, data] = *geo_attribute;
			buffer_size += data.buffer.size;
		}
		vertex_buffer.init({.size = buffer_size});

		auto buffer_bind_idx = 0;
		GLintptr buffer_offset = 0;
		for (auto const & attribute: desc.attribute_mappings)
		{
			auto const geo_attribute = desc.geometry.attributes.find(attribute.key);
			if (geo_attribute == desc.geometry.attributes.end())
				continue;

			auto const & [key, data] = *geo_attribute;

			glNamedBufferSubData(
				vertex_buffer.id,
				buffer_offset, data.buffer.size, data.buffer.begin()
			);

			glVertexArrayVertexBuffer(
				id,
				buffer_bind_idx,
				vertex_buffer.id, buffer_offset, data.type.size() * data.dimension
			);
			glVertexArrayAttribFormat(
				id,
				attribute.location,
				data.dimension, to_glenum(data.type), data.type.is_normalized(),
				0
			);
			glEnableVertexArrayAttrib(id, attribute.location);
			glVertexArrayAttribBinding(id, attribute.location, buffer_bind_idx);

			buffer_bind_idx++;
			buffer_offset += static_cast<GLintptr>(data.buffer.size);
		}

		element_buffer.init(Buffer::Desc{
			.data = span((byte *) desc.geometry.indices.data(), desc.geometry.indices.size() * sizeof(u32)),
		});

		glVertexArrayElementBuffer(id, element_buffer.id);

		element_count = desc.geometry.indices.size();
	}

	void update(Geometry::Primitive const & geometry, span<AttributeMapping const> attribute_mappings)
	{
		{ // check assertions
			assert(element_count == geometry.indices.size(), "Dynamic element buffer is not supported yet");

			usize new_buffer_size = 0;
			for (auto const & attribute: attribute_mappings)
			{
				auto const geo_attribute = geometry.attributes.find(attribute.key);
				if (geo_attribute == geometry.attributes.end())
					continue;

				auto const & [_, data] = *geo_attribute;
				new_buffer_size += data.buffer.size;
			}
			i32 current_buffer_size;
			glGetNamedBufferParameteriv(vertex_buffer.id, GL_BUFFER_SIZE, &current_buffer_size);
			assert(new_buffer_size == current_buffer_size, "Dynamic vertex buffer is not supported yet");
		}

		glInvalidateBufferData(vertex_buffer.id);
		glInvalidateBufferData(element_buffer.id);

		auto buffer_bind_idx = 0;
		GLintptr buffer_offset = 0;
		for (auto const & attribute: attribute_mappings)
		{
			auto const geo_attribute = geometry.attributes.find(attribute.key);
			if (geo_attribute == geometry.attributes.end())
				continue;

			auto const & [key, data] = *geo_attribute;

			glNamedBufferSubData(
				vertex_buffer.id,
				buffer_offset, data.buffer.size, data.buffer.begin()
			);

			glVertexArrayVertexBuffer(
				id,
				buffer_bind_idx,
				vertex_buffer.id, buffer_offset, data.type.size() * data.dimension
			);
			glVertexArrayAttribFormat(
				id,
				attribute.location,
				data.dimension, to_glenum(data.type), data.type.is_normalized(),
				0
			);
			glEnableVertexArrayAttrib(id, attribute.location);
			glVertexArrayAttribBinding(id, attribute.location, buffer_bind_idx);

			buffer_bind_idx++;
			buffer_offset += static_cast<GLintptr>(data.buffer.size);
		}

		glNamedBufferSubData(
			element_buffer.id,
			0, geometry.indices.size() * sizeof(u32), geometry.indices.data()
		);
	}
};
}