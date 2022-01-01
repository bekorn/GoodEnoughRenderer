#pragma once

#include <sstream>

#include "Lib/opengl/.hpp"
#include "Lib/render/.hpp"
#include "Lib/imgui/.hpp"

#include "globals.hpp"
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

		SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Once);
		Begin("Metrics", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

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

		i32 current_program;
		GL::glGetIntegerv(GL::GL_CURRENT_PROGRAM, &current_program);
		Text("Current Program id: %d", current_program);

		End();
	}

	void mesh_settings_window()
	{
		using namespace ImGui;

		Begin("Mesh Settings", nullptr, ImGuiWindowFlags_NoCollapse);

		static u64 mesh_index;
		{
			static u64 min_index = 0, max_index;
			max_index = assets.meshes.size() - 1;
			SliderScalar("Mesh Index", ImGuiDataType_U64, &mesh_index, &min_index, &max_index);
		}

		auto & mesh = assets.meshes[mesh_index];

		LabelText("Name", "%s", mesh.name.c_str());

		Spacing(), Separator(), Text("Primitives");

		if (mesh.primitives.empty())
		{
			Text("Mesh has no primitives");
		}
		else
		{
			{
				vector<u32> vao_ids;
				for (auto & drawable: mesh.primitives)
					vao_ids.push_back(drawable.vertex_array.id);

				std::stringstream ss;
				for (auto id : vao_ids)
					ss << id << ", ";
				LabelText("VAO ids", "%s", ss.str().data());
			}

			{
				bool any_vertex_array_loaded = false;
				for (auto const & primitive: mesh.primitives)
					any_vertex_array_loaded |= primitive.is_loaded();

				if (any_vertex_array_loaded)
				{
					if (Button("Unload all primitives"))
						for (auto & primitive: mesh.primitives)
							primitive.unload();
				}
				else
				{
					if (Button("Load all primitives"))
						for (auto & primitive: mesh.primitives)
							primitive.load(assets.program);
				}
			}


			static u64 primitive_index;
			{
				static u64 min_index = 0, max_index;
				max_index = mesh.primitives.size() - 1;
				SliderScalar("Primitive Index", ImGuiDataType_U64, &primitive_index, &min_index, &max_index);
			}

			auto const & primitive = *mesh.primitives[primitive_index].primitive_ptr;

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
		}

		Spacing(), Separator(), Text("Transform");

		SliderFloat3("Position", begin(mesh.position), -2, 2, "%.2f");
		SliderFloat3("Rotation", begin(mesh.rotation), 0, 360, "%.2f");
		SliderFloat("Scale", &mesh.scale, 0.001, 10, "%.2f");

		Spacing(), Separator(), Text("Material");

		vector<u32> material_indices;
		for (auto & drawable: mesh.primitives)
			material_indices.push_back(drawable.material_ptr.index);

		static i32 material_index;
		{
			std::stringstream ss;
			for (auto & i: material_indices)
				ss << i << '\0';
			Combo("Material Index", &material_index, ss.str().c_str());
		}

		auto const & material = assets.materials[material_index];

		ColorEdit4("Base Color", begin(
			dynamic_cast<Render::Material_gltf_pbrMetallicRoughness*>(material.get())->base_color_factor
		));

		End();
	}

	void assets_window()
	{
		using namespace ImGui;

		Begin("Assets");

		static u64 texture_index = 0, min_index = 0;
		u64 const max_index = assets.textures.size() - 1;

		SliderScalar("Index", ImGuiDataType_U64, &texture_index, &min_index, &max_index);

		auto const id = assets.textures[texture_index].id;
		LabelText("id", "%d", id);

		Image(
			reinterpret_cast<void*>(i64(id)),
			{240, 240}
		);

		End();
	}

	std::string program_error;

	void shader_window()
	{
		using namespace ImGui;

		Begin("Shader Info", nullptr, ImGuiWindowFlags_NoCollapse);

		if (Button("Reload Shader"))
			if (auto error = assets.load_program())
				program_error = error.value();

		if (not program_error.empty())
			SameLine(), TextColored({255, 0, 0, 255}, "%s", program_error.data());

		auto const & program = assets.program;

		if (BeginTable("uniform_mappings", 3, ImGuiTableFlags_BordersInnerH))
		{
			TableSetupColumn("Uniform Location"), TableSetupColumn("Type"), TableSetupColumn("Name");
			TableHeadersRow();

			for (auto const & mapping : program.uniform_mappings)
			{
				TableNextRow();
				TableNextColumn();
				Text("%d", mapping.location);
				TableNextColumn();
				Text("%s", GL::GLSLTypeToString(mapping.glsl_type).data());
				TableNextColumn();
				Text("%s", mapping.key.data());
			}
			EndTable();
		}

		if (BeginTable("attribute_mappings", 3, ImGuiTableFlags_BordersInnerH))
		{
			TableSetupColumn("Attribute Location"), TableSetupColumn("Type"), TableSetupColumn("Name");
			TableHeadersRow();

			for (auto const & mapping : program.attribute_mappings)
			{
				TableNextRow();
				TableNextColumn();
				Text("%d", mapping.location);
				TableNextColumn();
				Text("%s", GL::GLSLTypeToString(mapping.glsl_type).data());
				TableNextColumn();
				Text("%s", (std::stringstream() << mapping.key).str().data());
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

	void create() final
	{
	}

	void render(GLFW::Window const & window, FrameInfo const & frame_data) final
	{
		metrics_window(frame_data);
		game_window();
		game_settings_window();
		mesh_settings_window();
		assets_window();
		shader_window();
		camera_window();
	}
};
