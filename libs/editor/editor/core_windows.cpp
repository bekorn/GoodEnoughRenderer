#include "core_windows.hpp"

#include <file_io/core.hpp>
#include <opengl/globals.hpp>

namespace Editor
{
void GameWindow::init(Context const & ctx)
{
	framebuffer.init(GL::FrameBuffer::Desc{
		.resolution = ctx.game.framebuffer.resolution,
		.attachments = {
			{
				.type = &GL::FrameBuffer::color0,
				.desc = GL::Texture2D::AttachmentDesc{
					.internal_format = GL::GL_RGBA16F,
					.mag_filter = GL::GL_NEAREST,
				},
			},
			{
				.type = &GL::FrameBuffer::depth,
				.desc = GL::Texture2D::AttachmentDesc{
					.internal_format = GL::GL_DEPTH_COMPONENT16,
				},
			},
		}
	});

	// border
	border.init(ctx, *this);
}

void GameWindow::update(Context & ctx)
{
	using namespace ImGui;

	BeginDisabled(ctx.state.has_program_errors);

	Checkbox("Render", &ctx.state.should_game_render);
	if (render_single_frame)
		render_single_frame = ctx.state.should_game_render = false;
	if (render_single_frame = (SameLine(), Button("Render Single Frame")))
		ctx.state.should_game_render = true;

	EndDisabled();

	if (SameLine(), Button("Save Frame"))
		should_save_frame = true;

	{
		Text("Resolution: %dx%d", ctx.game.framebuffer.resolution.x, ctx.game.framebuffer.resolution.y);
		SameLine(), SliderFloat("Scale", &scale, 0.1, 10.0, "%.1f");

		auto resolution = ImVec2(f32(ctx.game.framebuffer.resolution.x) * scale, f32(ctx.game.framebuffer.resolution.y) * scale);

		auto border_color = ImVec4{0, 0, 0, 1};
		if (ctx.state.has_program_errors)
			border_color.x = 1;

		auto game_pos = GetCursorPos();
		ImageGL(
			reinterpret_cast<void *>(i64(ctx.game.framebuffer.color0.id)),
			resolution
		);
		auto editor_pos = ImVec2(game_pos.x - 1, game_pos.y - 1); // because of image borders
		SetCursorPos(editor_pos), ImageGL(
			reinterpret_cast<void *>(i64(framebuffer.color0.id)),
			resolution, {0, 1}, {1, 0}, {1, 1, 1, 1}, border_color
		);

		SameLine(), ImageGL(
			reinterpret_cast<void *>(i64(ctx.game.framebuffer.depth.id)),
			resolution
		);
	}
}

void GameWindow::render(Context const & ctx)
{
	using namespace GL;

	if (should_save_frame)
	{
		should_save_frame = false;

		auto const & game_fb = ctx.game.framebuffer;

		auto border_width = 3;
		auto capture_resolution = game_fb.resolution + 2*border_width;

		Texture2D frame;
		frame.init(Texture2D::AttachmentDesc{
			.dimensions = capture_resolution,
			.internal_format = GL_RGB8,
		});
		auto clear_color = u8x3(0); // alternative orange+red = u8x3(242, 48, 27);
		glClearTexImage(frame.id, 0, GL_RGB, GL_BYTE, begin(clear_color));

		glNamedFramebufferReadBuffer(game_fb.id, GL_COLOR_ATTACHMENT0);
		glCopyTextureSubImage2D(
			frame.id, 0,
			border_width, border_width,
			0, 0, game_fb.resolution.x, game_fb.resolution.y
		);

		ByteBuffer pixels(compMul(capture_resolution) * 3*1); // pixel format = RGB8

		auto aligns_to_4 = (capture_resolution.x * 3) % 4 == 0;
		glPixelStorei(GL_PACK_ALIGNMENT, aligns_to_4 ? 4 : 1);
		glGetTextureImage(frame.id, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels.size, pixels.data_as<void>());

		auto asset_dir = ctx.game.assets.descriptions.root / "capture";
		if (not std::filesystem::exists(asset_dir))
			std::filesystem::create_directories(asset_dir);

		auto now = std::time(nullptr);
		File::WriteImage(
			asset_dir / fmt::format("{:%Y_%m_%d %H_%M_%S}.png", *std::localtime(&now)),
			File::Image{
				.buffer = move(pixels),
				.dimensions = capture_resolution,
				.channels = 3,
				.is_format_f32 = false,
			},
			true
		);
	}

	// if game is not rendering, editor should not as well
	if (not ctx.state.should_game_render)
		return;

	// Clear framebuffer
	{
		glViewport(i32x2(0), framebuffer.resolution);
		glColorMask(true, true, true, true), glDepthMask(true);
		const f32 clear_depth{1};
		glClearNamedFramebufferfv(framebuffer.id, GL_DEPTH, 0, &clear_depth);
		const f32x4 clear_color{0, 0, 0, 0};
		glClearNamedFramebufferfv(framebuffer.id, GL_COLOR, 0, begin(clear_color));
	}

	// Draw the border of the selected node
	border.render(ctx, *this);

	// Draw gizmos
	{
		glEnable(GL_CULL_FACE), glCullFace(GL_BACK);
		glEnable(GL_DEPTH_TEST), glDepthFunc(GL_LESS);
		glColorMask(true, true, true, true), glDepthMask(true);

		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.id);

		auto & gizmo_program = ctx.editor_assets.programs.get("gizmo"_name);
		glUseProgram(gizmo_program.id);

		i32 size = glm::max(50.f, 0.1f * compMin(framebuffer.resolution));
		auto const padding = 8;
		glViewport(framebuffer.resolution - (size + padding), i32x2(size));

		auto view = visit([](Render::Camera auto & c) { return c.get_view_without_translate(); }, ctx.game.camera);
		auto proj = glm::ortho<f32>(-1, +1, -1, +1, -1, +1);
		auto transform = proj * view;
		glUniformMatrix4fv(
			GetLocation(gizmo_program.uniform_mappings, "transform"),
			1, false, begin(transform)
		);

		for (auto & drawable: ctx.editor_assets.meshes.get("AxisGizmo:mesh:0:Cube"_name).drawables)
		{
			glBindVertexArray(drawable.vertex_array.id);
			glDrawElements(GL_TRIANGLES, drawable.vertex_array.element_count, GL_UNSIGNED_INT, nullptr);
		}
	}

