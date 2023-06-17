#pragma once

#include <core/core.hpp>
#include "Lib/opengl/core.hpp"
#include "Lib/opengl/pixel_format.hpp"

namespace Texture
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
	i32x2 dimensions;
	i32 channels;
	GL::COLOR_SPACE color_space;
	i32 levels;
	GL::GLenum min_filter;
	GL::GLenum mag_filter;
};

LoadedData Load(Desc const & desc);
}