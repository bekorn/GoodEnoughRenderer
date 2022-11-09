#pragma once

#include "Lib/core/core.hpp"

namespace Cubemap
{
	struct Description
	{
		std::filesystem::path path;
	};

	struct LoadedData
	{
		ByteBuffer data;
		i32x2 face_dimensions;
		i32 channels;
		bool is_sRGB;
	};

	LoadedData Load(Description const & description);
}