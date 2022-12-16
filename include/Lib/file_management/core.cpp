#pragma message("-- read FILE/core.Cpp --")

#include <fstream>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_ONLY_HDR
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

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

	Image LoadImage(std::filesystem::path const & path, bool should_flip_vertically)
	{
		auto const file_data = LoadAsBytes(path);

		stbi_set_flip_vertically_on_load_thread(should_flip_vertically);

		i32x2 dimensions{0, 0};
		i32 channels;
		void * raw_pixel_data = stbi_load_from_memory(
			file_data.data_as<const unsigned char>(), file_data.size,
			&dimensions.x, &dimensions.y,
			&channels, 0
		);

		if (raw_pixel_data == nullptr)
			fmt::print(stderr, "File::LoadImage failed. path: {}, error: {}\n", path, stbi_failure_reason());

		return {
			.buffer = {
				move(raw_pixel_data),
				usize(dimensions.x * dimensions.y * channels)
			},
			.dimensions = dimensions,
			.channels = channels,
		};
	}

	void WriteImage(std::filesystem::path const & path, Image const & image)
	{
		auto p = path.string();
		auto success = stbi_write_png(
			p.c_str(),
			image.dimensions.x, image.dimensions.y,
			image.channels, image.buffer.data_as<void>(),
			image.dimensions.x * image.channels
		);

		if (not success)
			fmt::print(stderr, "File::WriteImage failed. path {}, error: {}\n", path, stbi_failure_reason());
	}
}