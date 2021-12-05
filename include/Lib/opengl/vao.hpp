#pragma once

#include ".pch.hpp"
#include "core_types.hpp"

namespace GL
{
	struct Attribute
	{
		struct Description
		{
			Buffer const & buffer;
			usize byte_offset = 0;
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
			glGenVertexArrays(1, &id);
			glBindVertexArray(id);

			for (auto & attribute: description.attributes)
			{
				glBindBuffer(GL_ARRAY_BUFFER, attribute.buffer.id);
				glVertexAttribPointer(
					attribute.location,
					attribute.vector_dimension, attribute.vector_data_type,
					attribute.normalized,
					attribute.stride,
					reinterpret_cast<void*>(attribute.byte_offset)
				);
				glEnableVertexAttribArray(attribute.location);
			}

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, description.element_array.id);

			element_count = description.element_count;

			glBindVertexArray(0);
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
			glGenVertexArrays(1, &id);
			glBindVertexArray(id);

			for (auto & attribute: description.attributes)
			{
				glBindBuffer(GL_ARRAY_BUFFER, attribute.buffer.id);
				glVertexAttribPointer(
					attribute.location,
					attribute.vector_dimension, attribute.vector_data_type,
					false,
					attribute.stride,
					reinterpret_cast<void*>(attribute.byte_offset)
				);
				glEnableVertexAttribArray(attribute.location);
			}

			vertex_count = description.vertex_count;

			glBindVertexArray(0);
		}
	};
}