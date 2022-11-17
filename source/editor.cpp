#include "editor.hpp"

void Editor::create_framebuffer()
{
	resolution = game.resolution;

	framebuffer_color_attachment.create(
		GL::Texture2D::AttachmentDescription{
			.dimensions = resolution,
			.internal_format = GL::GL_RGBA8,
		}
	);

	framebuffer_depth_attachment.create(
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
					.texture = framebuffer_color_attachment,
				},
				{
					.type = GL::GL_DEPTH_ATTACHMENT,
					.texture = framebuffer_depth_attachment,
				},
			}
		}
	);
}

void Editor::create_cubemap_framebuffer()
{
	auto view_resolution = i32x2(240, 240);

	cubemap_framebuffer_color_attachment.create(
		GL::Texture2D::AttachmentDescription{
			.dimensions = view_resolution,
			.internal_format = GL::GL_RGBA8,
		}
	);

	cubemap_framebuffer.create(
		GL::FrameBuffer::Description{
			.attachments = {
				{
					.type = GL::GL_COLOR_ATTACHMENT0,
					.texture = cubemap_framebuffer_color_attachment,
				}
			}
		}
	);
}

void Editor::create()
{
	create_framebuffer();
	create_cubemap_framebuffer();

	// Enable docking
	auto & io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigDockingWithShift = true;

	// Table styling
	ImGui::GetStyle().CellPadding.x = 6;

	// Load gizmo meshes
	auto & attribute_mappings = editor_assets.programs.get("gizmo"_name).attribute_mappings;
	for (auto & [_, mesh] : editor_assets.meshes)
		for (auto & drawable : mesh.drawables)
			drawable.load(attribute_mappings);

	has_program_errors = not game.assets.program_errors.empty() or not editor_assets.program_errors.empty();
	// Game should not render if any program failed to compile
	if (has_program_errors)
		should_game_render = false;
}

// TODO(bekorn): as I understand, imgui already has a buffer to keep formatted strings, so this might be unnecessary
// ImGui + fmtlib utility (especially handy for tables)
inline static fmt::memory_buffer _buffer;
inline static auto _buffer_iter = std::back_inserter(_buffer);
inline static auto const TextFMT = []<typename... T>(fmt::format_string<T...> fmt, T&&... args)
{
	_buffer.clear(), fmt::vformat_to(_buffer_iter, fmt, fmt::make_format_args(args...));
	ImGui::TextUnformatted(_buffer.begin(), _buffer.end());
};

