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
		bool is_format_f32; // otherwise format is u8
	};
	Image LoadImage(std::filesystem::path const & path, bool should_flip_vertically);

	void WriteImage(std::filesystem::path const & path, Image const & image, bool should_flip_vertically = true);
}
