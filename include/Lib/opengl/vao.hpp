#pragma once

#include "core.hpp"

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

	struct VAO_ElementDraw : OpenGLObject
	{
		GLsizei element_count = 0;

		VAO_ElementDraw() noexcept = default;
		VAO_ElementDraw(VAO_ElementDraw&&) noexcept = default;

		~VAO_ElementDraw()
		{
			glDeleteVertexArrays(1, &id);
		}

		struct Description
		{
			vector<Attribute::Description> const & attributes;
			Buffer const & element_array;
			u32 element_count;
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

			element_count = description.element_count;
		}
	};

	struct VAO_ArrayDraw : OpenGLObject
	{
		GLsizei vertex_count = 0;

		VAO_ArrayDraw() noexcept = default;
		VAO_ArrayDraw(VAO_ArrayDraw&&) noexcept = default;

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