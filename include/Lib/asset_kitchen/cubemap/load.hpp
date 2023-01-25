#pragma once

#include "Lib/core/core.hpp"
#include "Lib/opengl/core.hpp"

namespace Cubemap
{
struct Desc
{
	std::filesystem::path path;
	i32 levels;
	GL::GLenum min_filter;
	GL::GLenum mag_filter;
};

struct LoadedData
{
	ByteBuffer data;
	i32x2 face_dimensions;
	i32 channels;
	bool is_sRGB;
	i32 levels;
	GL::GLenum min_filter;
	GL::GLenum mag_filter;
};

LoadedData Load(Desc const & desc);
}