	// gamma correction
	{
		glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);

		glViewport(i32x2(0), framebuffer.resolution);
		glDepthMask(false), glDepthFunc(GL_ALWAYS);

		// TODO(bekorn): move this into the core project (a new project besides game and editor)
		auto & gamma_correction_program = ctx.game.assets.programs.get("gamma_correction"_name);
		glUseProgram(gamma_correction_program.id);
		glUniformHandleui64ARB(
			GetLocation(gamma_correction_program.uniform_mappings, "color_attachment"),
			framebuffer.color0.handle
		);
		glBindVertexArray(GL::dummy_vao.id);
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}
}

void GameWindow::Border::init(Context const & ctx, GameWindow const & game_window)
{
	for (auto & framebuffer: framebuffers)
		framebuffer.init(GL::FrameBuffer::Desc{
			.resolution = game_window.framebuffer.resolution,
			.attachments = {
				{
					.type = &GL::FrameBuffer::color0,
					.desc = GL::Texture2D::AttachmentDesc{
						.internal_format = GL::GL_RG16UI,
					},
				}
			}
		});
}

void GameWindow::Border::render(Context const & ctx, GameWindow const & game_window)
{
	auto & scene_tree = ctx.game.assets.scene_tree;
	auto & selected_name = ctx.state.selected_node_name;

	auto it = scene_tree.named_indices.find(selected_name);
	if (it == scene_tree.named_indices.end())
		return;

	auto & [_, node_index] = *it;
	auto const & node = scene_tree.get(node_index);
	if (node.mesh == nullptr)
		return;

	using namespace GL;

	glViewport(i32x2(0), framebuffers[0].resolution);

	glDisable(GL_DEPTH_TEST);
	glColorMask(true, true, true, true), glDepthMask(false);

	auto const clear_color = i32x2(-1);
	glClearNamedFramebufferiv(framebuffers[0].id, GL_COLOR, 0, begin(clear_color));
	glClearNamedFramebufferiv(framebuffers[1].id, GL_COLOR, 0, begin(clear_color));

	glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[0].id);

	// initialize
	glEnable(GL_SCISSOR_TEST), glScissor(i32x2(border_width), framebuffers[0].resolution - i32(2 * border_width));

	auto & jump_flood_init_program = ctx.editor_assets.programs.get("jump_flood_init");
	glUseProgram(jump_flood_init_program.id);
	auto view = visit([](Render::Camera auto & c){ return c.get_view(); }, ctx.game.camera);
	auto proj = visit([](Render::Camera auto & c){ return c.get_projection(); }, ctx.game.camera);
	auto transform = proj * view * node.matrix;
	glUniformMatrix4fv(
		GetLocation(jump_flood_init_program.uniform_mappings, "transform"),
		1, false, begin(transform)
	);
	for (auto & drawable: node.mesh->drawables)
	{
		glBindVertexArray(drawable.vertex_array.id);
		glDrawElements(GL_TRIANGLES, drawable.vertex_array.element_count, GL_UNSIGNED_INT, nullptr);
	}

	glDisable(GL_SCISSOR_TEST);

	// jump flood
	auto & jump_flood_program = ctx.editor_assets.programs.get("jump_flood");
	glUseProgram(jump_flood_program.id);

	i32 step = glm::compMax(framebuffers[0].resolution) / 2;
	bool pingpong_destination = 1;

	while (step != 0)
	{
		glBindFramebuffer(GL_FRAMEBUFFER , framebuffers[pingpong_destination].id);
		glUniformHandleui64ARB(
			GetLocation(jump_flood_program.uniform_mappings, "positions"),
			framebuffers[not pingpong_destination].color0.handle
		);
		glUniform1i(
			GetLocation(jump_flood_program.uniform_mappings, "step"),
			step
		);
		glBindVertexArray(GL::dummy_vao.id);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		step /= 2;
		pingpong_destination = not pingpong_destination;
	}

	// finalize border
	glViewport(i32x2(0), game_window.framebuffer.resolution);
	auto & border_program = ctx.editor_assets.programs.get("finalize_border");
	glUseProgram(border_program.id);
	glBindFramebuffer(GL_FRAMEBUFFER, game_window.framebuffer.id);
	glUniformHandleui64ARB(
		GetLocation(border_program.uniform_mappings, "positions"),
		framebuffers[not pingpong_destination].color0.handle
	);
	glUniformHandleui64ARB(
		GetLocation(border_program.uniform_mappings, "border_map"),
		ctx.editor_assets.textures.get("border_map"_name).handle
	);
	glUniform1f(
		GetLocation(border_program.uniform_mappings, "border_width"),
		border_width
	);
	glBindVertexArray(GL::dummy_vao.id);
	glDrawArrays(GL_TRIANGLES, 0, 3);
}

