#pragma once

#include "core.hpp"

namespace GL
{
	struct Texture2D : OpenGLObject
	{
		CTOR(Texture2D, default)
		COPY(Texture2D, delete)
		MOVE(Texture2D, default)

		~Texture2D()
		{
			glDeleteTextures(1, &id);
		}

		struct AttachmentDescription
		{
			i32x2 dimensions;
			GLenum internal_format;

			GLenum min_filter = GL_LINEAR;
			GLenum mag_filter = GL_LINEAR;
		};

		void create(AttachmentDescription const & description)
		{
			glCreateTextures(GL_TEXTURE_2D, 1, &id);

			glTextureStorage2D(
				id,
				1,
				description.internal_format,
				description.dimensions.x, description.dimensions.y
			);

			glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, description.min_filter);
			glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, description.mag_filter);
		}

		struct ImageDescription
		{
			i32x2 dimensions;
			bool has_alpha = false;

			GLenum min_filter = GL_LINEAR;
			GLenum mag_filter = GL_LINEAR;

			GLenum wrap_s = GL_CLAMP_TO_BORDER;
			GLenum wrap_t = GL_CLAMP_TO_BORDER;

			span<byte> data = {};
		};

		void create(ImageDescription const & description)
		{
			glCreateTextures(GL_TEXTURE_2D, 1, &id);

			auto channel_count = description.has_alpha ? 4 : 3;
			auto channel_format = description.has_alpha ? GL_RGBA : GL_RGB;

			// Pick correct storage alignment
			auto aligns_to_4 = (description.dimensions.x * channel_count) % 4 == 0;
			if (not aligns_to_4)
				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

			glTextureStorage2D(
				id,
				1,
				GL_RGBA8,
				description.dimensions.x, description.dimensions.y
			);
			glTextureSubImage2D(
				id,
				0,
				0, 0,
				description.dimensions.x, description.dimensions.y,
				channel_format, GL_UNSIGNED_BYTE, description.data.data()
			);

			glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, description.min_filter);
			glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, description.mag_filter);

			glTextureParameteri(id, GL_TEXTURE_WRAP_S, description.wrap_s);
			glTextureParameteri(id, GL_TEXTURE_WRAP_T, description.wrap_t);

			if (not (description.min_filter == GL_NEAREST or description.min_filter == GL_LINEAR))
				glGenerateTextureMipmap(id);

			if (not aligns_to_4)
				glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
		}
	};
}