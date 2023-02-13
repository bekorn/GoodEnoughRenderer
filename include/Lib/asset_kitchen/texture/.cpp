#include "load.hpp"
#include "convert.hpp"

#include "Lib/file_management/core.hpp"

namespace Texture
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
	assert(("Cubemap filter is unknown", false));
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
	// regular textures (first-pixel == uv(0,1)) require a vertical flip
	auto image_file = File::LoadImage(desc.path, true);

	return {
		.data = move(image_file.buffer),
		.dimensions = image_file.dimensions,
		.channels = image_file.channels,
		.color_space = image_file.is_format_f32 ? GL::COLOR_SPACE::LINEAR_F32 : GL::COLOR_SPACE::LINEAR_U8,
		.levels = desc.levels,
		.min_filter = desc.min_filter,
		.mag_filter = desc.mag_filter,
	};
}

GL::Texture2D Convert(LoadedData const & loaded)
{
	GL::Texture2D texture;
	texture.init(
		GL::Texture2D::ImageDesc{
			.dimensions = loaded.dimensions,
			.has_alpha = loaded.channels == 4,
			.color_space = loaded.color_space,
			.levels = loaded.levels,
			.min_filter = loaded.min_filter,
			.mag_filter = loaded.mag_filter,
			.data = loaded.data.span_as<byte>(),
		}
	);
	return texture;
}
}