void MetricsWindow::update(Context & ctx)
{
	using namespace ImGui;

	{
		auto average = average_frame.add(ctx.state.frame_info.idx, ctx.state.frame_info.seconds_since_last_frame);
		TextFMT("Average frame:  {:>6.2f} ms, {:>4.0f} fps", average * 1000., 1. / average);
	}

	{
		auto average = average_game_update.add(ctx.state.frame_info.idx, ctx.state.game_update_in_seconds);
		TextFMT("Average update: {:>6.2f} ms", average * 1000., 1. / average);
	}

	{
		auto average = average_game_render.add(ctx.state.frame_info.idx, ctx.state.game_render_in_seconds);
		SameLine(), TextFMT(",  render: {:>6.2f} ms", average * 1000., 1. / average);
	}

	TextFMT("Frame: {:6}, Time: {:7.2f}", ctx.state.frame_info.idx, ctx.state.frame_info.seconds_since_start);
}

void AttribLayoutWindow::update(Context & ctx)
{
	using namespace ImGui;

	auto & attrib_layouts = ctx.game.assets.attrib_layouts;
	auto & selected_name = ctx.state.selected_attrib_layout_name;

	if (BeginCombo("Attrib Layout", selected_name.string.data()))
	{
		for (auto const & [name, _]: attrib_layouts)
			if (Selectable(name.string.data()))
				selected_name = name;

		EndCombo();
	}
	if (auto it = attrib_layouts.find(selected_name); it == attrib_layouts.end())
	{
		Text("Pick an attrib layout");
		return;
	}
	else
	{
		auto & [name, attrib_layout] = *it;

		if (BeginTable("Attribs", 4, ImGuiTableFlags_BordersInnerH))
		{
			TableSetupColumn("Key");
			TableSetupColumn("Vec");
			TableSetupColumn("Location", ImGuiTableColumnFlags_WidthFixed, 60);
			TableSetupColumn("Group", ImGuiTableColumnFlags_WidthFixed, 40);
			TableHeadersRow();

			for (auto const & attrib: attrib_layout.attributes)
			{
				if (not attrib.is_used()) continue;
				TableNextColumn(), TextFMT("{}", attrib.key);
				TableNextColumn(), TextFMT("{}", attrib.vec);
				TableNextColumn(), TextFMT("{}", attrib.location);
				TableNextColumn(), TextFMT("{}", attrib.group);
			}
			EndTable();
		}
	}
}

void UniformBufferWindow::update(Context & ctx)
{
	using namespace ImGui;

	auto & uniform_blocks = ctx.game.assets.uniform_blocks;
	auto & selected_name = ctx.state.selected_uniform_buffer_name;

	if (BeginCombo("Uniform Buffer", selected_name.string.data()))
	{
		for (auto const & [name, _]: uniform_blocks)
			if (Selectable(name.string.data()))
				selected_name = name;

		EndCombo();
	}
	if (auto it = uniform_blocks.find(selected_name); it == uniform_blocks.end())
	{
		Text("Pick a uniform buffer");
		return;
	}
	else
	{
		auto & [_, uniform_buffer] = *it;

		LabelText("Name", "%s", uniform_buffer.key.data());
		LabelText("Binding", "%d", uniform_buffer.binding);
		LabelText("Size", "%d", uniform_buffer.data_size);

		if (BeginTable("Variables", 3, ImGuiTableFlags_BordersInnerH))
		{
			TableSetupColumn("GLSL Name");
			TableSetupColumn("Offset", ImGuiTableColumnFlags_WidthFixed);
			TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 100);
			TableHeadersRow();

			for (auto const & [key, variable]: uniform_buffer.variables)
			{
				TableNextColumn(), TextFMT("{}", key);
				TableNextColumn(), TextFMT("{}", variable.offset);
				TableNextColumn(), TextFMT("{}", GL::glsl_uniform_type_to_string(variable.glsl_type));
			}
			EndTable();
		}
	}
}

void ProgramWindow::init(const Context & ctx)
{
	assets = &ctx.game.assets;
}

