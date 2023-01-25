#pragma once

#include "Lib/core/core.hpp"
#include "Lib/opengl/core.hpp"

namespace Envmap
{
struct Desc
{
	std::filesystem::path path;
};

struct LoadedData
{
	vector<ByteBuffer> specular_mipmaps;
	ByteBuffer diffuse;
	i32x2 specular_face_dimensions;
	i32x2 diffuse_face_dimensions;
};

LoadedData Load(Desc const & desc);
}