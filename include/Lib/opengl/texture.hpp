#pragma once

#include ".pch.hpp"
#include "core_types.hpp"

namespace GL
{
	struct Texture2D : OpenGLObject
	{
		Texture2D() noexcept = default;
		Texture2D(Texture2D &&) noexcept = default;

		~Texture2D()
		{
			glDeleteTextures(1, &id);
		}

		struct AttachmentDescription
		{
			i32x2 size;

			GLenum internal_format;
			GLenum format;

			GLenum min_filter = GL_LINEAR;
			GLenum mag_filter = GL_LINEAR;
		};

		void create(AttachmentDescription const & description)
		{
			glGenTextures(1, &id);

			glBindTexture(GL_TEXTURE_2D, id);
			glTexImage2D(
				GL_TEXTURE_2D,
				0,
				description.internal_format,
				description.size.x, description.size.y,
				0, description.format, GL_UNSIGNED_BYTE, nullptr
			);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, description.min_filter);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, description.mag_filter);
		}

		struct ImageDescription
		{
			i32x2 dimensions;

			bool has_alpha = false;

			GLenum min_filter = GL_LINEAR;
			GLenum mag_filter = GL_LINEAR;

			GLenum wrap_s = GL_CLAMP_TO_BORDER;
			GLenum wrap_t = GL_CLAMP_TO_BORDER;

			const void* data = nullptr;
		};

		void create(ImageDescription const & description)
		{
			glGenTextures(1, &id);

			auto channel_count = description.has_alpha ? 4 : 3;
			auto channel_format = description.has_alpha ? GL_RGBA : GL_RGB;

			// Pick correct storage alignment
			auto aligns_to_4 = (description.dimensions.x * channel_count) % 4 == 0;
			if (not aligns_to_4)
				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

			glBindTexture(GL_TEXTURE_2D, id);
			glTexImage2D(
				GL_TEXTURE_2D,
				0,
				GL_RGBA,
				description.dimensions.x, description.dimensions.y,
				0, channel_format, GL_UNSIGNED_BYTE, description.data
			);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, description.min_filter);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, description.mag_filter);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, description.wrap_s);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, description.wrap_t);

			// TODO(bekorn): this will be needed for specific filter types
			// glGenerateMipmap(GL_TEXTURE_2D);

			if (not aligns_to_4)
				glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
		}
	};
}