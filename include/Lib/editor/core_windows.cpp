#include "core_windows.hpp"

namespace Editor
{
void GameWindow::create(Context const & ctx)
{
	resolution = ctx.game.resolution;

	color_attachment.create(
		GL::Texture2D::AttachmentDescription{
			.dimensions = resolution,
			.internal_format = GL::GL_RGBA8,
			.mag_filter = GL::GL_NEAREST,
		}
	);

	depth_attachment.create(
		GL::Texture2D::AttachmentDescription{
			.dimensions = resolution,
			.internal_format = GL::GL_DEPTH_COMPONENT16,
		}
	);

	framebuffer.create(
		GL::FrameBuffer::Description{
			.attachments = {
				{
					.type = GL::GL_COLOR_ATTACHMENT0,
					.texture = color_attachment,
				},
				{
					.type = GL::GL_DEPTH_ATTACHMENT,
					.texture = depth_attachment,
				},
			}
		}
	);


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
	static bool render_single_frame;
	if (render_single_frame)
		render_single_frame = ctx.state.should_game_render = false;
	if (render_single_frame = (SameLine(), Button("Render Single Frame")))
		ctx.state.should_game_render = true;

	EndDisabled();

	{
		Text("Resolution: %dx%d", ctx.game.resolution.x, ctx.game.resolution.y);
		static f32 scale = 0.5;
		SameLine(), SliderFloat("Scale", &scale, 0.1, 10.0, "%.1f");

		auto resolution = ImVec2(f32(ctx.game.resolution.x) * scale, f32(ctx.game.resolution.y) * scale);
		// custom uvs because defaults are flipped on y-axis
		auto uv0 = ImVec2(0, 1);
		auto uv1 = ImVec2(1, 0);

		auto border_color = ImVec4{0, 0, 0, 1};
		if (ctx.state.has_program_errors)
			border_color.x = 1;

		auto game_pos = GetCursorPos();
		Image(
			reinterpret_cast<void *>(i64(ctx.game.framebuffer_color_attachment.id)),
			resolution, uv0, uv1
		);
		auto editor_pos = ImVec2(game_pos.x - 1, game_pos.y - 1); // because of image borders
		SetCursorPos(editor_pos), Image(
			reinterpret_cast<void *>(i64(color_attachment.id)),
			resolution, uv0, uv1, {1, 1, 1, 1}, border_color
		);

		SameLine(), Image(
			reinterpret_cast<void *>(i64(ctx.game.framebuffer_depth_attachment.id)),
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
		glViewport(0, 0, resolution.x, resolution.y);
		glColorMask(true, true, true, true), glDepthMask(true);
		const f32 clear_depth{1};
		glClearNamedFramebufferfv(framebuffer.id, GL_DEPTH, 0, &clear_depth);
		const f32x4 clear_color{0, 0, 0, 0};
		glClearNamedFramebufferfv(framebuffer.id, GL_COLOR, 0, begin(clear_color));
	}

	// Draw gizmos
	{
		glEnable(GL_CULL_FACE), glCullFace(GL_BACK);
		glEnable(GL_DEPTH_TEST), glDepthFunc(GL_LESS);
		glColorMask(true, true, true, true), glDepthMask(true);

		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.id);

		auto & gizmo_program = ctx.editor_assets.programs.get("gizmo"_name);
		glUseProgram(gizmo_program.id);

		auto size = glm::max(50.f, 0.1f * glm::compMin(resolution));
		auto const padding = 8;
		glViewport(resolution.x - size - padding, resolution.y - size - padding, size, size);

		auto view = visit(
			[](Camera auto & c) { return glm::lookAt(f32x3(0), c.target - c.position, c.up); }, ctx.game.camera
		);
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

		glViewport(0, 0, resolution.x, resolution.y);
		glDepthMask(false), glDepthFunc(GL_ALWAYS);

		// TODO(bekorn): move this into the core project (a new project besides game and editor)
		auto & gamma_correction_program = ctx.game.assets.programs.get("gamma_correction"_name);
		glUseProgram(gamma_correction_program.id);
		glUniformHandleui64ARB(
			GetLocation(gamma_correction_program.uniform_mappings, "color_attachment"),
			color_attachment.handle
		);
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}
}

void MetricsWindow::update(Context & ctx)
{
	using namespace ImGui;

	{
		static array<f64, 30> deltas{};
		static usize delta_idx = 0;

		deltas[delta_idx] = ctx.state.frame_info.seconds_since_last_frame;
		delta_idx = (delta_idx + 1) % deltas.size();
		f64 average_delta = 0;
		for (auto const & delta: deltas)
			average_delta += delta;
		average_delta /= deltas.size();

		TextFMT("Average frame:  {:>6.2f} ms, {:>4.0f} fps", average_delta * 1000., 1. / average_delta);
	}

	{
		static array<f64, 30> deltas{};
		static usize delta_idx = 0;

		deltas[delta_idx] = ctx.state.game_update_in_seconds;
		delta_idx = (delta_idx + 1) % deltas.size();
		f64 average_delta = 0;
		for (auto const & delta: deltas)
			average_delta += delta;
		average_delta /= deltas.size();

		TextFMT("Average update: {:>6.2f} ms", average_delta * 1000., 1. / average_delta);
	}

	{
		static array<f64, 30> deltas{};
		static usize delta_idx = 0;

		deltas[delta_idx] = ctx.state.game_render_in_seconds;
		delta_idx = (delta_idx + 1) % deltas.size();
		f64 average_delta = 0;
		for (auto const & delta: deltas)
			average_delta += delta;
		average_delta /= deltas.size();

		SameLine(), TextFMT(",  render: {:>6.2f} ms", average_delta * 1000., 1. / average_delta);
	}

	TextFMT("Frame: {:6}, Time: {:7.2f}", ctx.state.frame_info.idx, ctx.state.frame_info.seconds_since_start);
}

void UniformBufferWindow::update(Context & ctx)
{
	using namespace ImGui;

	static Name uniform_buffer_name;
	if (BeginCombo("Uniform Buffer", uniform_buffer_name.string.data()))
	{
		for (auto const & [name, _]: ctx.game.assets.uniform_blocks)
			if (Selectable(name.string.data()))
				uniform_buffer_name = name;

		EndCombo();
	}
	if (not ctx.game.assets.uniform_blocks.contains(uniform_buffer_name))
	{
		Text("Pick a uniform buffer");
		return;
	}
	auto named_uniform_buffer = ctx.game.assets.uniform_blocks.get_named(uniform_buffer_name);
	auto & uniform_buffer = named_uniform_buffer.data;

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

void CubemapWindow::create(const Context & ctx)
{
	auto view_resolution = i32x2(240, 240);

	framebuffer_color_attachment.create(
		GL::Texture2D::AttachmentDescription{
			.dimensions = view_resolution,
			.internal_format = GL::GL_RGBA8,
		}
	);

	framebuffer.create(
		GL::FrameBuffer::Description{
			.attachments = {
				{
					.type = GL::GL_COLOR_ATTACHMENT0,
					.texture = framebuffer_color_attachment,
				}
			}
		}
	);
}

void CubemapWindow::update(Context & ctx)
{
	using namespace ImGui;

	should_render = true;

	static bool is_changed = false;
	static Name current_name;
	if (BeginCombo("Cubemap", current_name.string.data()))
	{
		for (auto & [name, _]: ctx.game.assets.texture_cubemaps)
			if (Selectable(name.string.data()))
				current_name = name, is_changed = true;

		EndCombo();
	}
	// TODO(bekorn): maybe rename Assets::texture_cubemaps -> Assets::cubemaps
	if (auto it = ctx.game.assets.texture_cubemaps.find(current_name); it != ctx.game.assets.texture_cubemaps.end())
	{
		auto & [_, cubemap] = *it;
		SameLine(), Text("(id %d)", cubemap.id);

		static bool is_level_changed = false;
		static i32 levels = 0, current_level = 0;
		if (is_changed)
		{
			current_level = 0;
			GL::glGetTextureParameteriv(cubemap.id, GL::GL_TEXTURE_IMMUTABLE_LEVELS, &levels);

			is_changed = false;
			is_level_changed = true;
		}

		if (SliderInt("Level", &current_level, 0, levels - 1))
			is_level_changed = true;

		static f32x2 size, view_size;
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
			reinterpret_cast<void *>(i64(framebuffer_color_attachment.id)),
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
	glViewport(0, 0, 240, 240);

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
			framebuffer_color_attachment.handle
		);
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}
}

void ProgramWindow::update(Context & ctx)
{
	using namespace ImGui;

	if (ctx.state.has_program_errors)
		state = State::ERROR;
	else
		state = State::OKAY;

	static Name program_name;
	static Assets * assets = &ctx.game.assets;
	if (BeginCombo("Program", program_name.string.data()))
	{
		Selectable("Game", false, ImGuiSelectableFlags_Disabled);
		for (auto const & [name, _]: ctx.game.assets.programs)
		{
			if (Selectable(name.string.data()))
				program_name = name, assets = &ctx.game.assets;

			if (ctx.game.assets.program_errors.contains(name))
				SameLine(), TextColored({1, 0, 0, 1}, "ERROR");
		}

		Selectable("Editor", false, ImGuiSelectableFlags_Disabled);
		for (auto const & [name, _]: ctx.editor_assets.programs)
		{
			if (Selectable(name.string.data()))
				program_name = name, assets = &ctx.editor_assets;

			if (ctx.editor_assets.program_errors.contains(name))
				SameLine(), TextColored({1, 0, 0, 1}, "ERROR");
		}

		EndCombo();
	}
	if (not assets->programs.contains(program_name))
	{
		Text("Pick a program");
		return;
	}
	auto named_program = assets->programs.get_named(program_name);

	SameLine(), Text("(id: %d)", named_program.data.id);

	if (Button("Reload"))
		if (not assets->reload_glsl_program(named_program.name))
			ctx.state.should_game_render = false;

	if (auto iter = assets->program_errors.find(program_name); iter != assets->program_errors.end())
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

	static bool is_texture_changed = false;
	static Name texture_name;
	if (BeginCombo("Texture", texture_name.string.data()))
	{
		for (auto & [name, _]: ctx.game.assets.textures)
			if (Selectable(name.string.data()))
				texture_name = name, is_texture_changed = true;

		EndCombo();
	}
	if (auto it = ctx.game.assets.textures.find(texture_name); it != ctx.game.assets.textures.end())
	{
		auto & [_, texture] = *it;
		SameLine(), Text("(id %d)", texture.id);

		static bool is_level_changed = false;
		static i32 texture_levels = 0, current_level = 0;
		if (is_texture_changed)
		{
			current_level = 0;
			GL::glGetTextureParameteriv(texture.id, GL::GL_TEXTURE_IMMUTABLE_LEVELS, &texture_levels);

			is_texture_changed = false;
			is_level_changed = true;
		}

		if (SliderInt("Level", &current_level, 0, texture_levels - 1))
			is_level_changed = true;

		static f32x2 texture_size, view_size;
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
			texture_view = move(new_view);

			GL::glGetTextureLevelParameterfv(texture_view.id, 0, GL::GL_TEXTURE_WIDTH, &texture_size.x);
			GL::glGetTextureLevelParameterfv(texture_view.id, 0, GL::GL_TEXTURE_HEIGHT, &texture_size.y);

			f32 const max_resolution = 240;
			view_size = max_resolution / glm::compMax(texture_size) * texture_size;

			is_level_changed = false;
		}
		LabelText("Resolution", "%d x %d", i32(texture_size.x), i32(texture_size.y));
		Image(
			reinterpret_cast<void *>(i64(texture_view.id)),
			{view_size.x, view_size.y}
		);
	}
	else
	{
		Text("Pick a texture");
	}
}