void ProgramWindow::update(Context & ctx)
{
	using namespace ImGui;

	if (ctx.state.has_program_errors)
		state = State::ERROR;
	else
		state = State::OKAY;

	auto & selected_name = ctx.state.selected_program_name;

	if (BeginCombo("Program", selected_name.string.data()))
	{
		Selectable("Game", false, ImGuiSelectableFlags_Disabled);
		for (auto const & [name, _]: ctx.game.assets.programs)
		{
			if (Selectable(name.string.data()))
				selected_name = name, assets = &ctx.game.assets;

			if (ctx.game.assets.program_errors.contains(name))
				SameLine(), TextColored({1, 0, 0, 1}, "ERROR");
		}

		Selectable("Editor", false, ImGuiSelectableFlags_Disabled);
		for (auto const & [name, _]: ctx.editor_assets.programs)
		{
			if (Selectable(name.string.data()))
				selected_name = name, assets = &ctx.editor_assets;

			if (ctx.editor_assets.program_errors.contains(name))
				SameLine(), TextColored({1, 0, 0, 1}, "ERROR");
		}

		EndCombo();
	}
	if (not assets->programs.contains(selected_name))
	{
		Text("Pick a program");
		return;
	}
	auto & program = assets->programs.get(selected_name);

	SameLine(), Text("(id: %d)", program.id);

	if (Button("Reload"))
		if (not assets->reload_glsl_program(selected_name))
			ctx.state.should_game_render = false;

	if (auto iter = assets->program_errors.find(selected_name); iter != assets->program_errors.end())
	{
		PushStyleColor(ImGuiCol_Text, {1, 0, 0, 1});
		TextWrapped("%s", iter->second.data());
		PopStyleColor();
		return;
	}

	if (auto iter = assets->attrib_layouts.find(program.attrib_layout_name); iter != assets->attrib_layouts.end())
	{
		if (BeginTable("attrib_layout", 4, ImGuiTableFlags_BordersInnerH))
		{
			TableSetupColumn("Attribute");
			TableSetupColumn("Per Patch", ImGuiTableColumnFlags_WidthFixed);
			TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 100);
			TableSetupColumn("Location", ImGuiTableColumnFlags_WidthFixed);
			TableHeadersRow();

			auto & attrib_layout = iter->second;
			for (auto const & attrib: attrib_layout)
			{
				if (not attrib.is_used()) continue;
				TableNextColumn(), TextFMT("{}", attrib.key);
				TableNextColumn(), TextFMT("{}", attrib.is_per_patch);
				TableNextColumn(), TextFMT("{}", attrib.vec);
				TableNextColumn(), TextFMT("{}", attrib.location);
			}
			EndTable();
		}
	}
	else
	{
		TextFMT("{}", "does not have a vertex layout");
	}

	if (BeginTable("uniform_mappings", 3, ImGuiTableFlags_BordersInnerH))
	{
		TableSetupColumn("Uniform");
		TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 100);
		TableSetupColumn("Location", ImGuiTableColumnFlags_WidthFixed);
		TableHeadersRow();

		for (auto const & uniform: program.uniform_mappings)
		{
			TableNextColumn(), TextFMT("{}", uniform.key);
			TableNextColumn(), TextFMT("{}", GL::glsl_uniform_type_to_string(uniform.glsl_type));
			TableNextColumn(), TextFMT("{}", uniform.location);
		}
		EndTable();
	}

	if (BeginTable("uniform_block_mappings", 2, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingStretchProp))
	{
		TableSetupColumn("Uniform Block");
		TableSetupColumn("Variables");
		TableHeadersRow();

		for (auto const & uniform_block: program.uniform_block_mappings)
		{
			TableNextColumn();
			if (BeginTable("uniform_block", 3, ImGuiTableFlags_BordersInnerH))
			{
				TableSetupColumn("Name");
				TableSetupColumn("Binding", ImGuiTableColumnFlags_WidthFixed);
				TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed);
				TableHeadersRow();

				TableNextColumn(), TextFMT("{}", uniform_block.key);
				TableNextColumn(), TextFMT("{}", uniform_block.location);
				TableNextColumn(), TextFMT("{}", uniform_block.data_size);

				EndTable();
			}
			TableNextColumn();
			if (BeginTable("block_variables", 3, ImGuiTableFlags_BordersInnerH))
			{
				TableSetupColumn("Offset", ImGuiTableColumnFlags_WidthFixed);
				TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 100);
				TableSetupColumn("Name");
				TableHeadersRow();

				for (auto const & variable: uniform_block.variables)
				{
					TableNextColumn(), TextFMT("{}", variable.offset);
					TableNextColumn(), TextFMT("{}", GL::glsl_uniform_type_to_string(variable.glsl_type));
					TableNextColumn(), TextFMT("{}", variable.key);
				}
				EndTable();
			}
		}
		EndTable();
	}

	if (BeginTable("storage_block_mappings", 2, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingStretchProp))
	{
		TableSetupColumn("Storage Block");
		TableSetupColumn("Variables");
		TableHeadersRow();

		for (auto const & storage_block: program.storage_block_mappings)
		{
			TableNextColumn();
			if (BeginTable("storage_block", 3, ImGuiTableFlags_BordersInnerH))
			{
				TableSetupColumn("Name");
				TableSetupColumn("Binding", ImGuiTableColumnFlags_WidthFixed);
				TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed);
				TableHeadersRow();

				TableNextColumn(), TextFMT("{}", storage_block.key);
				TableNextColumn(), TextFMT("{}", storage_block.location);
				TableNextColumn(), TextFMT("{}", storage_block.data_size);

				EndTable();
			}
			TableNextColumn();
			if (BeginTable("block_variables", 3, ImGuiTableFlags_BordersInnerH))
			{
				TableSetupColumn("Offset", ImGuiTableColumnFlags_WidthFixed);
				TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 100);
				TableSetupColumn("Name");
				TableHeadersRow();

				for (auto const & variable: storage_block.variables)
				{
					TableNextColumn(), TextFMT("{}", variable.offset);
					TableNextColumn(), TextFMT("{}", GL::glsl_uniform_type_to_string(variable.glsl_type));
					TableNextColumn(), TextFMT("{}", variable.key);
				}
				EndTable();
			}
		}
		EndTable();
	}
}

