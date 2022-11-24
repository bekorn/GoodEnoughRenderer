#include "load.hpp"
#include "convert.hpp"

#include "Lib/file_management/core.hpp"

namespace Cubemap
{
	namespace Helpers
	{
		GL::GLenum ToGLenum(std::string_view filter)
		{
			using namespace std::string_view_literals;
			if (filter == "NEAREST"sv) return GL::GL_NEAREST;
			if (filter == "LINEAR"sv) return GL::GL_LINEAR;
			if (filter == "NEAREST_MIPMAP_NEAREST"sv) return GL::GL_NEAREST_MIPMAP_NEAREST;
			if (filter == "NEAREST_MIPMAP_LINEAR"sv) return GL::GL_NEAREST_MIPMAP_LINEAR;
			if (filter == "LINEAR_MIPMAP_NEAREST"sv) return GL::GL_LINEAR_MIPMAP_NEAREST;
			if (filter == "LINEAR_MIPMAP_LINEAR"sv) return GL::GL_LINEAR_MIPMAP_LINEAR;
			assert(("Cubemap filter is unknown", false));
		}
	}

	std::pair<Name, Description> Parse(File::JSON::JSONObj o, std::filesystem::path const & root_dir)
	{
		Description description;
		description.path = root_dir / o.FindMember("path")->value.GetString();
		description.levels = File::JSON::GetI32(o, "levels", 1);
		description.min_filter = Helpers::ToGLenum(
			File::JSON::GetString(o, "min_filter", description.levels == 1 ? "LINEAR" : "LINEAR_MIPMAP_LINEAR")
		);
		description.mag_filter = Helpers::ToGLenum(
			File::JSON::GetString(o, "mag_filter", "LINEAR")
		);

		return {
			o.FindMember("name")->value.GetString(),
			description
		};
	}

	LoadedData Load(Description const & description)
	{
		auto image_file = File::LoadImage(description.path);
		auto face_dimensions = image_file.dimensions / i32x2{6, 1};

		auto buffer = ByteBuffer(image_file.buffer.size);
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

		return LoadedData{
			.data = move(buffer),
			.face_dimensions = face_dimensions,
			.channels = image_file.channels,
			.is_sRGB = false,
			.levels = description.levels,
			.min_filter = description.min_filter,
			.mag_filter = description.mag_filter,
		};
	}

	GL::TextureCubemap Convert(LoadedData const & loaded)
	{
		GL::TextureCubemap cubemap;
		cubemap.create(
			GL::TextureCubemap::ImageDescription{
				.face_dimensions = loaded.face_dimensions,
				.has_alpha = loaded.channels == 4,
				.is_sRGB = loaded.is_sRGB,
				.levels = loaded.levels,
				.min_filter = loaded.min_filter,
				.mag_filter = loaded.mag_filter,
				.data = loaded.data.span_as<byte>(),
			}
		);
		return cubemap;
	}
}
