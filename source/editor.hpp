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

		End();
	}

	void mesh_settings_window()
	{
		using namespace ImGui;

		Begin("Mesh Settings", nullptr, ImGuiWindowFlags_NoCollapse);

		static Name mesh_name;
		if (BeginCombo("Mesh", mesh_name.string.data()))
		{
			// TODO(bekorn): remove .resources access, Managed should provide an API for this
			for (auto const & [name, _]: assets.meshes.resources)
				if (Selectable(name.string.data()))
					mesh_name = name;

			EndCombo();
		}
		if (not assets.meshes.resources.contains(mesh_name))
		{
			Text("Pick a mesh");
			End();
			return;
		}
		auto & mesh = assets.meshes.get(mesh_name);


		Spacing(), Separator(), Text("Transform");

		SliderFloat3("Position", begin(mesh.position), -2, 2, "%.2f");
		auto rotation_in_degrees = glm::degrees(mesh.rotation);
		SliderFloat3("Rotation", begin(rotation_in_degrees), 0, 360, "%.2f");
		mesh.rotation = glm::radians(rotation_in_degrees);
		SliderFloat("Scale", &mesh.scale, 0.001, 10, "%.3f", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat);


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
		}

		{
			bool any_vertex_array_loaded = false;
			for (auto const & drawable: mesh.drawables)
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


		Spacing(), Separator(), Text("Material");

		LabelText("Name", "%s", drawable.named_material.name.string.data());

		auto & material = drawable.named_material.data;

		ColorEdit4("Base Color", begin(
			dynamic_cast<Render::Material_gltf_pbrMetallicRoughness*>(material.get())->base_color_factor
		));

		End();
	}

	void textures_window()
	{
		using namespace ImGui;

		Begin("Textures");

		static Name texture_name;
		if (BeginCombo("Texture", texture_name.string.data()))
		{
			// TODO(bekorn): remove .resources access, Managed should provide an API for this
			for (auto & [name, _]: assets.textures.resources)
				if (Selectable(name.string.data()))
					texture_name = name;

			EndCombo();
		}
		if (auto maybe = assets.textures.find(texture_name))
		{
			auto & texture = *maybe;
			SameLine(), Text("(id %d)", texture.id);

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

		Begin("Program Info", nullptr, ImGuiWindowFlags_NoCollapse);

		static Name program_name;
		if (BeginCombo("Program", program_name.string.data()))
		{
			// TODO(bekorn): remove .resources access, Managed should provide an API for this
			for (auto const & [name, _]: assets.programs.resources)
				if (Selectable(name.string.data()))
					program_name = name;

			EndCombo();
		}
		if (not assets.programs.resources.contains(program_name))
		{
			Text("Pick a program");
			End();
			return;
		}
		auto named_program = assets.programs.get_named(program_name);

		SameLine(), Text("(id: %d)", named_program.data.id);

		if (Button("Reload"))
			assets.load_glsl(named_program.name);

		if (auto const & error = assets.program_errors.get(program_name); not error.empty())
			TextColored({255, 0, 0, 255}, "%s", error.data());

		if (BeginTable("uniform_mappings", 3, ImGuiTableFlags_BordersInnerH))
		{
			TableSetupColumn("Uniform Location"), TableSetupColumn("Type"), TableSetupColumn("Name");
			TableHeadersRow();

			for (auto const & mapping : named_program.data.uniform_mappings)
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

			for (auto const & mapping : named_program.data.attribute_mappings)
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
		textures_window();
		program_window();
		camera_window();
	}
};