void TextureWindow::update(Context & ctx)
{
	using namespace ImGui;

	auto & textures = ctx.game.assets.textures;
	auto & selected_name = ctx.state.selected_texture_name;

	if (BeginCombo("##", selected_name.string.data()))
	{
		for (auto & [name, _]: textures)
			if (Selectable(name.string.data()))
				selected_name = name, is_texture_changed = true;

		EndCombo();
	}
	if (auto it = textures.find(selected_name); it != textures.end())
	{
		auto & [_, texture] = *it;
		SameLine(), Text("id %d", texture.id);

		if (is_texture_changed)
		{
			using namespace GL;

			current_level = 0;
			glGetTextureParameteriv(texture.id, GL_TEXTURE_IMMUTABLE_LEVELS, &texture_levels);

			GLenum format;
			glGetTextureLevelParameteriv(texture.id, 0, GL_TEXTURE_INTERNAL_FORMAT, &format);
			if (format == GL_RGB8) texture_format = "RGB8";
			else if (format == GL_RGBA8) texture_format = "RGBA8";
			else if (format == GL_RGB32F) texture_format = "RGB32F";
			else if (format == GL_RGBA32F) texture_format = "RGBA32F";
			else if (format == GL_SRGB8) texture_format = "SRGB8";
			else if (format == GL_SRGB8_ALPHA8) texture_format = "SRGB8_ALPHA8";
			else texture_format = "UNKNOWN";

			texture_size = 0;
			int is_compressed;
			glGetTextureLevelParameteriv(texture.id, 0, GL_TEXTURE_COMPRESSED, &is_compressed);
			if (is_compressed)
				for (int i = 0; i < texture_levels; ++i)
				{
					i32 level_size;
					glGetTextureLevelParameteriv(texture.id, i, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &level_size);
					texture_size += level_size;
				}
			else
			{
				// assumes all levels have the same format (because Texture2D uses glTextureStorage2D)
				i32 channel_bits, texel_size = 0;
				glGetTextureLevelParameteriv(texture.id, 0, GL_TEXTURE_RED_SIZE, &channel_bits), texel_size += channel_bits;
				glGetTextureLevelParameteriv(texture.id, 0, GL_TEXTURE_GREEN_SIZE, &channel_bits), texel_size += channel_bits;
				glGetTextureLevelParameteriv(texture.id, 0, GL_TEXTURE_BLUE_SIZE, &channel_bits), texel_size += channel_bits;
				glGetTextureLevelParameteriv(texture.id, 0, GL_TEXTURE_ALPHA_SIZE, &channel_bits), texel_size += channel_bits;
				texel_size /= 8;

				for (int i = 0; i < texture_levels; ++i)
				{
					i32x2 dimensions;
					glGetTextureLevelParameteriv(texture.id, i, GL_TEXTURE_WIDTH, &dimensions.x);
					glGetTextureLevelParameteriv(texture.id, i, GL_TEXTURE_HEIGHT, &dimensions.y);

					texture_size += texel_size * compMul(dimensions);
				}
			}


			is_texture_changed = false;
			is_level_changed = true;
		}

		LabelText("Format", "%.*s", texture_format.size(), texture_format.data());
		if (texture_size >> 20 != 0)
			LabelText("Size", "%d MiB", texture_size >> 20);
		else
			LabelText("Size", "%d KiB", texture_size >> 10);

		if (SliderInt("Level", &current_level, 0, texture_levels - 1))
			is_level_changed = true;

		if (is_level_changed)
		{
			using namespace GL;

			Texture2D new_view;
			new_view.init(
				Texture2D::ViewDesc{
					.source = texture,
					.base_level = current_level,
					.level_count = 1
				}
			);
			view = move(new_view);

			glGetTextureLevelParameterfv(view.id, 0, GL_TEXTURE_WIDTH, &level_resolution.x);
			glGetTextureLevelParameterfv(view.id, 0, GL_TEXTURE_HEIGHT, &level_resolution.y);

			f32 const max_resolution = 240;
			view_size = max_resolution / compMax(level_resolution) * level_resolution;

			is_level_changed = false;
		}
		LabelText("Resolution", "%d x %d", i32(level_resolution.x), i32(level_resolution.y));
		ImageGL(
			reinterpret_cast<void *>(i64(view.id)),
			{view_size.x, view_size.y}
		);
	}
	else
	{
		Text("Pick a texture");
	}
}