void Editor::metrics_window(FrameInfo const & frame_info, f64 game_update_in_seconds, f64 game_render_in_seconds)
{
	using namespace ImGui;

	Begin("Metrics", nullptr, ImGuiWindowFlags_NoCollapse);

	{
		static array<f64, 30> deltas{};
		static usize delta_idx = 0;

		deltas[delta_idx] = frame_info.seconds_since_last_frame;
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

		deltas[delta_idx] = game_update_in_seconds;
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

		deltas[delta_idx] = game_render_in_seconds;
		delta_idx = (delta_idx + 1) % deltas.size();
		f64 average_delta = 0;
		for (auto const & delta: deltas)
			average_delta += delta;
		average_delta /= deltas.size();

		SameLine(), TextFMT(",  render: {:>6.2f} ms", average_delta * 1000., 1. / average_delta);
	}

	TextFMT("Frame: {:6}, Time: {:7.2f}", frame_info.idx, frame_info.seconds_since_start);

	auto environment_map_time = game.environment_map_timer.average_in_nanoseconds;
	TextFMT("{:30} {:6} us | {:6} ns", "Enrironment Mapping", environment_map_time / 1'000, environment_map_time);

	auto gamma_correction_time = game.gamma_correction_timer.average_in_nanoseconds;
	TextFMT("{:30} {:6} us | {:6} ns", "Gamma Correction", gamma_correction_time / 1'000, gamma_correction_time);

	End();
}

void Editor::game_window()
{
	using namespace ImGui;

	Begin("Game", nullptr, ImGuiWindowFlags_NoCollapse);

	BeginDisabled(has_program_errors);

	Checkbox("Render", &should_game_render);
	static bool render_single_frame;
	if (render_single_frame)
		render_single_frame = should_game_render = false;
	if (render_single_frame = (SameLine(), Button("Render Single Frame")))
		should_game_render = true;

	EndDisabled();

	{
		Text("Resolution: %dx%d", game.resolution.x, game.resolution.y);
		static f32 scale = 0.5;
		SameLine(), SliderFloat("Scale", &scale, 0.1, 1.0, "%.1f");

		auto resolution = ImVec2(f32(game.resolution.x) * scale, f32(game.resolution.y) * scale);
		// custom uvs because defaults are flipped on y-axis
		auto uv0 = ImVec2(0, 1);
		auto uv1 = ImVec2(1, 0);

		auto border_color = ImVec4{0, 0, 0, 1};
		if (has_program_errors)
			border_color.x = 1;

		Image(
			reinterpret_cast<void*>(i64(game.framebuffer_color_attachment.id)),
			resolution, uv0, uv1
		);
		SameLine(GetCursorPosX()), Image(
			reinterpret_cast<void*>(i64(framebuffer_color_attachment.id)),
			resolution, uv0, uv1, {1, 1, 1, 1}, border_color
		);

		SameLine(), Image(
			reinterpret_cast<void*>(i64(game.framebuffer_depth_attachment.id)),
			resolution, uv0, uv1
		);
	}

	End();
}

void Editor::game_settings_window()
{
	using namespace ImGui;

	Begin("Game Settings", nullptr, ImGuiWindowFlags_NoCollapse);

	if (BeginCombo("Environment Map", game.settings.environment_map_name.string.data()))
	{
		for (auto & [name, _]: game.assets.texture_cubemaps)
			if (Selectable(name.string.data()))
				game.settings.environment_map_name = name;

		EndCombo();
	}

	Checkbox("Zpass", &game.settings.is_zpass_on);
	SameLine(), Checkbox("is env map comp", &game.settings.is_environment_mapping_comp);

	Checkbox("is gamma correction comp", &game.settings.is_gamma_correction_comp);

	static bool is_vsync_on = true;
	if (Checkbox("is vsync on", &is_vsync_on))
		glfwSwapInterval(is_vsync_on ? 1 : 0);

	End();
}

void Editor::node_settings_window()
{
	using namespace ImGui;

	Begin("Node Settings", nullptr, ImGuiWindowFlags_NoCollapse);

	static Name node_name;
	bool node_changed = false;
	if (BeginCombo("Node", node_name.string.data()))
	{
		auto indent = ImGui::GetStyle().IndentSpacing;
		for (auto & node: game.assets.scene_tree.depth_first())
		{
			if (node.depth) Indent(indent * node.depth);

			if (Selectable(node.name.string.data()))
				node_name = node.name, node_changed = true;

			if (node.depth) ImGui::Unindent(indent * node.depth);
		}

		EndCombo();
	}
	if (not game.assets.scene_tree.named_indices.contains(node_name))
	{
		Text("Pick a node");
		End();
		return;
	}
	auto & node_index = game.assets.scene_tree.named_indices.get(node_name);
	auto & node = game.assets.scene_tree.get(node_index);


	auto const & parent_name = node.depth == 0
							   ? "-"
							   : game.assets.scene_tree.get({node.depth - 1, node.parent_index}).name.string.data();
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
	SliderFloat("Scale", &scalar_scale, 0.001, 10, "%.3f", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat);
	transform.scale = f32x3(scalar_scale);

	End();
}

void Editor::mesh_settings_window()
{
	using namespace ImGui;

	Begin("Mesh Settings", nullptr, ImGuiWindowFlags_NoCollapse);

	static Name mesh_name;
	if (BeginCombo("Mesh", mesh_name.string.data()))
	{
		for (auto const & [name, _]: game.assets.meshes)
			if (Selectable(name.string.data()))
				mesh_name = name;

		EndCombo();
	}
	if (not game.assets.meshes.contains(mesh_name))
	{
		Text("Pick a mesh");
		End();
		return;
	}
	auto & mesh = game.assets.meshes.get(mesh_name);


	Spacing(), Separator(), Text("Drawables");

	if (mesh.drawables.empty())
	{
		Text("Mesh has no drawables");
		End();
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
				auto & attribute_mappings = game.assets.programs.get(GLTF::pbrMetallicRoughness_program_name).attribute_mappings;
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

		for (auto const &[key, data]: primitive.attributes)
		{
			TableNextColumn(), TextFMT("{}", key);
			TableNextColumn(), TextFMT("{}x{}", data.type, data.dimension);
			TableNextColumn(), TextFMT("{}", data.buffer.size / (data.type.size() * data.dimension));
		}
		EndTable();
	}


	Spacing(), Separator();
	LabelText("Material", "%s", drawable.named_material.name.string.data());

	End();
}

void Editor::material_settings_window()
{
	using namespace ImGui;

	Begin("Material Settings", nullptr, ImGuiWindowFlags_NoCollapse);

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
		End();
		return;
	}
	auto & material = game.assets.materials.get(material_name);
	auto & block = material->get_block();

	auto buffer = ByteBuffer(block.data_size);
	material->write_to_buffer(buffer.begin());

	bool edited = false;
	for (auto &[name, variable]: block.variables)
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

	End();
}

void Editor::texture_2ds_window()
{
	using namespace ImGui;

	Begin("Textures");

	static bool is_texture_changed = false;
	static Name texture_name;
	if (BeginCombo("Texture", texture_name.string.data()))
	{
		for (auto & [name, _]: game.assets.textures)
			if (Selectable(name.string.data()))
				texture_name = name, is_texture_changed = true;

		EndCombo();
	}
	if (auto it = game.assets.textures.find(texture_name); it != game.assets.textures.end())
	{
		auto & [_, texture] = *it;
		SameLine(), Text("(id %d)", texture.id);

		static bool is_level_changed = false;
		static i32 texture_levels = 0, current_level = 0;
		if (is_texture_changed)
		{
			current_level = 0;
			GL::glGetTextureParameteriv(texture.id, GL::GL_TEXTURE_IMMUTABLE_LEVELS, &texture_levels);

			GL::glDeleteTextures(1, &texture_view.id);
			GL::glGenTextures(1, &texture_view.id);
			GL::glTextureView(texture_view.id, GL::GL_TEXTURE_2D, texture.id, GL::GL_RGBA8, 0, texture_levels, 0, 1);

			is_texture_changed = false;
			is_level_changed = true;
		}

		if (SliderInt("Level", &current_level, 0, texture_levels - 1))
			is_level_changed = true;

		static f32x2 texture_size, view_size;
		if (is_level_changed)
		{
			GL::glTextureParameteri(texture_view.id, GL::GL_TEXTURE_BASE_LEVEL, current_level);

			GL::glGetTextureLevelParameterfv(texture_view.id, current_level, GL::GL_TEXTURE_WIDTH, &texture_size.x);
			GL::glGetTextureLevelParameterfv(texture_view.id, current_level, GL::GL_TEXTURE_HEIGHT, &texture_size.y);

			f32 const max_resolution = 240;
			view_size = max_resolution / glm::compMax(texture_size) * texture_size;

			is_level_changed = false;
		}
		LabelText("Resolution", "%d x %d", i32(texture_size.x), i32(texture_size.y));
		Image(
			reinterpret_cast<void*>(i64(texture_view.id)),
			{view_size.x, view_size.y}
		);
	}
	else
	{
		Text("Pick a texture");
	}

	End();
}

void Editor::texture_cubemaps_window()
{
	using namespace ImGui;

	Begin("Cubemaps");

	static bool is_cubemap_changed = false;
	static Name cubemap_name;
	if (BeginCombo("Cubemap", cubemap_name.string.data()))
	{
		for (auto & [name, _]: game.assets.texture_cubemaps)
			if (Selectable(name.string.data()))
				cubemap_name = name, is_cubemap_changed = true;

		EndCombo();
	}
	// TODO(bekorn): maybe rename Assets::texture_cubemaps -> Assets::cubemaps
	if (auto it = game.assets.texture_cubemaps.find(cubemap_name); it != game.assets.texture_cubemaps.end())
	{
		auto & [_, cubemap] = *it;
		SameLine(), Text("(id %d)", cubemap.id);

		static bool is_level_changed = false;
		static i32 cubemap_levels = 0, current_level = 0;
		if (is_cubemap_changed)
		{
			using namespace GL;

			current_level = 0;
			glGetTextureParameteriv(cubemap.id, GL_TEXTURE_IMMUTABLE_LEVELS, &cubemap_levels);

			is_cubemap_changed = false;
			is_level_changed = true;
		}

		if (SliderInt("Level", &current_level, 0, cubemap_levels - 1))
			is_level_changed = true;

		static f32x2 cubemap_size, view_size;
		if (is_level_changed)
		{
			using namespace GL;

			glDeleteTextures(1, &cubemap_view.id);
			glGenTextures(1, &cubemap_view.id);
			glTextureView(cubemap_view.id, GL_TEXTURE_CUBE_MAP, cubemap.id, GL_RGBA8, current_level, 1, 0, 6);

			cubemap_view.handle = glGetTextureHandleARB(cubemap_view.id);
			glMakeTextureHandleResidentARB(cubemap_view.handle);

			glGetTextureLevelParameterfv(cubemap_view.id, 0, GL_TEXTURE_WIDTH, &cubemap_size.x);
			glGetTextureLevelParameterfv(cubemap_view.id, 0, GL_TEXTURE_HEIGHT, &cubemap_size.y);

			f32 const max_resolution = 240;
			view_size = max_resolution / glm::compMax(cubemap_size) * cubemap_size;

			is_level_changed = false;
		}
		LabelText("Resolution", "%d x %d", i32(cubemap_size.x), i32(cubemap_size.y));
		Image(
			reinterpret_cast<void*>(i64(cubemap_framebuffer_color_attachment.id)),
			{view_size.x, view_size.y},
			{0, 1}, {1, 0}
		);
	}
	else
	{
		Text("Pick a cubemap");
	}

	End();
}

void Editor::program_window()
{
	using namespace ImGui;

	if (has_program_errors)
	{
		PushStyleColor(ImGuiCol_Tab, {0.8, 0, 0, 1});
		PushStyleColor(ImGuiCol_TabActive, {1, 0, 0, 1});
		PushStyleColor(ImGuiCol_TabHovered, {1, 0, 0, 1});
	}
	Begin("Program Info", nullptr);
	if (has_program_errors)
		PopStyleColor(3);

	static Name program_name;
	static Assets * assets = &game.assets;
	if (BeginCombo("Program", program_name.string.data()))
	{
		Selectable("Game", false, ImGuiSelectableFlags_Disabled);
		for (auto const & [name, _]: game.assets.programs)
		{
			if (Selectable(name.string.data()))
				program_name = name, assets = &game.assets;

			if (game.assets.program_errors.contains(name))
				SameLine(), TextColored({1, 0, 0, 1}, "ERROR");
		}

		Selectable("Editor", false, ImGuiSelectableFlags_Disabled);
		for (auto const & [name, _]: editor_assets.programs)
		{
			if (Selectable(name.string.data()))
				program_name = name, assets = &editor_assets;

			if (editor_assets.program_errors.contains(name))
				SameLine(), TextColored({1, 0, 0, 1}, "ERROR");
		}

		EndCombo();
	}
	if (not assets->programs.contains(program_name))
	{
		Text("Pick a program");
		End();
		return;
	}
	auto named_program = assets->programs.get_named(program_name);

	SameLine(), Text("(id: %d)", named_program.data.id);

	if (Button("Reload"))
		if (not assets->reload_glsl_program(named_program.name))
			should_game_render = false;

	if (auto iter = assets->program_errors.find(program_name); iter != assets->program_errors.end())
	{
		TextColored({1, 0, 0, 1}, "%s", iter->second.data());
		End();
		return;
	}

	if (BeginTable("attribute_mappings", 4, ImGuiTableFlags_BordersInnerH))
	{
		TableSetupColumn("Attribute");
		TableSetupColumn("Per Patch", ImGuiTableColumnFlags_WidthFixed);
		TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 100);
		TableSetupColumn("Location", ImGuiTableColumnFlags_WidthFixed);
		TableHeadersRow();

		for (auto const & attribute : named_program.data.attribute_mappings)
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

		for (auto const & uniform : named_program.data.uniform_mappings)
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

		for (auto const & uniform_block : named_program.data.uniform_block_mappings)
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

				for (auto const & variable : uniform_block.variables)
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

		for (auto const & storage_block : named_program.data.storage_block_mappings)
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

				for (auto const & variable : storage_block.variables)
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

	End();
}

void Editor::uniform_buffer_window()
{
	using namespace ImGui;

	Begin("Uniform Buffer Info", nullptr);

	static Name uniform_buffer_name;
	if (BeginCombo("Uniform Buffer", uniform_buffer_name.string.data()))
	{
		for (auto const & [name, _]: game.assets.uniform_blocks)
			if (Selectable(name.string.data()))
				uniform_buffer_name = name;

		EndCombo();
	}
	if (not game.assets.uniform_blocks.contains(uniform_buffer_name))
	{
		Text("Pick a uniform buffer");
		End();
		return;
	}
	auto named_uniform_buffer = game.assets.uniform_blocks.get_named(uniform_buffer_name);
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

	End();
}

void Editor::camera_window()
{
	using namespace ImGui;

	Begin("Camera Info", nullptr, ImGuiWindowFlags_NoCollapse);

	if (std::holds_alternative<PerspectiveCamera>(game.camera))
	{
		auto & camera = std::get<PerspectiveCamera>(game.camera);
		Text("Perspective camera");
		DragFloat3("Position", begin(camera.position));
		DragFloat3("Up", begin(camera.up));
		DragFloat3("Target", begin(camera.target));
		DragFloat("FOV", &camera.fov);
		DragFloat("Near", &camera.near);
		DragFloat("Far", &camera.far);
	}
	else if (std::holds_alternative<OrthographicCamera>(game.camera))
	{
		auto & camera = std::get<OrthographicCamera>(game.camera);
		Text("Orthographic camera");
		DragFloat3("Position", begin(camera.position));
		DragFloat3("Up", begin(camera.up));
		DragFloat3("Target", begin(camera.target));
		DragFloat("Left", &camera.left);
		DragFloat("Right", &camera.right);
		DragFloat("Bottom", &camera.bottom);
		DragFloat("Top", &camera.top);
	}

	End();
}

void Editor::workspaces()
{
	using namespace ImGui;

	// see imgui_demo.cpp ShowExampleAppDockSpace function
	auto * viewport = GetMainViewport();
	SetNextWindowPos(viewport->WorkPos);
	SetNextWindowSize(viewport->WorkSize);
	SetNextWindowViewport(viewport->ID);
	PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
	PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
	Begin(
		"MainWindow", nullptr,
		ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus
		| ImGuiWindowFlags_NoNavFocus
	);
	ImGui::PopStyleVar(2);


	BeginTabBar("Workspaces");

	struct Workspace
	{
		const char * name;
		ImGuiID id;

		explicit Workspace(const char* name):
			name(name),
			id(GetID(name))
		{}
	};

	static array workspaces = {
		Workspace("Workspace 1"),
		Workspace("Workspace 2"),
		Workspace("Workspace 3"),
	};

	for (auto & workspace: workspaces)
	{
		if (BeginTabItem(workspace.name))
			DockSpace(workspace.id), EndTabItem();
		else
			DockSpace(workspace.id, {0, 0}, ImGuiDockNodeFlags_KeepAliveOnly);
	}

	EndTabBar();

	End();
}

void Editor::update(GLFW::Window const & window, FrameInfo const & frame_info, f64 game_update_in_seconds, f64 game_render_in_seconds)
{
	has_program_errors = not editor_assets.program_errors.empty() or not game.assets.program_errors.empty();

	workspaces();

	// TODO(bekorn): these windows are monolithic right now,
	//  it should be similar to Blender's, each workspace can have arbitrary windows with dynamic types
	metrics_window(frame_info, game_update_in_seconds, game_render_in_seconds);
	game_window();
	game_settings_window();
	node_settings_window();
	mesh_settings_window();
	material_settings_window();
	texture_2ds_window();
	texture_cubemaps_window();
	uniform_buffer_window();
	program_window();
	camera_window();

	//	ImGui::ShowDemoWindow();
}

void Editor::render(GLFW::Window const & window, FrameInfo const & frame_info)
{
	using namespace GL;

	// Render cubemap framebuffer
	{
		glBindFramebuffer(GL_FRAMEBUFFER, cubemap_framebuffer.id);
		glViewport(0, 0, 240, 240);

		auto & environment_map_program = game.assets.programs.get("environment_map_pipe");
		glUseProgram(environment_map_program.id);
		glUniformHandleui64ARB(
			GetLocation(environment_map_program.uniform_mappings, "environment_map"),
			cubemap_view.handle
		);
		auto view = visit([](Camera auto & c) { return c.get_view(); }, game.camera);
		auto projection = visit([](Camera auto & c) { return c.get_projection(); }, game.camera);
		auto view_projection = projection * view;
		auto invVP = glm::inverse(f32x3x3(view_projection));
		f32x4x3 view_dirs;
		view_dirs[0] = invVP * f32x3(-1, -1, 1); // left   bottom
		view_dirs[1] = invVP * f32x3(+1, -1, 1); // right  bottom
		view_dirs[2] = invVP * f32x3(-1, +1, 1); // left   up
		view_dirs[3] = invVP * f32x3(+1, +1, 1); // right  up
		glUniformMatrix4x3fv(
			GetLocation(environment_map_program.uniform_mappings, "view_directions"),
			1, false, begin(view_dirs)
		);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		// gamma correction
		{
			auto & gamma_correction_program = game.assets.programs.get("gamma_correction_pipe"_name);
			glUseProgram(gamma_correction_program.id);
			glUniformHandleui64ARB(
				GetLocation(gamma_correction_program.uniform_mappings, "color_attachment"),
				cubemap_framebuffer_color_attachment.handle
			);
			glDrawArrays(GL_TRIANGLES, 0, 3);
		}
	}

	// if game is not rendering, editor should not as well
	if (not should_game_render)
		return;

	// Draw gizmos
	{
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.id);
		glViewport(0, 0, resolution.x, resolution.y);
		glColorMask(true, true, true, true), glDepthMask(true);
		const f32 clear_depth{1};
		glClearNamedFramebufferfv(framebuffer.id, GL_DEPTH, 0, &clear_depth);
		const f32x4 clear_color{0, 0, 0, 0};
		glClearNamedFramebufferfv(framebuffer.id, GL_COLOR, 0, begin(clear_color));

		glEnable(GL_CULL_FACE), glCullFace(GL_BACK);
		glEnable(GL_DEPTH_TEST), glDepthFunc(GL_LESS);

		auto & gizmo_program = editor_assets.programs.get("gizmo"_name);
		glUseProgram(gizmo_program.id);

		auto size = glm::max(50.f, 0.1f * glm::min(resolution.x, resolution.y));
		auto const padding = 8;
		glViewport(resolution.x - size - padding, resolution.y - size - padding, size, size);

		auto view = visit([](Camera auto & c){ return glm::lookAt(f32x3(0), c.target - c.position, c.up); }, game.camera);
		auto proj = glm::ortho<f32>(-1, +1, -1, +1, -1, +1);
		auto transform = proj * view;

		auto location_TransformMV = GetLocation(gizmo_program.uniform_mappings, "transform");
		glUniformMatrix4fv(location_TransformMV, 1, false, begin(transform));

		for (auto & drawable : editor_assets.meshes.get("AxisGizmo:mesh:0:Cube"_name).drawables)
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
		auto & gamma_correction_program = game.assets.programs.get("gamma_correction_pipe"_name);
		glUseProgram(gamma_correction_program.id);
		glUniformHandleui64ARB(
			GetLocation(gamma_correction_program.uniform_mappings, "color_attachment"),
			framebuffer_color_attachment.handle
		);
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}
}
