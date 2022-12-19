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

		struct ImageDescription
		{
			i32x2 face_dimensions;
			bool has_alpha = false;
			bool is_sRGB = false;

			// levels = 0 to generate mips all the way to 1x1
			i32 levels = 1;

			GLenum min_filter = GL_LINEAR;
			GLenum mag_filter = GL_LINEAR;

			span<byte> data = {};
		};

		void create(ImageDescription const & description)
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
			if (not description.data.empty())
				glTextureSubImage3D(
					id,
					0,
					0, 0, 0,
					description.face_dimensions.x, description.face_dimensions.y, 6,
					description.has_alpha ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, description.data.data()
				);

			glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, description.min_filter);
			glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, description.mag_filter);

			// https://registry.khronos.org/OpenGL/extensions/AMD/AMD_seamless_cubemap_per_texture.txt
			glTextureParameteri(id, GL_TEXTURE_CUBE_MAP_SEAMLESS, true);

			if (not description.data.empty() and not(description.min_filter == GL_NEAREST or description.min_filter == GL_LINEAR))
				glGenerateTextureMipmap(id);

			if (not aligns_to_4)
				glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

			handle = glGetTextureHandleARB(id);
			// Since all the textures will always be needed, their residency doesn't need management
			glMakeTextureHandleResidentARB(handle);
		}

		struct ViewDescription
		{
			TextureCubemap const & source;

			i32 base_level = 0;
			i32 level_count = 0; // 0 -> all levels

			optional<GLenum> min_filter = nullopt;
			optional<GLenum> mag_filter = nullopt;
		};

		void create(ViewDescription const & description)
		{
			i32 level_count;

			if (description.level_count != 0)
				level_count = description.level_count;
			else
				glGetTextureParameteriv(description.source.id, GL_TEXTURE_IMMUTABLE_LEVELS, &level_count);

			GLenum min_filter, mag_filter;

			if (description.min_filter.has_value())
				min_filter = description.min_filter.value();
			else
				glGetTextureParameteriv(description.source.id, GL_TEXTURE_MIN_FILTER, &min_filter);

			if (description.mag_filter.has_value())
				mag_filter = description.mag_filter.value();
			else
				glGetTextureParameteriv(description.source.id, GL_TEXTURE_MAG_FILTER, &mag_filter);


			glGenTextures(1, &id);
			glTextureView(
				id, GL_TEXTURE_CUBE_MAP,
				description.source.id, GL_RGBA8,
				description.base_level, level_count,
				0, 6
			);

			glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, min_filter);
			glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, mag_filter);

			// https://registry.khronos.org/OpenGL/extensions/AMD/AMD_seamless_cubemap_per_texture.txt
			glTextureParameteri(id, GL_TEXTURE_CUBE_MAP_SEAMLESS, true);

			handle = glGetTextureHandleARB(id);
			// Since all the textures will always be needed, their residency doesn't need management
			glMakeTextureHandleResidentARB(handle);
		}
	};
}