#include "editor_windows.hpp"

#include "game.hpp"

#include "Lib/file_management/core.hpp"

void GameSettingsWindow::update(Editor::Context & ctx)
{
	auto & game = static_cast<Game&>(ctx.game);

	using namespace ImGui;

	if (BeginCombo("Environment Map", game.settings.environment_map_name.string.data()))
	{
		for (auto & [name, _]: game.assets.texture_cubemaps)
			if (Selectable(name.string.data()))
				game.settings.environment_map_name = name;

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
		Image(
			reinterpret_cast<void *>(i64(texture.id)),
			{240, 120}
		);

		if (Button("Map Equirectangular Projection to Cubemap"))
			should_map_equirectangular_to_cubemap = true;
	}
}

void IblBakerWindow::render(Editor::Context const & ctx)
{
	if (not should_map_equirectangular_to_cubemap)
		return;

	should_map_equirectangular_to_cubemap = false;

	using namespace GL;

	auto & textures = ctx.game.assets.textures;
	auto & cubemaps = ctx.game.assets.texture_cubemaps;
	if (auto it = textures.find(selected_name); it != textures.end())
	{
		auto & [_, texture] = *it;

		// cubemap face rendering configuration
		f32x4x3 const base_view_dirs = {
			{-1, -1, +1}, // uv 0,0
			{+1, -1, +1}, // uv 1,0
			{-1, +1, +1}, // uv 0,1
			{+1, +1, +1}, // uv 1,1
		};
		f32x3 const dirs[6] = {
			{+1, 0, 0},
			{-1, 0, 0},
			{0, +1, 0},
			{0, -1, 0},
			{0, 0, +1},
			{0, 0, -1},
		};
		f32x3 const ups[6] = {
			{0, +1,  0},
			{0, +1,  0},
			{0,  0, -1},
			{0,  0, +1},
			{0, +1,  0},
			{0, +1,  0},
		};

		// settings
		auto const face_dimensions = i32x2(256);
		auto const texture_size_in_bytes = compMul(face_dimensions)*6 * 1*3; // pixel format = RGB8

		auto const cubemap_description = TextureCubemap::ImageDescription{
			.face_dimensions = face_dimensions,
			.has_alpha = false,
			.is_sRGB = false,
			.levels = 1,
		};

		// temporary framebuffer
		OpenGLObject fb;
		glCreateFramebuffers(1, &fb.id);
		glBindFramebuffer(GL_FRAMEBUFFER, fb.id);
		glViewport(i32x2(0), face_dimensions);
		glDepthMask(false), glDepthFunc(GL_ALWAYS);


		/// Map equirectangular into cubemap
		auto cubemap_name = Name(selected_name.string + "_cubemap");
		auto & cubemap = cubemaps.get_or_generate(cubemap_name);
		if (cubemap.id == 0)
			cubemap.create(cubemap_description);

		auto & equirect_to_cubemap_program = ctx.editor_assets.programs.get("equirectangular_to_cubemap"_name);
		glUseProgram(equirect_to_cubemap_program.id);

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

		{
			ByteBuffer pixels(texture_size_in_bytes);
			glGetTextureImage(
				cubemap.id, 0,
				GL_RGB, GL_UNSIGNED_BYTE,
				pixels.size, pixels.data_as<void>()
			);
			File::WriteImage(
				ctx.game.assets.descriptions.root / "cubemap" / (cubemap_name.string + ".png"),
				File::Image{
					.buffer = move(pixels),
					.dimensions = face_dimensions * i32x2(1, 6),
					.channels = 3,
				}
			);
		}

		glDeleteFramebuffers(1, &fb.id);
	}
}
