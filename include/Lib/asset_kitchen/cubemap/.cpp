#include "load.hpp"
#include "convert.hpp"

#include "Lib/file_management/core.hpp"

namespace Cubemap
{
namespace Helpers
{
GL::GLenum to_glenum(std::string_view filter)
{
	using namespace std::string_view_literals;
	if (filter == "NEAREST"sv) return GL::GL_NEAREST;
	if (filter == "LINEAR"sv) return GL::GL_LINEAR;
	if (filter == "NEAREST_MIPMAP_NEAREST"sv) return GL::GL_NEAREST_MIPMAP_NEAREST;
	if (filter == "NEAREST_MIPMAP_LINEAR"sv) return GL::GL_NEAREST_MIPMAP_LINEAR;
	if (filter == "LINEAR_MIPMAP_NEAREST"sv) return GL::GL_LINEAR_MIPMAP_NEAREST;
	if (filter == "LINEAR_MIPMAP_LINEAR"sv) return GL::GL_LINEAR_MIPMAP_LINEAR;
	assert_case_not_handled();
}
}

std::pair<Name, Desc> Parse(File::JSON::JSONObj o, std::filesystem::path const & root_dir)
{
	Desc desc;
	desc.path = root_dir / o.FindMember("path")->value.GetString();
	desc.levels = File::JSON::GetI32(o, "levels", 1);
	desc.min_filter = Helpers::to_glenum(
		File::JSON::GetString(o, "min_filter", desc.levels == 1 ? "LINEAR" : "LINEAR_MIPMAP_LINEAR")
	);
	desc.mag_filter = Helpers::to_glenum(
		File::JSON::GetString(o, "mag_filter", "LINEAR")
	);

	return {
		o.FindMember("name")->value.GetString(),
		desc
	};
}

LoadedData Load(Desc const & desc)
{
	// regular textures (first-pixel == uv(0,1)) require a vertical flip,
	// but cubemaps are expecting first-pixel == uv(0,1) already
	auto image_file = File::LoadImage(desc.path, false);

	auto loaded_data = LoadedData{
		.channels = image_file.channels,
		.is_sRGB = false,
		.levels = desc.levels,
		.min_filter = desc.min_filter,
		.mag_filter = desc.mag_filter,
	};

	if (6 * image_file.dimensions.x == image_file.dimensions.y)
	{
		// faces are stacked vertically (face pixels are separate)
		loaded_data.face_dimensions = image_file.dimensions / i32x2{1, 6};
		loaded_data.data = move(image_file.buffer);
	}
	else if (image_file.dimensions.x == 6 * image_file.dimensions.y)
	{
		// faces are stacked horizontally (face pixels are interleaved)
		auto face_dimensions = image_file.dimensions / i32x2{6, 1};
		auto buffer = ByteBuffer(image_file.buffer.size);

		// un-interleave the buffer (basically turns it into above case)
		auto dst_data = buffer.data_as<u8>();
		auto dst_idx = 0;
		auto src_data = image_file.buffer.data_as<u8>();
		for (auto face = 0; face < 6; ++face)
		{
			auto src_idx = face * face_dimensions.x * image_file.channels;
			for (auto y = 0; y < face_dimensions.y; ++y)
			{
				std::memcpy(dst_data + dst_idx, src_data + src_idx, face_dimensions.x * image_file.channels);
				dst_idx += face_dimensions.x * image_file.channels;
				src_idx += image_file.dimensions.x * image_file.channels;
			}
		}

		loaded_data.face_dimensions = face_dimensions;
		loaded_data.data = move(buffer);
	}

	return loaded_data;
}

GL::TextureCubemap Convert(LoadedData const & loaded)
{
	GL::TextureCubemap cubemap;
	cubemap.init(
		GL::TextureCubemap::ImageDesc{
			.face_dimensions = loaded.face_dimensions,
			.has_alpha = loaded.channels == 4,
			.color_space = loaded.is_sRGB ? GL::COLOR_SPACE::SRGB_U8 : GL::COLOR_SPACE::LINEAR_U8,
			.levels = loaded.levels,
			.min_filter = loaded.min_filter,
			.mag_filter = loaded.mag_filter,
			.data = loaded.data.span_as<byte>(),
		}
	);
	return cubemap;
}
}
