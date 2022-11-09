#include "load.hpp"
#include "convert.hpp"

#include "Lib/file_management/core.hpp"

namespace Cubemap
{
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
		};
	}

	GL::TextureCubemap Convert(LoadedData const & loaded)
	{
		GL::TextureCubemap cubemap;
		cubemap.create(
			GL::TextureCubemap::CubeMapImageDescription{
				.face_dimensions = loaded.face_dimensions,
				.has_alpha = loaded.channels == 4,
				.is_sRGB = loaded.is_sRGB,
				.min_filter = GL::GL_LINEAR_MIPMAP_LINEAR,
				.mag_filter = GL::GL_LINEAR,
				.data = loaded.data.span_as<byte>(),
			}
		);
		return cubemap;
	}

	std::pair<Name, Description> Parse(File::JSON::JSONObj o, std::filesystem::path const & root_dir)
	{
		return {
			o.FindMember("name")->value.GetString(),
			{
				.path = root_dir / o.FindMember("path")->value.GetString(),
			}
		};
	}
}