void CubemapWindow::init(const Context & ctx)
{
	auto view_resolution = i32x2(240, 240);

	framebuffer.init(
		GL::FrameBuffer::Desc{
			.resolution = view_resolution,
			.attachments = {
				{
					.type = &GL::FrameBuffer::color0,
					.desc = GL::Texture2D::AttachmentDesc{
						.internal_format = GL::GL_RGBA16F,
					},
				}
			}
		}
	);
}

void CubemapWindow::update(Context & ctx)
{
	using namespace ImGui;

	should_render = true;

	// TODO(bekorn): maybe rename Assets::texture_cubemaps -> Assets::cubemaps
	auto & cubemaps = ctx.game.assets.texture_cubemaps;
	auto & selected_name = ctx.state.selected_cubemap_name;

	if (BeginCombo("Cubemap", selected_name.string.data()))
	{
		for (auto & [name, _]: cubemaps)
			if (Selectable(name.string.data()))
				selected_name = name, is_changed = true;

		EndCombo();
	}
	if (auto it = cubemaps.find(selected_name); it != cubemaps.end())
	{
		auto & [_, cubemap] = *it;
		SameLine(), Text("(id %d)", cubemap.id);

		if (is_changed)
		{
			current_level = 0;
			GL::glGetTextureParameteriv(cubemap.id, GL::GL_TEXTURE_IMMUTABLE_LEVELS, &levels);

			is_changed = false;
			is_level_changed = true;
		}

		if (SliderInt("Level", &current_level, 0, levels - 1))
			is_level_changed = true;

		if (is_level_changed)
		{
			GL::TextureCubemap new_view;
			new_view.init(
				GL::TextureCubemap::ViewDesc{
					.source = cubemap,
					.base_level = current_level,
					.level_count = 1
				}
			);
			view = move(new_view);

			GL::glGetTextureLevelParameterfv(view.id, 0, GL::GL_TEXTURE_WIDTH, &size.x);
			GL::glGetTextureLevelParameterfv(view.id, 0, GL::GL_TEXTURE_HEIGHT, &size.y);

			f32 const max_resolution = 240;
			view_size = max_resolution / glm::compMax(size) * size;

			is_level_changed = false;
		}
		LabelText("Resolution", "%d x %d", i32(size.x), i32(size.y));
		Image(
			reinterpret_cast<void *>(i64(framebuffer.color0.id)),
			{view_size.x, view_size.y},
			{0, 1}, {1, 0}
		);
	}
	else
	{
		Text("Pick a cubemap");
	}
}

void CubemapWindow::render(const Context & ctx)
{
	using namespace GL;

	if (not should_render)
		return;

	should_render = false;

	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.id);
	glViewport(i32x2(0), framebuffer.resolution);

	auto & environment_mapping_program = ctx.game.assets.programs.get("environment_mapping");
	glUseProgram(environment_mapping_program.id);
	glUniformHandleui64ARB(
		GetLocation(environment_mapping_program.uniform_mappings, "environment_map"),
		view.handle
	);
	auto view = visit([](Render::Camera auto & c) { return c.get_view(); }, ctx.game.camera);
	auto projection = visit([](Render::Camera auto & c) { return c.get_projection(); }, ctx.game.camera);
	auto view_projection = projection * view;
	auto invVP = inverse(f32x3x3(view_projection));
	auto view_dirs = invVP * f32x4x3{
		{-1, -1, +1}, // uv 0,0
		{+1, -1, +1}, // uv 1,0
		{-1, +1, +1}, // uv 0,1
		{+1, +1, +1}, // uv 1,1
	};
	glUniformMatrix4x3fv(
		GetLocation(environment_mapping_program.uniform_mappings, "view_directions"),
		1, false, begin(view_dirs)
	);
	glBindVertexArray(GL::dummy_vao.id);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	// TODO(bekorn): HDR -> LDR tone mapping? Maybe it can be optional via a checkbox.
	//  also,what does TextureWindow show? does it even have/need gamma correction & tone mapping?

	// gamma correction
	{
		glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);

		glDepthMask(false), glDepthFunc(GL_ALWAYS);

		// TODO(bekorn): move this into the core project (a new project besides game and editor)
		auto & gamma_correction_program = ctx.game.assets.programs.get("gamma_correction"_name);
		glUseProgram(gamma_correction_program.id);
		glUniformHandleui64ARB(
			GetLocation(gamma_correction_program.uniform_mappings, "color_attachment"),
			framebuffer.color0.handle
		);
		glBindVertexArray(GL::dummy_vao.id);
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}
}

void VolumeWindow::init(const Context & ctx)
{
	auto view_resolution = i32x2(240, 240);

	framebuffer.init(
		GL::FrameBuffer::Desc{
			.resolution = view_resolution,
			.attachments = {
				{
					.type = &GL::FrameBuffer::color0,
					.desc = GL::Texture2D::AttachmentDesc{
						.internal_format = GL::GL_RGBA16F,
					},
				},
				{
					.type = &GL::FrameBuffer::depth,
					.desc = GL::Texture2D::AttachmentDesc{
						.internal_format = GL::GL_DEPTH_COMPONENT16,
					},
				}
			}
		}
	);
}