void MeshWindow::update(Context & ctx)
{
	using namespace ImGui;

	static Name mesh_name;
	if (BeginCombo("Mesh", mesh_name.string.data()))
	{
		for (auto const & [name, _]: ctx.game.assets.meshes)
			if (Selectable(name.string.data()))
				mesh_name = name;

		EndCombo();
	}
	if (not ctx.game.assets.meshes.contains(mesh_name))
	{
		Text("Pick a mesh");
		return;
	}
	auto & mesh = ctx.game.assets.meshes.get(mesh_name);


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
				auto & attribute_mappings = ctx.game.assets.programs.get(
					GLTF::pbrMetallicRoughness_program_name
				).attribute_mappings;
				for (auto & drawable: mesh.drawables)
					drawable.load(attribute_mappings);
			}
		}
	}

	static u64 drawable_index;
	{
		static u64 min_index = 0, max_index;
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

void NodeEditor::update(Context & ctx)
{
	using namespace ImGui;

	static Name node_name;
	bool node_changed = false;
	if (BeginCombo("Node", node_name.string.data()))
	{
		auto indent = ImGui::GetStyle().IndentSpacing;
		for (auto & node: ctx.game.assets.scene_tree.depth_first())
		{
			if (node.depth) Indent(indent * node.depth);

			if (Selectable(node.name.string.data()))
				node_name = node.name, node_changed = true;

			if (node.depth) ImGui::Unindent(indent * node.depth);
		}

		EndCombo();
	}
	if (not ctx.game.assets.scene_tree.named_indices.contains(node_name))
	{
		Text("Pick a node");
		return;
	}
	auto & node_index = ctx.game.assets.scene_tree.named_indices.get(node_name);
	auto & node = ctx.game.assets.scene_tree.get(node_index);


	auto const & parent_name = node.depth == 0
							   ? "-"
							   : ctx.game.assets.scene_tree.get({node.depth - 1, node.parent_index}).name.string.data();
	LabelText("Parent", "%s", parent_name);

	auto const & mesh_name = node.mesh == nullptr
							 ? "-"
							 : "<TODO>";
	LabelText("Mesh", "%s", mesh_name);


	Spacing(), Separator(), Text("Transform");
	auto & transform = node.transform;

	SliderFloat3("Position", begin(transform.position), -2, 2, "%.2f");

	static f32x3 mesh_orientation;
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

void GameSettingsWindow::update(Context & ctx)
{
	using namespace ImGui;

	ColorEdit3("Clear color", begin(ctx.game.clear_color));

	static bool is_vsync_on = true;
	if (Checkbox("is vsync on", &is_vsync_on))
		glfwSwapInterval(is_vsync_on ? 1 : 0);
}

void MaterialWindow::update(Context & ctx)
{
	using namespace ImGui;

	static Name material_name;
	if (BeginCombo("Material", material_name.string.data()))
	{
		for (auto const & [name, _]: ctx.game.assets.materials)
			if (Selectable(name.string.data()))
				material_name = name;

		EndCombo();
	}
	if (not ctx.game.assets.materials.contains(material_name))
	{
		Text("Pick a material");
		return;
	}
	auto & material = ctx.game.assets.materials.get(material_name);
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
		//		ctx.game.gltf_material_is_dirty.push(material_name); // !!! Temporary
	}
}
}