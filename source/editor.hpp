#pragma once

#include <sstream>

#include "Lib/opengl/.hpp"
#include "Lib/render/.hpp"
#include "Lib/imgui/.hpp"

#include "renderer.hpp"
#include "game.hpp"

struct Editor final : IRenderer
{
	Assets & assets;
	Game & game;

	explicit Editor(Assets & assets, Game & game) :
		assets(assets), game(game)
	{}

	void metrics_window(FrameInfo const & frame_info)
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

			Text("Average frame takes %-6.4f ms, (%06.1f fps)", average_delta, 1. / average_delta);
		}

		Text("Frame: %06llu, Time: %g", frame_info.idx, frame_info.seconds_since_start);

		End();
	}

	void game_window()
	{
		using namespace ImGui;

		Begin("Game", nullptr, ImGuiWindowFlags_NoCollapse);

		f32 scale = 0.5;
		Text("Resolution: %dx%d, Scale: %.2f", game.resolution.x, game.resolution.y, scale);
		Image(
			reinterpret_cast<void*>(i64(game.framebuffer_attachments[0].id)),
			{f32(game.resolution.x) * scale, f32(game.resolution.y) * scale},
			{0, 1}, {1, 0} // because default is flipped on y-axis
		);

		End();
	}

	void game_settings_window()
	{
		using namespace ImGui;

		Begin("Game Settings", nullptr, ImGuiWindowFlags_NoCollapse);

		ColorEdit3("Clear color", begin(game.clear_color));

		End();
	}

	void node_settings_window()
	{
		using namespace ImGui;

		Begin("Node Settings", nullptr, ImGuiWindowFlags_NoCollapse);

		static Name node_name;
		bool node_changed = false;
		if (BeginCombo("Node", node_name.string.data()))
		{
			auto indent = ImGui::GetStyle().IndentSpacing;
			for (auto & node: assets.scene_tree.depth_first())
			{
				if (node.depth) Indent(indent * node.depth);

				if (Selectable(node.name.string.data()))
					node_name = node.name, node_changed = true;

				if (node.depth) ImGui::Unindent(indent * node.depth);
			}

			EndCombo();
		}
		if (not assets.scene_tree.named_indices.contains(node_name))
		{
			Text("Pick a node");
			End();
			return;
		}
		auto & node_index = assets.scene_tree.named_indices.get(node_name);
		auto & node = assets.scene_tree.get(node_index);


		auto const & parent_name = node.depth == 0
								   ? "-"
								   : assets.scene_tree.get({node.depth - 1, node.parent_index}).name.string.data();
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

	void mesh_settings_window()
	{
		using namespace ImGui;

		Begin("Mesh Settings", nullptr, ImGuiWindowFlags_NoCollapse);

		static Name mesh_name;
		if (BeginCombo("Mesh", mesh_name.string.data()))
		{
			for (auto const & [name, _]: assets.meshes)
				if (Selectable(name.string.data()))
					mesh_name = name;

			EndCombo();
		}
		if (not assets.meshes.contains(mesh_name))
		{
			Text("Pick a mesh");
			End();
			return;
		}
		auto & mesh = assets.meshes.get(mesh_name);


		Spacing(), Separator(), Text("Drawables");

		if (mesh.drawables.empty())
		{
			Text("Mesh has no drawables");
			End();
			return;
		}

		{
			std::stringstream ss;
			for (auto & drawable: mesh.drawables)
				ss << drawable.vertex_array.id << ", ";
			LabelText("VertexArrays", "%s", ss.str().data());


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
					for (auto & drawable: mesh.drawables)
						drawable.load(assets.programs.get(GLTF::pbrMetallicRoughness_program_name));
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

			std::ostringstream ss;
			for (auto const &[key, data]: primitive.attributes)
			{
				TableNextRow();

				TableNextColumn(); ss << key;
				TextUnformatted(ss.str().data()), ss.str({});

				TableNextColumn(); ss << data.type << 'x' << (u32)data.dimension;
				TextUnformatted(ss.str().data()), ss.str({});

				TableNextColumn(); ss << data.buffer.size / (data.type.size() * data.dimension);
				TextUnformatted(ss.str().data()), ss.str({});
			}
			EndTable();
		}


		Spacing(), Separator();
		LabelText("Material", "%s", drawable.named_material.name.string.data());

		End();
	}

	void material_settings_window()
	{
		using namespace ImGui;

		Begin("Material Settings", nullptr, ImGuiWindowFlags_NoCollapse);

		static Name material_name;
		if (BeginCombo("Material", material_name.string.data()))
		{
			for (auto const & [name, _]: assets.materials)
				if (Selectable(name.string.data()))
					material_name = name;

			EndCombo();
		}
		if (not assets.materials.contains(material_name))
		{
			Text("Pick a material");
			End();
			return;
		}
		auto & material = assets.materials.get(material_name);
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

	void textures_window()
	{
		using namespace ImGui;

		Begin("Textures");

		static Name texture_name;
		if (BeginCombo("Texture", texture_name.string.data()))
		{
			for (auto & [name, _]: assets.textures)
				if (Selectable(name.string.data()))
					texture_name = name;

			EndCombo();
		}
		if (auto it = assets.textures.find(texture_name); it != assets.textures.end())
		{
			auto & [_, texture] = *it;
			SameLine(), Text("(id %d)", texture.id);
			LabelText("Handle", "%llu", texture.handle);

			Image(
				reinterpret_cast<void*>(i64(texture.id)),
				{240, 240}
			);
		}
		else
		{
			Text("Pick a texture");
		}

		End();
	}

	void program_window()
	{
		using namespace ImGui;

		Begin("Program Info", nullptr);

		static Name program_name;
		if (BeginCombo("Program", program_name.string.data()))
		{
			for (auto const & [name, _]: assets.programs)
				if (Selectable(name.string.data()))
					program_name = name;

			EndCombo();
		}
		if (not assets.programs.contains(program_name))
		{
			Text("Pick a program");
			End();
			return;
		}
		auto named_program = assets.programs.get_named(program_name);

		SameLine(), Text("(id: %d)", named_program.data.id);

		if (Button("Reload"))
			assets.load_glsl_program(named_program.name);

		if (auto const & error = assets.program_errors.get(program_name); not error.empty())
			TextColored({255, 0, 0, 255}, "%s", error.data());

		if (BeginTable("attribute_mappings", 4, ImGuiTableFlags_BordersInnerH))
		{
			TableSetupColumn("Attribute");
			TableSetupColumn("Per Patch", ImGuiTableColumnFlags_WidthFixed);
			TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 100);
			TableSetupColumn("Location", ImGuiTableColumnFlags_WidthFixed);
			TableHeadersRow();

			for (auto const & attribute : named_program.data.attribute_mappings)
			{
				TableNextRow();
				TableNextColumn();
				Text("%s", (std::stringstream() << attribute.key).str().data());
				TableNextColumn();
				Text("%s", attribute.per_patch ? "true" : "false");
				TableNextColumn();
				Text("%s", GL::GLSLTypeToString(attribute.glsl_type).data());
				TableNextColumn();
				Text("%d", attribute.location);
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
				TableNextRow();
				TableNextColumn();
				Text("%s", uniform.key.data());
				TableNextColumn();
				Text("%s", GL::GLSLTypeToString(uniform.glsl_type).data());
				TableNextColumn();
				Text("%d", uniform.location);
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
				TableNextRow();
				TableNextColumn();
				if (BeginTable("uniform_block", 3, ImGuiTableFlags_BordersInnerH))
				{
					TableSetupColumn("Name");
					TableSetupColumn("Binding", ImGuiTableColumnFlags_WidthFixed);
					TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed);
					TableHeadersRow();

					TableNextRow();
					TableNextColumn();
					Text("%s", uniform_block.key.data());
					TableNextColumn();
					Text("%d", uniform_block.location);
					TableNextColumn();
					Text("%d", uniform_block.data_size);

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
						TableNextRow();
						TableNextColumn();
						Text("%d", variable.offset);
						TableNextColumn();
						Text("%s", GL::GLSLTypeToString(variable.glsl_type).data());
						TableNextColumn();
						Text("%s", variable.key.data());
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
				TableNextRow();
				TableNextColumn();
				if (BeginTable("storage_block", 3, ImGuiTableFlags_BordersInnerH))
				{
					TableSetupColumn("Name");
					TableSetupColumn("Binding", ImGuiTableColumnFlags_WidthFixed);
					TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed);
					TableHeadersRow();

					TableNextRow();
					TableNextColumn();
					Text("%s", storage_block.key.data());
					TableNextColumn();
					Text("%d", storage_block.location);
					TableNextColumn();
					Text("%d", storage_block.data_size);

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
						TableNextRow();
						TableNextColumn();
						Text("%d", variable.offset);
						TableNextColumn();
						Text("%s", GL::GLSLTypeToString(variable.glsl_type).data());
						TableNextColumn();
						Text("%s", variable.key.data());
					}
					EndTable();
				}
			}
			EndTable();
		}

		End();
	}

	void uniform_buffer_window()
	{
		using namespace ImGui;

		Begin("Uniform Buffer Info", nullptr);

		static Name uniform_buffer_name;
		if (BeginCombo("Uniform Buffer", uniform_buffer_name.string.data()))
		{
			for (auto const & [name, _]: assets.uniform_blocks)
				if (Selectable(name.string.data()))
					uniform_buffer_name = name;

			EndCombo();
		}
		if (not assets.uniform_blocks.contains(uniform_buffer_name))
		{
			Text("Pick a uniform buffer");
			End();
			return;
		}
		auto named_uniform_buffer = assets.uniform_blocks.get_named(uniform_buffer_name);
		auto & uniform_buffer = named_uniform_buffer.data;

		if (Button("Reload"))
			assets.load_glsl_uniform_block(named_uniform_buffer.name);

		if (auto const & error = assets.program_errors.get(named_uniform_buffer.name); not error.empty())
			TextColored({255, 0, 0, 255}, "%s", error.data());

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
				TableNextRow();
				TableNextColumn();
				Text("%s", key.data());
				TableNextColumn();
				Text("%d", variable.offset);
				TableNextColumn();
				Text("%s", GL::GLSLTypeToString(variable.glsl_type).data());
			}
			EndTable();
		}

		End();
	}

	void camera_window()
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

	void workspaces()
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

	void create() final
	{
		// Enable docking
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

		// Table styling
		ImGui::GetStyle().CellPadding.x = 6;
	}

	void render(GLFW::Window const & window, FrameInfo const & frame_info) final
	{
		workspaces();

		// TODO(bekorn): these windows are monolithic right now,
		//  it should be similar to Blender's, each workspace can have arbitrary windows with dynamic types
		metrics_window(frame_info);
		game_window();
		game_settings_window();
		node_settings_window();
		mesh_settings_window();
		material_settings_window();
		textures_window();
		program_window();
		uniform_buffer_window();
		camera_window();

//		ImGui::ShowDemoWindow();
	}
};