void VolumeWindow::update(Context & ctx)
{
	using namespace ImGui;

	should_render = true;

	auto & volumes = ctx.game.assets.volumes;
	auto & selected_name = ctx.state.selected_volume_name;

	if (BeginCombo("Volume", selected_name.string.data()))
	{
		for (auto & [name, _]: volumes)
			if (Selectable(name.string.data()))
				selected_name = name, is_changed = true;

		EndCombo();
	}
	if (auto it = volumes.find(selected_name); it != volumes.end())
	{
		auto & [_, volume] = *it;
		SameLine(), Text("(id %d)", volume.id);

		if (is_changed)
		{
			current_level = 0;
			GL::glGetTextureParameteriv(volume.id, GL::GL_TEXTURE_IMMUTABLE_LEVELS, &levels);

			is_changed = false;
			is_level_changed = true;
		}

		if (SliderInt("Level", &current_level, 0, levels - 1))
			is_level_changed = true;

		if (is_level_changed)
		{
			GL::Texture3D new_view;
			new_view.init(
				GL::Texture3D::ViewDesc{
					.source = volume,
					.base_level = current_level,
					.level_count = 1
				}
			);
			view = move(new_view);

			GL::glGetTextureLevelParameterfv(view.id, 0, GL::GL_TEXTURE_WIDTH, &size.x);
			GL::glGetTextureLevelParameterfv(view.id, 0, GL::GL_TEXTURE_HEIGHT, &size.y);
			GL::glGetTextureLevelParameterfv(view.id, 0, GL::GL_TEXTURE_DEPTH, &size.z);

			f32 const max_resolution = 240;
			view_size = max_resolution / compMax(f32x2(size)) * f32x2(size);

			is_level_changed = false;
		}

		LabelText("Resolution", "%d x %d x %d", i32(size.x), i32(size.y), i32(size.z));

		SliderFloat3("Slice Position", begin(slice_pos), 0, 1, "%.2f");

		Image(
			reinterpret_cast<void *>(i64(framebuffer.color0.id)),
			{view_size.x, view_size.y},
			{0, 1}, {1, 0}
		);
	}
	else
	{
		Text("Pick a volume");
	}
}

void VolumeWindow::render(const Context & ctx)
{
	using namespace GL;

	if (not should_render)
		return;

	should_render = false;

	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.id);
	glViewport(i32x2(0), framebuffer.resolution);

	glDisable(GL_CULL_FACE);
	glColorMask(true, true, true, true), glDepthMask(true), glDepthFunc(GL_LESS);

	f32x4 clear_color(1, 1, 1, 0.5);
	glClearNamedFramebufferfv(framebuffer.id, GL_COLOR, 0, begin(clear_color));
	f32 clear_depth(1);
	glClearNamedFramebufferfv(framebuffer.id, GL_DEPTH, 0, &clear_depth);

	auto & volume_slice_program = ctx.editor_assets.programs.get("volume_slice");
	glUseProgram(volume_slice_program.id);
	glUniformHandleui64ARB(
		GetLocation(volume_slice_program.uniform_mappings, "volume"),
		view.handle
	);
	glUniform3fv(
		GetLocation(volume_slice_program.uniform_mappings, "slice_pos"),
		1, begin(slice_pos)
	);
	auto view = visit([](Render::Camera auto & c) { return c.get_view(); }, ctx.game.camera);
	auto projection = visit([](Render::Camera auto & c) { return c.get_projection(); }, ctx.game.camera);
	auto view_projection = projection * view;
	auto invVP = inverse(f32x3x3(view_projection));
	glUniformMatrix4fv(
		GetLocation(volume_slice_program.uniform_mappings, "TransformVP"),
		1, false, begin(view_projection)
	);
	glDrawArrays(GL_POINTS, 0, 3);

	// gamma correction
	{
		glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);

		glDepthMask(false), glDepthFunc(GL_ALWAYS);

		// TODO(bekorn): move this into the core project (a new project besides game and editor)
		auto & gamma_correction_program = ctx.game.assets.programs.get("gamma_correction"_name);
		glUseProgram(gamma_correction_program.id);
		glUniformHandleui64ARB(
			GetLocation(gamma_correction_program.uniform_mappings, "color_attachment"),
			framebuffer.color0.handle
		);
		glBindVertexArray(GL::dummy_vao.id);
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}
}

