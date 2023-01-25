#include "envmap_baker.hpp"

#include "utils.hpp"

#include "Lib/file_management/core.hpp"

namespace Editor
{
void EnvmapBakerWindow::update(Editor::Context & ctx)
{
	using namespace ImGui;

	auto & textures = ctx.game.assets.textures;

	if (Button("Generate BRDF LUT"))
		should_generate_brdf_lut = true;

	Separator();

	if (BeginCombo("Texture", selected_name.string.data()))
	{
		for (auto & [name, _]: textures)
			if (Selectable(name.string.data()))
				selected_name = name, is_texture_changed = true;

		EndCombo();
	}
	if (auto it = textures.find(selected_name); it != textures.end())
	{
		auto & [_, texture] = *it;
		SameLine(), Text("(id %d)", texture.id);

		if (is_texture_changed)
		{
			is_texture_changed = false;

			GL::glGetTextureLevelParameterfv(texture.id, 0, GL::GL_TEXTURE_WIDTH, &texture_size.x);
			GL::glGetTextureLevelParameterfv(texture.id, 0, GL::GL_TEXTURE_HEIGHT, &texture_size.y);
		}

		LabelText("Resolution", "%d x %d", i32(texture_size.x), i32(texture_size.y));
		Editor::ImageGL(
			reinterpret_cast<void *>(i64(texture.id)),
			{240, 120}
		);

		if (Button("Generate Envmap"))
			should_generate_envmap = true;
	}
	else
	{
		TextUnformatted("Select a Texture");
	}
}

void EnvmapBakerWindow::render(Editor::Context const & ctx)
{
	if (should_generate_brdf_lut)
	{
		generate_brdf_lut(ctx);
		should_generate_brdf_lut = false;
	}

	if (should_generate_envmap)
	{
		generate_envmap(ctx);
		should_generate_envmap = false;
	}
}

void EnvmapBakerWindow::generate_brdf_lut(Editor::Context const & ctx) const
{
	using namespace GL;

	auto & textures = ctx.game.assets.textures;

	// temporary framebuffer
	OpenGLObject fb;
	glCreateFramebuffers(1, &fb.id);
	glBindFramebuffer(GL_FRAMEBUFFER, fb.id);
	glDepthMask(false), glDepthFunc(GL_ALWAYS);

	// brdf lut texture
	auto const texture_dimensions = i32x2(256);
	auto texture_name = "envmap_brdf_lut"_name;
	auto & texture = textures.get_or_generate(texture_name);
	if (texture.id == 0)
		texture.init(Texture2D::ImageDesc{
			.dimensions = texture_dimensions,
			.has_alpha = false,
			.color_space = GL::COLOR_SPACE::LINEAR_F32,
		});

	auto & program = ctx.editor_assets.programs.get("envmap_brdf_lut"_name);
	glUseProgram(program.id);


	/// Generate brdf lut
	glNamedFramebufferTexture(fb.id, GL_COLOR_ATTACHMENT0, texture.id, 0);
	glViewport({0, 0}, texture_dimensions);

	glDrawArrays(GL_TRIANGLES, 0, 3);


	/// Save texture
	auto asset_dir = ctx.game.assets.descriptions.root / "envmap";
	std::filesystem::create_directories(asset_dir);

	ByteBuffer pixels(compMul(texture_dimensions) * 3*4); // pixel format = RGB16F (but uses f32)
	glGetTextureImage(texture.id, 0, GL_RGB, GL_FLOAT, pixels.size, pixels.data_as<void>());
	File::WriteImage(
		asset_dir / "brdf_lut.hdr",
		File::Image{
			.buffer = move(pixels),
			.dimensions = texture_dimensions,
			.channels = 3,
			.is_format_f32 = true,
		},
		true
	);


	/// Clean up
	glDeleteFramebuffers(1, &fb.id);
}

void EnvmapBakerWindow::generate_envmap(Editor::Context const & ctx) const
{
	using namespace GL;

	auto & textures = ctx.game.assets.textures;
	auto & cubemaps = ctx.game.assets.texture_cubemaps;
	if (auto it = textures.find(selected_name); it != textures.end())
	{
		auto & [_, texture] = *it;

		// cubemap face rendering configuration
		f32x4x3 const base_view_dirs{
			{-1, -1, -1}, // uv 0,0
			{+1, -1, -1}, // uv 1,0
			{-1, +1, -1}, // uv 0,1
			{+1, +1, -1}, // uv 1,1
		};
		f32x3 const dirs[6]{
			{+1,  0,  0},
			{-1,  0,  0},
			{ 0, +1,  0},
			{ 0, -1,  0},
			{ 0,  0, +1},
			{ 0,  0, -1},
		};
		f32x3 const ups[6]{
			{0, -1,  0},
			{0, -1,  0},
			{0,  0, +1},
			{0,  0, -1},
			{0, -1,  0},
			{0, -1,  0},
		};

		// temporary framebuffer
		OpenGLObject fb;
		glCreateFramebuffers(1, &fb.id);
		glBindFramebuffer(GL_FRAMEBUFFER, fb.id);
		glDepthMask(false), glDepthFunc(GL_ALWAYS);


		/// Map equirectangular into cubemap
		auto const cubemap_face_dimensions = i32x2(1024);
		auto cubemap_name = Name(selected_name.string + "_cubemap");
		auto & cubemap = cubemaps.get_or_generate(cubemap_name);
		if (cubemap.id == 0)
			cubemap.init(TextureCubemap::ImageDesc{
				.face_dimensions = cubemap_face_dimensions,
				.has_alpha = false,
				.color_space = GL::COLOR_SPACE::LINEAR_F32,
				.levels = 0,
				.min_filter = GL_LINEAR_MIPMAP_LINEAR,
			});

		auto & equirect_to_cubemap_program = ctx.editor_assets.programs.get("equirectangular_to_cubemap"_name);
		glUseProgram(equirect_to_cubemap_program.id);

		glViewport(i32x2(0), cubemap_face_dimensions);

		glUniformHandleui64ARB(
			GetLocation(equirect_to_cubemap_program.uniform_mappings, "equirect"),
			texture.handle
		);

		for (auto face = 0; face < 6; ++face)
		{
			glNamedFramebufferTextureLayer(fb.id, GL_COLOR_ATTACHMENT0, cubemap.id, 0, face);

			auto view_dirs = inverse(f32x3x3(lookAt(f32x3(0), dirs[face], ups[face]))) * base_view_dirs;
			glUniformMatrix4x3fv(
				GetLocation(equirect_to_cubemap_program.uniform_mappings, "view_dirs"),
				1, false, begin(view_dirs)
			);

			glDrawArrays(GL_TRIANGLES, 0, 3);
		}

		glGenerateTextureMipmap(cubemap.id);


		/// Generate diffuse envmap
		auto const d_face_dimensions = i32x2(32);
		auto d_name = Name(selected_name.string + "_diffuse");
		auto & d_envmap = cubemaps.get_or_generate(d_name);
		if (d_envmap.id == 0)
			d_envmap.init(TextureCubemap::ImageDesc{
				.face_dimensions = d_face_dimensions,
				.has_alpha = false,
				.color_space = GL::COLOR_SPACE::LINEAR_F32,
				.levels = 1,
			});

		auto & d_program = ctx.editor_assets.programs.get("envmap_diffuse"_name);
		glUseProgram(d_program.id);

		glViewport(i32x2(0), d_face_dimensions);

		glUniformHandleui64ARB(
			GetLocation(d_program.uniform_mappings, "environment"),
			cubemap.handle
		);

		for (auto face = 0; face < 6; ++face)
		{
			glNamedFramebufferTextureLayer(fb.id, GL_COLOR_ATTACHMENT0, d_envmap.id, 0, face);

			auto view_dirs = inverse(f32x3x3(lookAt(f32x3(0), dirs[face], ups[face]))) * base_view_dirs;
			glUniformMatrix4x3fv(
				GetLocation(d_program.uniform_mappings, "view_dirs"),
				1, false, begin(view_dirs)
			);

			glDrawArrays(GL_TRIANGLES, 0, 3);
		}


		/// Generate specular envmap
		auto const s_face_dimensions = i32x2(1024);
		auto s_name = Name(selected_name.string + "_specular");
		auto & s_envmap = cubemaps.get_or_generate(s_name);
		if (s_envmap.id == 0)
			s_envmap.init(TextureCubemap::ImageDesc{
				.face_dimensions = s_face_dimensions,
				.has_alpha = false,
				.color_space = GL::COLOR_SPACE::LINEAR_F32,
				.levels = 7, // smallest mip is 16x16 to keep cubemap meaningful
				.min_filter = GL_LINEAR_MIPMAP_LINEAR,
			});

		auto & s_program = ctx.editor_assets.programs.get("envmap_specular"_name);
		glUseProgram(s_program.id);

		glUniformHandleui64ARB(
			GetLocation(s_program.uniform_mappings, "environment"),
			cubemap.handle
		);

		i32 levels;
		glGetTextureParameteriv(s_envmap.id, GL_TEXTURE_IMMUTABLE_LEVELS, &levels);

		for (auto level = 0; level < levels; ++level)
		{
			i32x2 level_dimensions;
			glGetTextureLevelParameteriv(s_envmap.id, level, GL_TEXTURE_WIDTH, &level_dimensions.x);
			glGetTextureLevelParameteriv(s_envmap.id, level, GL_TEXTURE_HEIGHT, &level_dimensions.y);

			glViewport(i32x2(0), level_dimensions);

			glUniform1f(
				GetLocation(s_program.uniform_mappings, "roughness"),
				f32(level) / f32(levels - 1)
			);

			for (auto face = 0; face < 6; ++face)
			{
				glNamedFramebufferTextureLayer(fb.id, GL_COLOR_ATTACHMENT0, s_envmap.id, level, face);

				auto view_dirs = inverse(f32x3x3(lookAt(f32x3(0), dirs[face], ups[face]))) * base_view_dirs;
				glUniformMatrix4x3fv(
					GetLocation(s_program.uniform_mappings, "view_dirs"),
					1, false, begin(view_dirs)
				);

				glDrawArrays(GL_TRIANGLES, 0, 3);
			}
		}


		/// Save textures
		auto asset_dir = ctx.game.assets.descriptions.root / "envmap" / selected_name.string;

		if (std::filesystem::exists(asset_dir))
		{
			if (auto ec = File::ClearFolder(asset_dir))
			{
				fmt::print(stderr, "Failed to clear directory {}. Error: {}", asset_dir, ec.value().message());
				return;
			}
		}
		else
		{
			std::filesystem::create_directories(asset_dir);
		}

		{
			ByteBuffer pixels(compMul(d_face_dimensions)*6 * 3*4); // pixel format = RGB16F (but uses f32)
			glGetTextureImage(d_envmap.id, 0, GL_RGB, GL_FLOAT, pixels.size, pixels.data_as<void>());
			File::WriteImage(
				asset_dir / "diffuse.hdr",
				File::Image{
					.buffer = move(pixels),
					.dimensions = d_face_dimensions * i32x2(1, 6),
					.channels = 3,
					.is_format_f32 = true,
				},
				false
			);
		}
		{
			i32 previous_pack_alignment;
			glGetIntegerv(GL_PACK_ALIGNMENT, &previous_pack_alignment);

			for (auto level = 0; level < levels; ++level)
			{
				i32x2 face_dimensions;
				glGetTextureLevelParameteriv(s_envmap.id, level, GL_TEXTURE_WIDTH, &face_dimensions.x);
				glGetTextureLevelParameteriv(s_envmap.id, level, GL_TEXTURE_HEIGHT, &face_dimensions.y);

				auto aligns_to_4 = (face_dimensions.x * 3) % 4 == 0;
				glPixelStorei(GL_PACK_ALIGNMENT, aligns_to_4 ? 4 : 1);

				ByteBuffer pixels(compMul(face_dimensions)*6 * 3*4); // pixel format = RGB16F (but uses f32)
				glGetTextureImage(s_envmap.id, level, GL_RGB, GL_FLOAT, pixels.size, pixels.data_as<void>());
				File::WriteImage(
					asset_dir / fmt::format("specular_mipmap{}.hdr", level),
					File::Image{
						.buffer = move(pixels),
						.dimensions = face_dimensions * i32x2(1, 6),
						.channels = 3,
						.is_format_f32 = true,
					},
					false
				);
			}

			glPixelStorei(GL_PACK_ALIGNMENT, previous_pack_alignment);
		}


		/// Clean up
		glDeleteFramebuffers(1, &fb.id);
	}
}
}