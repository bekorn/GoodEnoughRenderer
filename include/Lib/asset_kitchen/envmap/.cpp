#include "load.hpp"
#include "convert.hpp"

namespace Envmap
{
std::pair<Name, Desc> Parse(File::JSON::JSONObj o, std::filesystem::path const & root_dir)
{
	return {
		o.FindMember("name")->value.GetString(),
		{.path =  root_dir / o.FindMember("path")->value.GetString()}
	};
}

LoadedData Load(Desc const & desc)
{
	auto specular_mip0 = File::LoadImage(desc.path / "specular_mipmap0.hdr", false);
	auto diffuse = File::LoadImage(desc.path / "diffuse.hdr", false);

	LoadedData loaded{
		.diffuse = move(diffuse.buffer),
		.specular_face_dimensions = specular_mip0.dimensions / i32x2(1, 6),
		.diffuse_face_dimensions = diffuse.dimensions / i32x2(1, 6),
	};

	loaded.specular_mipmaps.emplace_back(move(specular_mip0.buffer));

	i32 level = 1;
	std::filesystem::path mip_path;
	while (mip_path = desc.path / fmt::format("specular_mipmap{}.hdr", level), exists(mip_path))
	{
		loaded.specular_mipmaps.emplace_back(File::LoadImage(mip_path, false).buffer);
		++level;
	}

	return loaded;
}

void Convert(LoadedData const & loaded, Name const & name, Managed<GL::TextureCubemap> & cubemaps)
{
	auto & diffuse = cubemaps.generate(name.string + "_diffuse").data;
	diffuse.init(GL::TextureCubemap::ImageDesc{
		.face_dimensions = loaded.diffuse_face_dimensions,
		.has_alpha = false,
		.color_space = GL::COLOR_SPACE::LINEAR_F32,
		.levels = 1,
		.data = loaded.diffuse.span_as<byte>(),
	});

	auto & specular = cubemaps.generate(name.string + "_specular").data;
	specular.init(GL::TextureCubemap::ImageDesc{
		.face_dimensions = loaded.specular_face_dimensions,
		.has_alpha = false,
		.color_space = GL::COLOR_SPACE::LINEAR_F32,
		.levels = static_cast<i32>(loaded.specular_mipmaps.size()),
		.min_filter = GL::GL_LINEAR_MIPMAP_LINEAR,
	});

	for (auto level = 0; level < loaded.specular_mipmaps.size(); ++level)
	{
		i32x2 face_dimensions;
		glGetTextureLevelParameteriv(specular.id, level, GL::GL_TEXTURE_WIDTH, &face_dimensions.x);
		glGetTextureLevelParameteriv(specular.id, level, GL::GL_TEXTURE_HEIGHT, &face_dimensions.y);

		auto aligns_to_4 = (face_dimensions.x * 3) % 4 == 0;
		GL::glPixelStorei(GL::GL_UNPACK_ALIGNMENT, aligns_to_4 ? 4 : 1);

		GL::glTextureSubImage3D(
			specular.id, level,
			0, 0, 0,
			face_dimensions.x, face_dimensions.y, 6,
			GL::GL_RGB, GL::GL_FLOAT, loaded.specular_mipmaps[level].data_as<f32>()
		);
	}
	GL::glPixelStorei(GL::GL_UNPACK_ALIGNMENT, 4);
}
}