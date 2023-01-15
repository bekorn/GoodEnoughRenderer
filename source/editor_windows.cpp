#include "editor_windows.hpp"

#include "game.hpp"

#include "Lib/file_management/core.hpp"

void GameSettingsWindow::update(Editor::Context & ctx)
{
	auto & game = static_cast<Game&>(ctx.game);

	using namespace ImGui;

	if (BeginCombo("Envmap Specular", game.settings.envmap_specular.string.data()))
	{
		for (auto & [name, _]: game.assets.texture_cubemaps)
			if (Selectable(name.string.data()))
				game.settings.envmap_specular = name;

		EndCombo();
	}
	if (BeginCombo("Envmap Diffuse", game.settings.envmap_diffuse.string.data()))
	{
		for (auto & [name, _]: game.assets.texture_cubemaps)
			if (Selectable(name.string.data()))
				game.settings.envmap_diffuse = name;

		EndCombo();
	}

	Checkbox("Zpass", &game.settings.is_zpass_on);
	SameLine(), Checkbox("is env map comp", &game.settings.is_environment_mapping_comp);
	SameLine(), Checkbox("is gamma correction comp", &game.settings.is_gamma_correction_comp);

	static bool is_vsync_on = true;
	if (Checkbox("is vsync on", &is_vsync_on))
		glfwSwapInterval(is_vsync_on ? 1 : 0);
}


void MetricsWindow::update(Editor::Context & ctx)
{
	if constexpr (not GL::Timer::should_query)
		return;

	auto & game = static_cast<Game &>(ctx.game);

	{
		auto time = game.environment_mapping_timer.average_in_nanoseconds;
		Editor::TextFMT("{:30} {:6} us | {:6} ns", "Enrironment Mapping", time / 1'000, time);
	}
	{
		auto time = game.tone_mapping_timer.average_in_nanoseconds;
		Editor::TextFMT("{:30} {:6} us | {:6} ns", "Tone Mapping", time / 1'000, time);
	}
	{
		auto time = game.gamma_correction_timer.average_in_nanoseconds;
		Editor::TextFMT("{:30} {:6} us | {:6} ns", "Gamma Correction", time / 1'000, time);
	}
}


void MaterialWindow::update(Editor::Context & ctx)
{
	auto & game = static_cast<Game&>(ctx.game);

	using namespace ImGui;

	static Name material_name;
	if (BeginCombo("Material", material_name.string.data()))
	{
		for (auto const & [name, _]: game.assets.materials)
			if (Selectable(name.string.data()))
				material_name = name;

		EndCombo();
	}
	if (not game.assets.materials.contains(material_name))
	{
		Text("Pick a material");
		return;
	}
	auto & material = game.assets.materials.get(material_name);
	auto & block = material->get_block();

	auto buffer = ByteBuffer(block.data_size);
	material->write_to_buffer(buffer.begin());

	bool edited = false;
	for (auto & [name, variable]: block.variables)
		switch (variable.glsl_type)
		{
		case GL::GL_FLOAT:
		{
			edited |= DragFloat(name.data(), buffer.data_as<f32>(variable.offset));
			break;
		}
		case GL::GL_FLOAT_VEC2:
		{
			edited |= DragFloat2(name.data(), buffer.data_as<f32>(variable.offset));
			break;
		}
		case GL::GL_FLOAT_VEC3:
		{
			edited |= ColorEdit3(name.data(), buffer.data_as<f32>(variable.offset));
			break;
		}
		case GL::GL_FLOAT_VEC4:
		{
			edited |= ColorEdit4(name.data(), buffer.data_as<f32>(variable.offset));
			break;
		}
		case GL::GL_UNSIGNED_INT64_ARB:
		{
			// TODO(bekorn): display the texture
			// TODO(bekorn): should be editable
			LabelText(name.data(), "%llu", *buffer.data_as<u64>(variable.offset));
			break;
		}
		default:
		{ LabelText(name.data(), "%s is not supported", GL::GLSLTypeToString(variable.glsl_type).data()); }
		}

	if (edited)
	{
		material->read_from_buffer(buffer.begin());
		game.gltf_material_is_dirty.push(material_name); // !!! Temporary
	}
}


void IblBakerWindow::update(Editor::Context & ctx)
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

		if (Button("Generate IBL Environment"))
			should_generate_environment = true;
	}
	else
	{
		TextUnformatted("Select a Texture");
	}
}

void IblBakerWindow::render(Editor::Context const & ctx)
{
	if (should_generate_brdf_lut)
	{
		_generate_brdf_lut(ctx);
		should_generate_brdf_lut = false;
	}

	if (should_generate_environment)
	{
		_generate_environment(ctx);
		should_generate_environment = false;
	}
}

void IblBakerWindow::_generate_brdf_lut(Editor::Context const & ctx) const
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
	auto texture_name = Name("IBL_BRDF_LUT");
	auto & texture = textures.get_or_generate(texture_name);
	if (texture.id == 0)
		texture.create(Texture2D::ImageDescription{
			.dimensions = texture_dimensions,
			.has_alpha = false,
			.color_space = GL::COLOR_SPACE::LINEAR_FLOAT,
		});

	auto & program = ctx.editor_assets.programs.get("envmap_brdf_lut"_name);
	glUseProgram(program.id);


	/// Generate texture
	glNamedFramebufferTexture(fb.id, GL_COLOR_ATTACHMENT0, texture.id, 0);
	glViewport({0, 0}, texture_dimensions);

	glDrawArrays(GL_TRIANGLES, 0, 3);


	/// Save texture
	auto asset_dir = ctx.game.assets.descriptions.root / "envmap";
	std::filesystem::create_directories(asset_dir);

	ByteBuffer pixels(compMul(texture_dimensions) * 4 * 3); // pixel format = RGB16F (but uses f32)
	glGetTextureImage(texture.id, 0, GL_RGB, GL_FLOAT, pixels.size, pixels.data_as<void>());
	File::WriteImage(
		asset_dir / "envmap_brdf_lut.hdr",
		File::Image{
			.buffer = move(pixels),
			.dimensions = texture_dimensions,
			.channels = 3,
			.is_format_f32 = true,
		}
	);


	/// Clean up
	glDeleteFramebuffers(1, &fb.id);
}

