#include "core_windows.hpp"

namespace Editor
{
void GameWindow::create(Context const & ctx)
{
	framebuffer.create(GL::FrameBuffer::Description{
		.resolution = ctx.game.framebuffer.resolution,
		.attachments = {
			{
				.type = &GL::FrameBuffer::color0,
				.description= GL::Texture2D::AttachmentDescription{
					.internal_format = GL::GL_RGBA8,
					.mag_filter = GL::GL_NEAREST,
				},
			},
			{
				.type = &GL::FrameBuffer::depth,
				.description = GL::Texture2D::AttachmentDescription{
					.internal_format = GL::GL_DEPTH_COMPONENT16,
				},
			},
		}
	});

	// border
	border.create(ctx, *this);

	// Load gizmo meshes
	auto & attribute_mappings = ctx.editor_assets.programs.get("gizmo"_name).attribute_mappings;
	for (auto & [_, mesh]: ctx.editor_assets.meshes)
		for (auto & drawable: mesh.drawables)
			drawable.load(attribute_mappings);
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

	{
		Text("Resolution: %dx%d", ctx.game.framebuffer.resolution.x, ctx.game.framebuffer.resolution.y);
		SameLine(), SliderFloat("Scale", &scale, 0.1, 10.0, "%.1f");

		auto resolution = ImVec2(f32(ctx.game.framebuffer.resolution.x) * scale, f32(ctx.game.framebuffer.resolution.y) * scale);
		// custom uvs because defaults are flipped on y-axis
		auto uv0 = ImVec2(0, 1);
		auto uv1 = ImVec2(1, 0);

		auto border_color = ImVec4{0, 0, 0, 1};
		if (ctx.state.has_program_errors)
			border_color.x = 1;

		auto game_pos = GetCursorPos();
		Image(
			reinterpret_cast<void *>(i64(ctx.game.framebuffer.color0.id)),
			resolution, uv0, uv1
		);
		auto editor_pos = ImVec2(game_pos.x - 1, game_pos.y - 1); // because of image borders
		SetCursorPos(editor_pos), Image(
			reinterpret_cast<void *>(i64(framebuffer.color0.id)),
			resolution, uv0, uv1, {1, 1, 1, 1}, border_color
		);

		SameLine(), Image(
			reinterpret_cast<void *>(i64(ctx.game.framebuffer.depth.id)),
			resolution, uv0, uv1
		);
	}
}

void GameWindow::render(Context const & ctx)
{
	using namespace GL;

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

		auto view = visit([](Camera auto & c) { return c.get_view_without_translate(); }, ctx.game.camera);
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
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}
}

void GameWindow::Border::create(Context const & ctx, GameWindow const & game_window)
{
	for (auto & framebuffer: framebuffers)
		framebuffer.create(GL::FrameBuffer::Description{
			.resolution = game_window.framebuffer.resolution,
			.attachments = {
				{
					.type = &GL::FrameBuffer::color0,
					.description = GL::Texture2D::AttachmentDescription{
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

	glViewport(0, 0, framebuffers[0].resolution.x, framebuffers[0].resolution.y);
	glViewport(i32x2(0), framebuffers[0].resolution);

	glDisable(GL_DEPTH_TEST);
	glColorMask(true, true, true, true), glDepthMask(false);

	auto const clear_color = i32x2(-1);
	glClearNamedFramebufferiv(framebuffers[0].id, GL_COLOR, 0, begin(clear_color));
	glClearNamedFramebufferiv(framebuffers[1].id, GL_COLOR, 0, begin(clear_color));

	glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[0].id);

	// initialize
	glEnable(GL_SCISSOR_TEST);
	glScissor(
		border_width, border_width,
		framebuffers[0].resolution.x - 2 * border_width, framebuffers[0].resolution.y - 2 * border_width
	);
	glEnable(GL_SCISSOR_TEST), glScissor(i32x2(border_width), framebuffers[0].resolution - i32(2 * border_width));

	auto & jump_flood_init_program = ctx.editor_assets.programs.get("jump_flood_init");
	glUseProgram(jump_flood_init_program.id);
	auto view = visit([](Camera auto & c){ return c.get_view(); }, ctx.game.camera);
	auto proj = visit([](Camera auto & c){ return c.get_projection(); }, ctx.game.camera);
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
		glDrawArrays(GL_TRIANGLES, 0, 3);

		step /= 2;
		pingpong_destination = not pingpong_destination;
	}

	// finalize border
	glViewport(0, 0, game_window.framebuffer.resolution.x, game_window.framebuffer.resolution.y);
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
				TableNextColumn(), TextFMT("{}", GL::GLSLTypeToString(variable.glsl_type));
			}
			EndTable();
		}
	}
}

