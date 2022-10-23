#pragma once
#pragma message("-- read FILE/core.Hpp --")

#include "Lib/core/core.hpp"

namespace File
{
	ByteBuffer LoadAsBytes(std::filesystem::path const & path);

	ByteBuffer LoadAsBytes(std::filesystem::path const & path, usize file_size);

	std::string LoadAsString(std::filesystem::path const & path);

	struct Image
	{
		ByteBuffer buffer;
		i32x2 dimensions;
		i32 channels;
	};
	Image LoadImage(std::filesystem::path const & path);
}
