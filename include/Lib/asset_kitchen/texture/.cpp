#include "load.hpp"
#include "convert.hpp"

#include "Lib/file_management/core.hpp"

namespace Texture
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
		auto dimensions = image_file.dimensions;

		return LoadedData{
			.data = move(image_file.buffer),
			.dimensions = dimensions,
			.channels = image_file.channels,
			.is_sRGB = false,
			.levels = description.levels,
			.min_filter = description.min_filter,
			.mag_filter = description.mag_filter,
		};
	}

	GL::Texture2D Convert(LoadedData const & loaded)
	{
		GL::Texture2D texture;
		texture.create(
			GL::Texture2D::ImageDescription{
				.dimensions = loaded.dimensions,
				.has_alpha = loaded.channels == 4,
				.is_sRGB = loaded.is_sRGB,
				.levels = loaded.levels,
				.min_filter = loaded.min_filter,
				.mag_filter = loaded.mag_filter,
				.data = loaded.data.span_as<byte>(),
			}
		);
		return texture;
	}
}