void ProgramWindow::create(const Context & ctx)
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
	auto named_program = assets->programs.get_named(selected_name);

	SameLine(), Text("(id: %d)", named_program.data.id);

	if (Button("Reload"))
		if (not assets->reload_glsl_program(named_program.name))
			ctx.state.should_game_render = false;

	if (auto iter = assets->program_errors.find(selected_name); iter != assets->program_errors.end())
	{
		TextColored({1, 0, 0, 1}, "%s", iter->second.data());
		return;
	}

	if (BeginTable("attribute_mappings", 4, ImGuiTableFlags_BordersInnerH))
	{
		TableSetupColumn("Attribute");
		TableSetupColumn("Per Patch", ImGuiTableColumnFlags_WidthFixed);
		TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 100);
		TableSetupColumn("Location", ImGuiTableColumnFlags_WidthFixed);
		TableHeadersRow();

		for (auto const & attribute: named_program.data.attribute_mappings)
		{
			TableNextColumn(), TextFMT("{}", attribute.key);
			TableNextColumn(), TextFMT("{}", attribute.per_patch ? "true" : "false");
			TableNextColumn(), TextFMT("{}", GL::GLSLTypeToString(attribute.glsl_type));
			TableNextColumn(), TextFMT("{}", attribute.location);
		}
		EndTable();
	}

	if (BeginTable("uniform_mappings", 3, ImGuiTableFlags_BordersInnerH))
	{
		TableSetupColumn("Uniform");
		TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 100);
		TableSetupColumn("Location", ImGuiTableColumnFlags_WidthFixed);
		TableHeadersRow();

		for (auto const & uniform: named_program.data.uniform_mappings)
		{
			TableNextColumn(), TextFMT("{}", uniform.key);
			TableNextColumn(), TextFMT("{}", GL::GLSLTypeToString(uniform.glsl_type));
			TableNextColumn(), TextFMT("{}", uniform.location);
		}
		EndTable();
	}

	if (BeginTable("uniform_block_mappings", 2, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingStretchProp))
	{
		TableSetupColumn("Uniform Block");
		TableSetupColumn("Variables");
		TableHeadersRow();

		for (auto const & uniform_block: named_program.data.uniform_block_mappings)
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
					TableNextColumn(), TextFMT("{}", GL::GLSLTypeToString(variable.glsl_type));
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

		for (auto const & storage_block: named_program.data.storage_block_mappings)
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
					TableNextColumn(), TextFMT("{}", GL::GLSLTypeToString(variable.glsl_type));
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
			current_level = 0;
			GL::glGetTextureParameteriv(texture.id, GL::GL_TEXTURE_IMMUTABLE_LEVELS, &texture_levels);

			is_texture_changed = false;
			is_level_changed = true;
		}

		if (SliderInt("Level", &current_level, 0, texture_levels - 1))
			is_level_changed = true;

		if (is_level_changed)
		{
			GL::Texture2D new_view;
			new_view.create(
				GL::Texture2D::ViewDescription{
					.source = texture,
					.base_level = current_level,
					.level_count = 1
				}
			);
			view = move(new_view);

			GL::glGetTextureLevelParameterfv(view.id, 0, GL::GL_TEXTURE_WIDTH, &texture_size.x);
			GL::glGetTextureLevelParameterfv(view.id, 0, GL::GL_TEXTURE_HEIGHT, &texture_size.y);

			f32 const max_resolution = 240;
			view_size = max_resolution / glm::compMax(texture_size) * texture_size;

			is_level_changed = false;
		}
		LabelText("Resolution", "%d x %d", i32(texture_size.x), i32(texture_size.y));
		Image(
			reinterpret_cast<void *>(i64(view.id)),
			{view_size.x, view_size.y}
		);
	}
	else
	{
		Text("Pick a texture");
	}
}

