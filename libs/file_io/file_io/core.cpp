#pragma message("-- read FILE/core.Cpp --")

#include <fstream>

#include <lz4.h>

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
ByteBuffer LoadAsBytes(Path const & path)
{
	assert(std::filesystem::exists(path));
	std::basic_ifstream<byte> file(path, std::ios::in | std::ios::binary | std::ios::ate);

	usize file_size = file.tellg();
	ByteBuffer buffer(file_size);

	file.seekg(0);
	file.read(buffer.data.get(), file_size);

	return buffer;
}

ByteBuffer LoadAsBytes(Path const & path, usize file_size)
{
	assert(std::filesystem::exists(path));
	std::basic_ifstream<byte> file(path, std::ios::in | std::ios::binary);

	ByteBuffer buffer(file_size);
	file.read(buffer.data.get(), file_size);

	return buffer;
}
void WriteBytes(Path const & path, ByteBuffer const & buffer)
{
	std::basic_ofstream<byte> file(path, std::ios::out | std::ios::binary);
	if (file.fail())
	{
		fmt::print(stderr, "File::WriteBytes failed to open. path: {}\n", path);
		return;
	}

	file.write(buffer.data.get(), buffer.size), file.close();
	if (file.bad())
	{
		fmt::print(stderr, "File::WriteBytes failed to write. path: {}\n", path);
		return;
	}
}

std::string LoadAsString(Path const & path)
{
	assert(std::filesystem::exists(path));
	std::basic_ifstream<char> file(path, std::ios::in | std::ios::ate);

	usize file_size = file.tellg();
	std::string buffer(file_size, '\0');

	file.seekg(0);
	file.read(buffer.data(), file_size);

	return buffer;
}
void WriteString(Path const & path, std::string_view sv)
{
	std::basic_ofstream<char> file(path, std::ios::out);
	if (file.fail())
	{
		fmt::print(stderr, "File::WriteString failed to open. path: {}\n", path);
		return;
	}

	file.write(sv.data(), sv.size()), file.close();
	if (file.bad())
	{
		fmt::print(stderr, "File::WriteString failed to write. path: {}\n", path);
		return;
	}
}

Image LoadImage(Path const & path, bool should_flip_vertically)
{
	auto buffer = LoadAsBytes(path);
	auto outcome = DecodeImage(buffer, should_flip_vertically);

	if (outcome)
		return outcome.into_result();

	fmt::print(stderr, "File::DecodeImage failed. path: {}, error: {}\n", path, outcome.into_error());
	return {};
}
Expected<Image, const char *> DecodeImage(span<byte> buffer, bool should_flip_vertically)
{
	stbi_set_flip_vertically_on_load_thread(should_flip_vertically);

	auto const buffer_begin = reinterpret_cast<const unsigned char *>(buffer.data());
	auto const buffer_size = buffer.size();

	Image image;
	image.is_format_f32 = stbi_is_hdr_from_memory(buffer_begin, buffer_size);

	void * decoded_buffer;
	if (image.is_format_f32)
		decoded_buffer = stbi_loadf_from_memory(
			buffer_begin, buffer_size,
			&image.dimensions.x, &image.dimensions.y,
			&image.channels, 0
		);
	else
		decoded_buffer = stbi_load_from_memory(
			buffer_begin, buffer_size,
			&image.dimensions.x, &image.dimensions.y,
			&image.channels, 0
		);

	if (decoded_buffer == nullptr)
		return {stbi_failure_reason()};

	image.buffer = ByteBuffer(move(decoded_buffer), image.dimensions.x * image.dimensions.y * image.channels);

	return {move(image)};
}
void WriteImage(Path const & path, Image const & image, bool should_flip_vertically)
{
	auto p = path.string();

	stbi_flip_vertically_on_write(should_flip_vertically);

	bool success;
	if (image.is_format_f32)
		success = stbi_write_hdr(
			p.c_str(),
			image.dimensions.x, image.dimensions.y,
			image.channels, image.buffer.data_as<float>()
		);
	else
		success = stbi_write_png(
			p.c_str(),
			image.dimensions.x, image.dimensions.y,
			image.channels, image.buffer.data_as<void>(),
			image.dimensions.x * image.channels
		);

	if (not success)
		fmt::print(stderr, "File::WriteImage failed. path {}, error: {}\n", path, stbi_failure_reason());
}

ByteBuffer Compress(const ByteBuffer & src)
{
	auto dst = ByteBuffer(LZ4_compressBound(src.size));
	auto compressed_size = LZ4_compress_default(src.data_as<const char>(), dst.data_as<char>(), src.size, dst.size);
	assert(compressed_size > 0, "File::Compress failed");
	dst.size = compressed_size; // this breaks the relation between the allocated memory and size but it should be fine
	// TODO(bekorn): realloc?
	return dst;
}
ByteBuffer DeCompress(const ByteBuffer & src, usize uncompressed_size)
{
	auto dst = ByteBuffer(uncompressed_size);
	auto decompressed_size = LZ4_decompress_safe(src.data_as<const char>(), dst.data_as<char>(), src.size, dst.size);
	assert(decompressed_size == uncompressed_size, "File::DeCompress failed");
	dst.size = decompressed_size; // this breaks the relation between the allocated memory and size but it should be fine
	// TODO(bekorn): realloc?
	return dst;
}

optional<std::error_code> ClearFolder(Path const & path)
{
	std::error_code ec;
	for (auto & item : std::filesystem::directory_iterator(path))
	{
		std::filesystem::remove_all(item.path(), ec);
		if (ec) return ec;
	}
	return nullopt;
}
}