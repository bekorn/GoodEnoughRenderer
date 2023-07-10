#pragma once
#pragma message("-- read FILE/core.Hpp --")

#include <core/core.hpp>
#include <core/expected.hpp>

namespace File
{
using Path = std::filesystem::path;

ByteBuffer LoadAsBytes(Path const & path);

ByteBuffer LoadAsBytes(Path const & path, usize file_size);
void WriteBytes(Path const & path, ByteBuffer const & buffer);

std::string LoadAsString(Path const & path);
void WriteString(Path const & path, std::string_view sv);

struct Image
{
	ByteBuffer buffer;
	i32x2 dimensions;
	i32 channels;
	bool is_format_f32; // otherwise format is u8
};
Image LoadImage(Path const & path, bool should_flip_vertically);
Expected<Image, const char *> DecodeImage(span<byte> buffer, bool should_flip_vertically);
void WriteImage(Path const & path, Image const & image, bool should_flip_vertically);

ByteBuffer Compress(ByteBuffer const & buffer);
ByteBuffer DeCompress(ByteBuffer const & buffer, usize uncompressed_size);

optional<std::error_code> ClearFolder(Path const & path);
}