void CubemapWindow::create(const Context & ctx)
{
	auto view_resolution = i32x2(240, 240);

	framebuffer.create(
		GL::FrameBuffer::Description{
			.resolution = view_resolution,
			.attachments = {
				{
					.type = &GL::FrameBuffer::color0,
					.description = GL::Texture2D::AttachmentDescription{
						.internal_format = GL::GL_RGBA8,
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

	auto & cubemaps = ctx.game.assets.texture_cubemaps;
	auto & selected_name = ctx.state.selected_cubemap_name;

	if (BeginCombo("Cubemap", selected_name.string.data()))
	{
		for (auto & [name, _]: cubemaps)
			if (Selectable(name.string.data()))
				selected_name = name, is_changed = true;

		EndCombo();
	}
	// TODO(bekorn): maybe rename Assets::texture_cubemaps -> Assets::cubemaps
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
			new_view.create(
				GL::TextureCubemap::ViewDescription{
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
	auto view = visit([](Camera auto & c) { return c.get_view(); }, ctx.game.camera);
	auto projection = visit([](Camera auto & c) { return c.get_projection(); }, ctx.game.camera);
	auto view_projection = projection * view;
	auto invVP = glm::inverse(f32x3x3(view_projection));
	f32x4x3 view_dirs;
	view_dirs[0] = invVP * f32x3(-1, -1, 1); // left   bottom
	view_dirs[1] = invVP * f32x3(+1, -1, 1); // right  bottom
	view_dirs[2] = invVP * f32x3(-1, +1, 1); // left   up
	view_dirs[3] = invVP * f32x3(+1, +1, 1); // right  up
	glUniformMatrix4x3fv(
		GetLocation(environment_mapping_program.uniform_mappings, "view_directions"),
		1, false, begin(view_dirs)
	);
	glDrawArrays(GL_TRIANGLES, 0, 3);

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
		_buffer.clear(), fmt::format_to(_buffer_iter, "{}", fmt::join(ids, ","));
		LabelText("VertexArrays", "%.*s", int(_buffer.size()), _buffer.data());


		bool any_vertex_array_loaded = false;
		for (auto & drawable: mesh.drawables)
			any_vertex_array_loaded |= drawable.is_loaded();

		if (any_vertex_array_loaded)
		{
			if (Button("Unload all drawables"))
				for (auto & drawable: mesh.drawables)
					drawable.unload();
		}
		else
		{
			if (Button("Load all drawables"))
			{
				auto & program = ctx.game.assets.programs.get(GLTF::pbrMetallicRoughness_program_name);
				for (auto & drawable: mesh.drawables)
					drawable.load(program.attribute_mappings);
			}
		}
	}

	{
		max_index = mesh.drawables.size() - 1;
		SliderScalar("Drawable", ImGuiDataType_U64, &drawable_index, &min_index, &max_index);
	}
	auto const & drawable = mesh.drawables[drawable_index];
	SameLine(), Text("(id: %d)", drawable.vertex_array.id);


	Spacing(), Separator(), Text("Primitive");

	auto const & primitive = drawable.primitive;

	if (BeginTable("Attributes", 3, ImGuiTableFlags_BordersInnerH))
	{
		TableSetupColumn("Key"), TableSetupColumn("Data"), TableSetupColumn("Size");
		TableHeadersRow();

		for (auto const & [key, data]: primitive.attributes)
		{
			TableNextColumn(), TextFMT("{}", key);
			TableNextColumn(), TextFMT("{}x{}", data.type, data.dimension);
			TableNextColumn(), TextFMT("{}", data.buffer.size / (data.type.size() * data.dimension));
		}
		EndTable();
	}


	Spacing(), Separator();
	LabelText("Material", "%s", drawable.named_material.name.string.data());
}

void NodeEditor::update(Context & ctx)
{
	using namespace ImGui;

	auto & scene_tree = ctx.game.assets.scene_tree;
	auto & selected_name = ctx.state.selected_node_name;

	bool node_changed = false;
	if (BeginCombo("Node", selected_name.string.data()))
	{
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

	if (std::holds_alternative<PerspectiveCamera>(ctx.game.camera))
	{
		auto & camera = std::get<PerspectiveCamera>(ctx.game.camera);
		Text("Perspective camera");
		DragFloat3("Position", begin(camera.position));
		DragFloat3("Up", begin(camera.up));
		DragFloat3("Target", begin(camera.target));
		DragFloat("FOV", &camera.fov);
		DragFloat("Near", &camera.near);
		DragFloat("Far", &camera.far);
	}
	else if (std::holds_alternative<OrthographicCamera>(ctx.game.camera))
	{
		auto & camera = std::get<OrthographicCamera>(ctx.game.camera);
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