void IblBakerWindow::_generate_environment(Editor::Context const & ctx) const
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
			cubemap.create(TextureCubemap::ImageDescription{
				.face_dimensions = cubemap_face_dimensions,
				.has_alpha = false,
				.color_space = GL::COLOR_SPACE::LINEAR_FLOAT,
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


		/// Generate envmap diffuse
		auto const di_face_dimensions = i32x2(32);
		auto di_name = Name(selected_name.string + "_diffuse");
		auto & di = cubemaps.get_or_generate(di_name);
		if (di.id == 0)
			di.create(TextureCubemap::ImageDescription{
				.face_dimensions = di_face_dimensions,
				.has_alpha = false,
				.color_space = GL::COLOR_SPACE::LINEAR_FLOAT,
				.levels = 1,
			});

		auto & di_program = ctx.editor_assets.programs.get("envmap_diffuse"_name);
		glUseProgram(di_program.id);

		glViewport(i32x2(0), di_face_dimensions);

		glUniformHandleui64ARB(
			GetLocation(di_program.uniform_mappings, "environment"),
			cubemap.handle
		);

		for (auto face = 0; face < 6; ++face)
		{
			glNamedFramebufferTextureLayer(fb.id, GL_COLOR_ATTACHMENT0, di.id, 0, face);

			auto view_dirs = inverse(f32x3x3(lookAt(f32x3(0), dirs[face], ups[face]))) * base_view_dirs;
			glUniformMatrix4x3fv(
				GetLocation(di_program.uniform_mappings, "view_dirs"),
				1, false, begin(view_dirs)
			);

			glDrawArrays(GL_TRIANGLES, 0, 3);
		}


		/// Generate envmap specular
		auto const si_face_dimensions = i32x2(1024);
		auto si_name = Name(selected_name.string + "_specular");
		auto & si = cubemaps.get_or_generate(si_name);
		if (si.id == 0)
			si.create(TextureCubemap::ImageDescription{
				.face_dimensions = si_face_dimensions,
				.has_alpha = false,
				.color_space = GL::COLOR_SPACE::LINEAR_FLOAT,
				.levels = 7, // smallest mip is 16x16
				.min_filter = GL_LINEAR_MIPMAP_LINEAR,
			});

		auto & si_program = ctx.editor_assets.programs.get("envmap_specular"_name);
		glUseProgram(si_program.id);

		glUniformHandleui64ARB(
			GetLocation(si_program.uniform_mappings, "environment"),
			cubemap.handle
		);

		i32 levels;
		glGetTextureParameteriv(si.id, GL_TEXTURE_IMMUTABLE_LEVELS, &levels);

		for (auto level = 0; level < levels; ++level)
		{
			i32x2 level_dimensions;
			glGetTextureLevelParameteriv(si.id, level, GL_TEXTURE_WIDTH, &level_dimensions.x);
			glGetTextureLevelParameteriv(si.id, level, GL_TEXTURE_HEIGHT, &level_dimensions.y);

			glViewport(i32x2(0), level_dimensions);

			glUniform1f(
				GetLocation(si_program.uniform_mappings, "roughness"),
				f32(level) / f32(levels - 1)
			);

			for (auto face = 0; face < 6; ++face)
			{
				glNamedFramebufferTextureLayer(fb.id, GL_COLOR_ATTACHMENT0, si.id, level, face);

				auto view_dirs = inverse(f32x3x3(lookAt(f32x3(0), dirs[face], ups[face]))) * base_view_dirs;
				glUniformMatrix4x3fv(
					GetLocation(si_program.uniform_mappings, "view_dirs"),
					1, false, begin(view_dirs)
				);

				glDrawArrays(GL_TRIANGLES, 0, 3);
			}
		}


		/// Save textures as an Envmap asset
		auto asset_dir = ctx.game.assets.descriptions.root / "envmap" / selected_name.string;
		std::filesystem::create_directories(asset_dir);

		{
			ByteBuffer pixels(compMul(di_face_dimensions) * 6 * 4 * 3); // pixel format = RGB16F (but uses f32)
			glGetTextureImage(di.id, 0, GL_RGB, GL_FLOAT, pixels.size, pixels.data_as<void>());
			File::WriteImage(
				asset_dir / "diffuse.hdr",
				File::Image{
					.buffer = move(pixels),
					.dimensions = di_face_dimensions * i32x2(1, 6),
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
				glGetTextureLevelParameteriv(si.id, level, GL_TEXTURE_WIDTH, &face_dimensions.x);
				glGetTextureLevelParameteriv(si.id, level, GL_TEXTURE_HEIGHT, &face_dimensions.y);

				auto aligns_to_4 = (face_dimensions.x * 3) % 4 == 0;
				glPixelStorei(GL_PACK_ALIGNMENT, aligns_to_4 ? 4 : 1);

				ByteBuffer pixels(compMul(face_dimensions) * 6 * 4 * 3); // pixel format = RGB16F (but uses f32)
				glGetTextureImage(si.id, level, GL_RGB, GL_FLOAT, pixels.size, pixels.data_as<void>());
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
