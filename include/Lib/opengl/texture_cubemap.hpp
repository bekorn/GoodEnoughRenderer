#pragma once

#include "core.hpp"

namespace GL
{
	struct TextureCubemap : OpenGLObject
	{
		u64 handle;

		CTOR(TextureCubemap, default)
		COPY(TextureCubemap, delete)
		MOVE(TextureCubemap, default)

		~TextureCubemap()
		{
			glDeleteTextures(1, &id);
		}

		struct CubeMapImageDescription
		{
			i32x2 face_dimensions;
			bool has_alpha = false;
			bool is_sRGB = false;

			// levels = 0 to generate mips all the wasy to 1x1
			i32 levels = 1;

			GLenum min_filter = GL_LINEAR;
			GLenum mag_filter = GL_LINEAR;

			GLenum wrap_s = GL_CLAMP_TO_EDGE;
			GLenum wrap_t = GL_CLAMP_TO_EDGE;
			GLenum wrap_r = GL_CLAMP_TO_EDGE;

			span<byte> data = {};
		};

		void create(CubeMapImageDescription const & description)
		{
			glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &id);

			auto channel_count = description.has_alpha ? 4 : 3;

			// Pick correct storage alignment
			auto aligns_to_4 = (description.face_dimensions.x * channel_count) % 4 == 0;
			if (not aligns_to_4)
				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

			// Pick correct levels
			i32 levels = description.levels != 0
				? description.levels
				: 1 + i32(glm::log2(f32(glm::compMax(description.face_dimensions))));

			glTextureStorage2D(
				id,
				levels,
				description.is_sRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8,
				description.face_dimensions.x, description.face_dimensions.y
			);
			glTextureSubImage3D(
				id,
				0,
				0, 0, 0,
				description.face_dimensions.x, description.face_dimensions.y, 6,
				description.has_alpha ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, description.data.data()
			);

			glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, description.min_filter);
			glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, description.mag_filter);

			glTextureParameteri(id, GL_TEXTURE_WRAP_S, description.wrap_s);
			glTextureParameteri(id, GL_TEXTURE_WRAP_T, description.wrap_t);
			glTextureParameteri(id, GL_TEXTURE_WRAP_R, description.wrap_r);

			if (not(description.min_filter == GL_NEAREST or description.min_filter == GL_LINEAR))
				glGenerateTextureMipmap(id);

			if (not aligns_to_4)
				glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

			handle = glGetTextureHandleARB(id);
			// Since all the textures will always be needed, their residency doesn't need management
			glMakeTextureHandleResidentARB(handle);
		}
	};
}