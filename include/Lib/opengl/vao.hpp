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
			std::vector<Attribute::Description> const & attributes;
			Buffer const & element_array;
		};

		void create(Description const & description)
		{
			assert(not description.attributes.empty());

			glGenVertexArrays(1, &id);
			glBindVertexArray(id);

			for (auto & attribute: description.attributes)
			{
				assert(attribute.buffer.type == GL_ARRAY_BUFFER);
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

			assert(description.element_array.type == GL_ELEMENT_ARRAY_BUFFER);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, description.element_array.id);

			// Query element count
			glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &element_count);

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
			std::vector<Attribute::Description> const & attributes;
		};

		void create(Description const & description)
		{
			assert(not description.attributes.empty());

			glGenVertexArrays(1, &id);
			glBindVertexArray(id);

			for (auto & attribute: description.attributes)
			{
				assert(attribute.buffer.type == GL_ARRAY_BUFFER);
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

			// Query vertex count
			auto const & attr = description.attributes.front();
			glBindBuffer(attr.buffer.type, attr.buffer.id);
			glGetBufferParameteriv(attr.buffer.type, GL_BUFFER_SIZE, &vertex_count);
			vertex_count /= attr.vector_dimension;

			glBindVertexArray(0);
		}
	};
}