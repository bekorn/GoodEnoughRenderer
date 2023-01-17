#pragma once

#include "core.hpp"
#include "pixel_format.hpp"

namespace GL
{
	struct Texture2D : OpenGLObject
	{
		u64 handle;

		CTOR(Texture2D, default)
		COPY(Texture2D, delete)
		MOVE(Texture2D, default)

		~Texture2D()
		{
			glDeleteTextures(1, &id);
		}

		struct AttachmentDescription
		{
			i32x2 dimensions; // TODO(bekorn): rename to resolution
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

			handle = glGetTextureHandleARB(id);
			// Since all the textures will always be needed, their residency doesn't need management
			glMakeTextureHandleResidentARB(handle);
		}

		struct ImageDescription
		{
			i32x2 dimensions;
			bool has_alpha = false;
			COLOR_SPACE color_space = COLOR_SPACE::LINEAR_U8;

			// levels = 0 to generate mips all the wasy to 1x1
			i32 levels = 1;

			GLenum min_filter = GL_LINEAR;
			GLenum mag_filter = GL_LINEAR;

			GLenum wrap_s = GL_CLAMP_TO_EDGE;
			GLenum wrap_t = GL_CLAMP_TO_EDGE;

			span<byte> data = {};
		};

		void create(ImageDescription const & description)
		{
			glCreateTextures(GL_TEXTURE_2D, 1, &id);

			auto channel_count = description.has_alpha ? 4 : 3;

			// Pick correct levels
			i32 levels = description.levels != 0
				? description.levels
				: 1 + i32(glm::log2(f32(glm::compMax(description.dimensions))));

			// Pick correct internal format
			// TODO(bekorn): F32 takes too much space, it is here temporarily to handle some hdri cases, prefer F16
			GLenum format;
			switch (description.color_space)
			{
			case COLOR_SPACE::LINEAR_U8: format = description.has_alpha ? GL_RGBA8 : GL_RGB8;   break;
			case COLOR_SPACE::LINEAR_F32: format = description.has_alpha ? GL_RGBA32F : GL_RGB32F; break;
			case COLOR_SPACE::SRGB_U8: format = description.has_alpha ? GL_SRGB8_ALPHA8 : GL_SRGB8;  break;
			}

			glTextureStorage2D(
				id, levels, format,
				description.dimensions.x, description.dimensions.y
			);

			glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, description.min_filter);
			glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, description.mag_filter);

			glTextureParameteri(id, GL_TEXTURE_WRAP_S, description.wrap_s);
			glTextureParameteri(id, GL_TEXTURE_WRAP_T, description.wrap_t);

			if (not description.data.empty())
			{
				auto aligns_to_4 = (description.dimensions.x * channel_count) % 4 == 0;
				if (not aligns_to_4)
					glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

				glTextureSubImage2D(
					id,
					0,
					0, 0,
					description.dimensions.x, description.dimensions.y,
					description.has_alpha ? GL_RGBA : GL_RGB,
					description.color_space == COLOR_SPACE::LINEAR_F32 ? GL_FLOAT : GL_UNSIGNED_BYTE,
					description.data.data()
				);

				if (not aligns_to_4)
					glPixelStorei(GL_UNPACK_ALIGNMENT, 4); // set back to default

				if (not (description.min_filter == GL_NEAREST or description.min_filter == GL_LINEAR))
					glGenerateTextureMipmap(id);
			}

			handle = glGetTextureHandleARB(id);
			// Since all the textures will always be needed, their residency doesn't need management
			glMakeTextureHandleResidentARB(handle);
		}

		struct ViewDescription
		{
			Texture2D const & source;

			i32 base_level = 0;
			i32 level_count = 0; // 0 -> all levels

			optional<GLenum> min_filter = nullopt;
			optional<GLenum> mag_filter = nullopt;

			optional<GLenum> wrap_s = nullopt;
			optional<GLenum> wrap_t = nullopt;
		};

		void create(ViewDescription const & desc)
		{
			i32 level_count;

			if (desc.level_count != 0)
				level_count = desc.level_count;
			else
				glGetTextureParameteriv(desc.source.id, GL_TEXTURE_IMMUTABLE_LEVELS, &level_count);

			GLenum min_filter, mag_filter;

			if (desc.min_filter)
				min_filter = desc.min_filter.value();
			else
				glGetTextureParameteriv(desc.source.id, GL_TEXTURE_MIN_FILTER, &min_filter);

			if (desc.mag_filter)
				mag_filter = desc.mag_filter.value();
			else
				glGetTextureParameteriv(desc.source.id, GL_TEXTURE_MAG_FILTER, &mag_filter);

			GLenum wrap_s, wrap_t;

			if (desc.wrap_s)
				wrap_s = desc.wrap_s.value();
			else
				glGetTextureParameteriv(desc.source.id, GL_TEXTURE_WRAP_S, &wrap_s);

			if (desc.wrap_t)
				wrap_t = desc.wrap_t.value();
			else
				glGetTextureParameteriv(desc.source.id, GL_TEXTURE_WRAP_T, &wrap_t);

			GLenum internal_format;
			glGetTextureLevelParameteriv(desc.source.id, desc.base_level, GL_TEXTURE_INTERNAL_FORMAT, &internal_format);

			glGenTextures(1, &id);
			glTextureView(
				id, GL_TEXTURE_2D,
				desc.source.id, internal_format,
				desc.base_level, level_count,
				0, 1
			);

			glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, min_filter);
			glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, mag_filter);

			glTextureParameteri(id, GL_TEXTURE_WRAP_S, wrap_s);
			glTextureParameteri(id, GL_TEXTURE_WRAP_T, wrap_t);

			handle = glGetTextureHandleARB(id);
			// Since all the textures will always be needed, their residency doesn't need management
			glMakeTextureHandleResidentARB(handle);
		}
	};
}