#pragma once

#include "Lib/core/core.hpp"
#include "Lib/geometry/core.hpp"

#include "core.hpp"
#include "glue.hpp"

namespace GL
{
	struct Attribute
	{
		struct Description
		{
			index_ptr<vector<Buffer>> buffer;
			usize buffer_offset;
			optional<usize> buffer_stride;

			GLuint location;
			GLint vector_dimension;
			GLenum vector_data_type;
			bool normalized;
			GLint stride;
		};
	};

	struct VertexArray : OpenGLObject
	{
		vector<Buffer> vertex_buffers;
		Buffer element_buffer;
		GLsizei element_count;

		CTOR(VertexArray, default);
		COPY(VertexArray, delete);
		MOVE(VertexArray, default);

		~VertexArray()
		{
			glDeleteVertexArrays(1, &id);
		}

		void create(Geometry::Primitive const & geometry, ShaderProgram const & shader)
		{
			// TODO(bekorn): This method creates a vbo for each attribute,
			//  it would be better to merge them in a single vbo

			glCreateVertexArrays(1, &id);

			for (auto const & attribute: shader.attribute_mappings)
			{
				auto const geo_attribute = geometry.attributes.find(attribute.key);
				if (geo_attribute == geometry.attributes.end())
					continue;

				auto const & [key, data] = *geo_attribute;

				Buffer vertex_buffer;
				vertex_buffer.create(
					{
						.data = data.buffer.span_as<byte>(),
					}
				);
				auto const buffer_bind_index = vertex_buffers.size();

				glVertexArrayVertexBuffer(
					id,
					buffer_bind_index,
					vertex_buffer.id, 0, data.type.size() * data.dimension
				);
				glVertexArrayAttribFormat(
					id,
					attribute.location,
					data.dimension, IntoGLenum(data.type),data.type.is_normalized(),
					0
				);
				glEnableVertexArrayAttrib(id, attribute.location);
				glVertexArrayAttribBinding(id, attribute.location, buffer_bind_index);

				vertex_buffers.emplace_back(move(vertex_buffer));
			}

			element_buffer.create(
				{
					.data = span((byte*)geometry.indices.data(), geometry.indices.size() * sizeof(u32)),
				}
			);

			glVertexArrayElementBuffer(id, element_buffer.id);

			element_count = geometry.indices.size();
		}
	};

	struct VAO_ElementDraw : OpenGLObject
	{
		GLsizei element_count = 0;
		u32 element_offset;

		CTOR(VAO_ElementDraw, default);
		COPY(VAO_ElementDraw, delete);
		MOVE(VAO_ElementDraw, default);

		~VAO_ElementDraw()
		{
			glDeleteVertexArrays(1, &id);
		}

		struct Description
		{
			vector<Attribute::Description> const & attributes;
			Buffer const & element_array;
			u32 element_count;
			u32 element_offset;
		};

		void create(Description const & description)
		{
			glCreateVertexArrays(1, &id);

			for (auto & attribute: description.attributes)
			{
				glVertexArrayVertexBuffer(
					id,
					attribute.location,
					attribute.buffer->id,
					attribute.buffer_offset,
					attribute.buffer_stride.value_or(attribute.vector_dimension * ComponentTypeSize(attribute.vector_data_type))
				);
				glVertexArrayAttribFormat(
					id,
					attribute.location,
					attribute.vector_dimension, attribute.vector_data_type,
					attribute.normalized,
					0
				);
				glVertexArrayAttribBinding(id, attribute.location, attribute.location);
				glEnableVertexArrayAttrib(id, attribute.location);
			}

			glVertexArrayElementBuffer(id, description.element_array.id);
			element_offset = description.element_offset;

			element_count = description.element_count;
		}
	};

	struct VAO_ArrayDraw : OpenGLObject
	{
		GLsizei vertex_count = 0;

		CTOR(VAO_ArrayDraw, default);
		COPY(VAO_ArrayDraw, delete);
		MOVE(VAO_ArrayDraw, default);

		~VAO_ArrayDraw()
		{
			glDeleteVertexArrays(1, &id);
		}

		struct Description
		{
			vector<Attribute::Description> const & attributes;
			u32 vertex_count;
		};

		void create(Description const & description)
		{
			glCreateVertexArrays(1, &id);

			for (auto & attribute: description.attributes)
			{
				glVertexArrayVertexBuffer(
					id,
					attribute.location,
					attribute.buffer->id,
					attribute.buffer_offset,
					attribute.buffer_stride.value_or(attribute.vector_dimension * ComponentTypeSize(attribute.vector_data_type))
				);
				glVertexArrayAttribFormat(
					id,
					attribute.location,
					attribute.vector_dimension, attribute.vector_data_type,
					attribute.normalized,
					0
				);
				glVertexArrayAttribBinding(id, attribute.location, attribute.location);
				glEnableVertexArrayAttrib(id, attribute.location);
			}

			vertex_count = description.vertex_count;
		}
	};
}