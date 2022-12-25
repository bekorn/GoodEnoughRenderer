#pragma once

#include "core.hpp"
#include "pixel_format.hpp"

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
			COLOR_SPACE color_space = COLOR_SPACE::LINEAR_BYTE;

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

			// Pick correct levels
			i32 levels = description.levels != 0
				? description.levels
				: 1 + i32(glm::log2(f32(glm::compMax(description.face_dimensions))));

			// Pick correct internal format
			// TODO(bekorn): F32 takes too much space, it is here temporarily to handle some hdri cases, prefer F16
			GLenum format;
			switch (description.color_space)
			{
			case COLOR_SPACE::LINEAR_BYTE:  format = description.has_alpha ? GL_RGBA8        : GL_RGB8;   break;
			case COLOR_SPACE::LINEAR_FLOAT: format = description.has_alpha ? GL_RGBA32F      : GL_RGB32F; break;
			case COLOR_SPACE::SRGB:         format = description.has_alpha ? GL_SRGB8_ALPHA8 : GL_SRGB8;  break;
			}

			glTextureStorage2D(
				id, levels, format,
				description.face_dimensions.x, description.face_dimensions.y
			);

			glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, description.min_filter);
			glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, description.mag_filter);

			// https://registry.khronos.org/OpenGL/extensions/AMD/AMD_seamless_cubemap_per_texture.txt
			glTextureParameteri(id, GL_TEXTURE_CUBE_MAP_SEAMLESS, true);

			if (not description.data.empty())
			{
				auto aligns_to_4 = (description.face_dimensions.x * channel_count) % 4 == 0;
				if (not aligns_to_4)
					glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

				glTextureSubImage3D(
					id,
					0,
					0, 0, 0,
					description.face_dimensions.x, description.face_dimensions.y, 6,
					description.has_alpha ? GL_RGBA : GL_RGB,
					description.color_space == COLOR_SPACE::LINEAR_FLOAT ? GL_FLOAT : GL_UNSIGNED_BYTE,
					description.data.data()
				);

				if (not aligns_to_4)
					glPixelStorei(GL_UNPACK_ALIGNMENT, 4); // set back to default

				if (not(description.min_filter == GL_NEAREST or description.min_filter == GL_LINEAR))
					glGenerateTextureMipmap(id);
			}

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

		void create(ViewDescription const & dsc)
		{
			i32 level_count;

			if (dsc.level_count != 0)
				level_count = dsc.level_count;
			else
				glGetTextureParameteriv(dsc.source.id, GL_TEXTURE_IMMUTABLE_LEVELS, &level_count);

			GLenum min_filter, mag_filter;

			if (dsc.min_filter)
				min_filter = dsc.min_filter.value();
			else
				glGetTextureParameteriv(dsc.source.id, GL_TEXTURE_MIN_FILTER, &min_filter);

			if (dsc.mag_filter)
				mag_filter = dsc.mag_filter.value();
			else
				glGetTextureParameteriv(dsc.source.id, GL_TEXTURE_MAG_FILTER, &mag_filter);

			GLenum internal_format;
			glGetTextureLevelParameteriv(dsc.source.id, dsc.base_level, GL_TEXTURE_INTERNAL_FORMAT, &internal_format);

			glGenTextures(1, &id);
			glTextureView(
				id, GL_TEXTURE_CUBE_MAP,
				dsc.source.id, internal_format,
				dsc.base_level, level_count,
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