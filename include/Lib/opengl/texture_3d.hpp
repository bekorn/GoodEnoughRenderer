#pragma once

#include "core.hpp"
#include "pixel_format.hpp"

namespace GL
{
struct Texture3D : OpenGLObject
{
	u64 handle;

	CTOR(Texture3D, default)
	COPY(Texture3D, delete)
	MOVE(Texture3D, default)

	~Texture3D()
	{
		glDeleteTextures(1, &id);
	}

	struct VolumeDesc
	{
		i32x3 dimensions;
		GLenum internal_format;
		i32 levels = 1; // levels = 0 -> generate mips all the way to 1x1

		GLenum min_filter = GL_LINEAR;
		GLenum mag_filter = GL_LINEAR;

		GLenum wrap_s = GL_CLAMP_TO_EDGE;
		GLenum wrap_t = GL_CLAMP_TO_EDGE;
		GLenum wrap_r = GL_CLAMP_TO_EDGE;

		span<byte> data = {};
		GLenum data_format;
		GLenum data_type;
	};

	void init(VolumeDesc const & desc)
	{
		glCreateTextures(GL_TEXTURE_3D, 1, &id);

		// Pick correct levels
		i32 levels = desc.levels != 0
					 ? desc.levels
					 : 1 + i32(glm::log2(f32(compMax(desc.dimensions))));

		glTextureStorage3D(
			id, levels, desc.internal_format,
			desc.dimensions.x, desc.dimensions.y, desc.dimensions.z
		);

		glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, desc.min_filter);
		glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, desc.mag_filter);

		glTextureParameteri(id, GL_TEXTURE_WRAP_S, desc.wrap_s);
		glTextureParameteri(id, GL_TEXTURE_WRAP_T, desc.wrap_t);
		glTextureParameteri(id, GL_TEXTURE_WRAP_R, desc.wrap_r);

		if (not desc.data.empty())
		{
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

			glTextureSubImage3D(
				id,
				0,
				0, 0, 0,
				desc.dimensions.x, desc.dimensions.y, desc.dimensions.z,
				desc.data_format, desc.data_type,
				desc.data.data()
			);

			glPixelStorei(GL_UNPACK_ALIGNMENT, 4); // set back to default

			if (not (desc.min_filter == GL_NEAREST or desc.min_filter == GL_LINEAR))
				glGenerateTextureMipmap(id);
		}

		handle = glGetTextureHandleARB(id);
		// Since all the textures will always be needed, their residency doesn't need management
		glMakeTextureHandleResidentARB(handle);
	}

	struct ViewDesc
	{
		Texture3D const & source;

		optional<GLenum> internal_format = nullopt;

		i32 base_level = 0;
		i32 level_count = 0; // 0 -> all levels

		optional<GLenum> min_filter = nullopt;
		optional<GLenum> mag_filter = nullopt;

		optional<GLenum> wrap_s = nullopt;
		optional<GLenum> wrap_t = nullopt;
		optional<GLenum> wrap_r = nullopt;
	};

	void init(ViewDesc const & desc)
	{
		i32 level_count;

		if (desc.level_count != 0) level_count = desc.level_count;
		else glGetTextureParameteriv(desc.source.id, GL_TEXTURE_IMMUTABLE_LEVELS, &level_count);

		GLenum min_filter, mag_filter;

		if (desc.min_filter) min_filter = desc.min_filter.value();
		else glGetTextureParameteriv(desc.source.id, GL_TEXTURE_MIN_FILTER, &min_filter);

		if (desc.mag_filter) mag_filter = desc.mag_filter.value();
		else glGetTextureParameteriv(desc.source.id, GL_TEXTURE_MAG_FILTER, &mag_filter);

		GLenum wrap_s, wrap_t, wrap_r;

		if (desc.wrap_s) wrap_s = desc.wrap_s.value();
		else glGetTextureParameteriv(desc.source.id, GL_TEXTURE_WRAP_S, &wrap_s);

		if (desc.wrap_t) wrap_t = desc.wrap_t.value();
		else glGetTextureParameteriv(desc.source.id, GL_TEXTURE_WRAP_T, &wrap_t);

		if (desc.wrap_r) wrap_r = desc.wrap_r.value();
		else glGetTextureParameteriv(desc.source.id, GL_TEXTURE_WRAP_R, &wrap_r);

		GLenum internal_format;

		if (desc.internal_format) internal_format = desc.internal_format.value();
		else glGetTextureLevelParameteriv(desc.source.id, desc.base_level, GL_TEXTURE_INTERNAL_FORMAT, &internal_format);

		glGenTextures(1, &id);
		glTextureView(
			id, GL_TEXTURE_3D,
			desc.source.id, internal_format,
			desc.base_level, level_count,
			0, 1
		);

		glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, min_filter);
		glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, mag_filter);

		glTextureParameteri(id, GL_TEXTURE_WRAP_S, wrap_s);
		glTextureParameteri(id, GL_TEXTURE_WRAP_T, wrap_t);
		glTextureParameteri(id, GL_TEXTURE_WRAP_R, wrap_r);

		handle = glGetTextureHandleARB(id);
		// Since all the textures will always be needed, their residency doesn't need management
		glMakeTextureHandleResidentARB(handle);
	}
};
}