#pragma once

#include ".pch.hpp"
#include "core_types.hpp"

namespace GL
{
	struct Texture2D : OpenGLObject
	{
		Texture2D() noexcept = default;

		~Texture2D()
		{
			glDeleteTextures(1, &id);
		}

		struct Description
		{
			i32x2 size;

			bool has_alpha = false;

			GLenum min_filter = GL_LINEAR;
			GLenum max_filter = GL_LINEAR;

			GLenum wrap_s = GL_CLAMP_TO_BORDER;
			GLenum wrap_t = GL_CLAMP_TO_BORDER;
		};

		void create(Description const & description)
		{
			assert(description.size.x <= GL_MAX_TEXTURE_SIZE);
			assert(description.size.y <= GL_MAX_TEXTURE_SIZE);

			glGenTextures(1, &id);

			auto channel_count = description.has_alpha ? 4 : 3;
			auto channel_format = description.has_alpha ? GL_RGBA : GL_RGB;

			// Pick correct storage alignment
			auto aligns_to_4 = (description.size.x * channel_count) % 4 == 0;
			if (not aligns_to_4)
				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

			glBindTexture(GL_TEXTURE_2D, id);
			glTexImage2D(
				GL_TEXTURE_2D,
				0,
				GL_RGBA,
				description.size.x, description.size.y,
				0, channel_format, GL_UNSIGNED_BYTE, nullptr
			);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, description.min_filter);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, description.max_filter);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, description.wrap_s);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, description.wrap_t);

			// TODO(bekorn): this will be needed for specific filter types
			// glGenerateMipmap(GL_TEXTURE_2D);

			if (not aligns_to_4)
				glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
		}
	};
}