void MeshWindow::update(Context & ctx)
{
	using namespace ImGui;

	auto & meshes = ctx.game.assets.meshes;
	auto & selected_name = ctx.state.selected_mesh_name;

	if (BeginCombo("Mesh", selected_name.string.data()))
	{
		for (auto const & [name, _]: meshes)
			if (Selectable(name.string.data()))
				selected_name = name;

		EndCombo();
	}
	if (not meshes.contains(selected_name))
	{
		Text("Pick a mesh");
		return;
	}
	auto & mesh = meshes.get(selected_name);


	Spacing(), Separator(), Text("Drawables");

	if (mesh.drawables.empty())
	{
		Text("Mesh has no drawables");
		return;
	}

	{
		auto ids = mesh.drawables | std::views::transform([](auto & d) { return d.vertex_array.id; });
		// Pattern: using fmt for a ImGui::LabelText
		_buffer.clear(), fmt::format_to(_buffer_iter, "{}", fmt::join(ids, ","));
		LabelText("VertexArrays", "%.*s", int(_buffer.size()), _buffer.data());
	}

	{
		max_index = mesh.drawables.size() - 1;
		SliderScalar("Drawable", ImGuiDataType_U64, &drawable_index, &min_index, &max_index);
	}
	auto const & drawable = mesh.drawables[drawable_index];
	SameLine(), Text("(id: %d)", drawable.vertex_array.id);


	Spacing(), Separator(), Text("Primitive");

	auto const & vao = drawable.vertex_array;

	int vertex_buffer_size;
	glGetNamedBufferParameteriv(vao.vertex_buffer.id, GL::GL_BUFFER_SIZE, &vertex_buffer_size);
	LabelText("Vertex Count", "%d", vao.vertex_count);
	LabelText("Element Count", "%d", vao.element_count);

	// TODO(bekorn): make a way to access the layout through the material
// 	auto const & layout = drawable.named_material.shader.layout;
//	if (BeginTable("Attributes", 2, ImGuiTableFlags_BordersInnerH))
//	{
//		TableSetupColumn("Key"), TableSetupColumn("Data");
//		TableHeadersRow();
//
//		for (auto i = 0 ; i < Geometry::ATTRIBUTE_COUNT; ++i)
//		{
//			auto const & attrib = layout.attributes[i];
//			if (not attrib.is_used()) continue;
//
//			TableNextColumn(), TextFMT("{}", attrib.key);
//			TableNextColumn(), TextFMT("{}", attrib.vec);
//		}
//		EndTable();
//	}

	Spacing(), Separator();
	LabelText("Material", "%s", drawable.named_material.name.string.data());
	// TODO(bekorn): make the material selectable
//	if (SameLine(), Button("Select"))
//		ctx.state.selected_material_name = drawable.named_material.name;
}

void NodeEditor::update(Context & ctx)
{
	using namespace ImGui;

	auto & scene_tree = ctx.game.assets.scene_tree;
	auto & selected_name = ctx.state.selected_node_name;

	bool node_changed = false;
	if (BeginCombo("Node", selected_name.string.data()))
	{
		// provide empty option to deselect
		if (Selectable("##"))
			selected_name = "", node_changed = true;

		auto indent = GetStyle().IndentSpacing;
		for (auto & node: scene_tree.depth_first())
		{
			if (node.depth) Indent(indent * node.depth);

			if (Selectable(node.name.string.data()))
				selected_name = node.name, node_changed = true;

			if (node.depth) Unindent(indent * node.depth);
		}

		EndCombo();
	}
	if (auto it = scene_tree.named_indices.find(selected_name); it == scene_tree.named_indices.end())
	{
		Text("Pick a node");
		return;
	}
	else
	{
		auto & [_, node_index] = *it;
		auto & node = scene_tree.get(node_index);

		auto const & parent_name = node.depth == 0
								   ? "-"
								   : scene_tree.get({node.depth - 1, node.parent_index}).name.string.data();
		LabelText("Parent", "%s", parent_name);

		auto const & mesh_name = node.mesh == nullptr
								 ? "-"
								 : "<TODO>";
		LabelText("Mesh", "%s", mesh_name);


		Spacing(), Separator(), Text("Transform");
		auto & transform = node.transform;

		SliderFloat3("Position", begin(transform.position), -2, 2, "%.2f");

		if (node_changed)
		{
			mesh_orientation = glm::degrees(glm::eulerAngles(node.transform.rotation));
			mesh_orientation = glm::mod(mesh_orientation, 360.f);
		}
		if (SliderFloat3("Rotation", begin(mesh_orientation), 0, 360, "%.2f"))
		{
			transform.rotation = glm::quat(glm::radians(mesh_orientation));
		}

		auto scalar_scale = transform.scale.x;
		SliderFloat(
			"Scale", &scalar_scale, 0.001, 10, "%.3f", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat
		);
		transform.scale = f32x3(scalar_scale);
	}
}

void CameraWindow::update(Context & ctx)
{
	using namespace ImGui;

	if (std::holds_alternative<Render::PerspectiveCamera>(ctx.game.camera))
	{
		auto & camera = std::get<Render::PerspectiveCamera>(ctx.game.camera);
		Text("Perspective camera");
		DragFloat3("Position", begin(camera.position));
		DragFloat3("Up", begin(camera.up));
		DragFloat3("Target", begin(camera.target));
		DragFloat("FOV", &camera.fov);
		DragFloat("Near", &camera.near);
		DragFloat("Far", &camera.far);
	}
	else if (std::holds_alternative<Render::OrthographicCamera>(ctx.game.camera))
	{
		auto & camera = std::get<Render::OrthographicCamera>(ctx.game.camera);
		Text("Orthographic camera");
		DragFloat3("Position", begin(camera.position));
		DragFloat3("Up", begin(camera.up));
		DragFloat3("Target", begin(camera.target));
		DragFloat("Left", &camera.left);
		DragFloat("Right", &camera.right);
		DragFloat("Bottom", &camera.bottom);
		DragFloat("Top", &camera.top);
	}
}
}