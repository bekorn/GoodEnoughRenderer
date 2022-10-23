#pragma message("-- read FILE/core.Cpp --")

#include <fstream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "core.hpp"

namespace File
{
	ByteBuffer LoadAsBytes(std::filesystem::path const & path)
	{
		assert(std::filesystem::exists(path));
		std::basic_ifstream<byte> file(path, std::ios::in | std::ios::binary | std::ios::ate);

		usize file_size = file.tellg();
		ByteBuffer buffer(file_size);

		file.seekg(0);
		file.read(buffer.data.get(), file_size);

		return buffer;
	}

	ByteBuffer LoadAsBytes(std::filesystem::path const & path, usize file_size)
	{
		assert(std::filesystem::exists(path));
		std::basic_ifstream<byte> file(path, std::ios::in | std::ios::binary);

		ByteBuffer buffer(file_size);
		file.read(buffer.data.get(), file_size);

		return buffer;
	}

	std::string LoadAsString(std::filesystem::path const & path)
	{
		assert(std::filesystem::exists(path));
		std::basic_ifstream<char> file(path, std::ios::in | std::ios::binary | std::ios::ate);

		usize file_size = file.tellg();
		std::string buffer(file_size, '\0');

		file.seekg(0);
		file.read(buffer.data(), file_size);

		return buffer;
	}

	Image LoadImage(std::filesystem::path const & path)
	{
		auto const file_data = LoadAsBytes(path);

		i32x2 dimensions;
		i32 channels;
		void * raw_pixel_data = stbi_load_from_memory(
			file_data.data_as<const unsigned char>(), file_data.size,
			&dimensions.x, &dimensions.y,
			&channels, 0
		);

		return {
			.buffer = {
				move(raw_pixel_data),
				usize(dimensions.x * dimensions.y * channels)
			},
			.dimensions = dimensions,
			.channels = channels,
		};
	}
}