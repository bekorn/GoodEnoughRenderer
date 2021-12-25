#pragma once

#include <sstream>

#include "Lib/opengl/.hpp"
#include "Lib/render/.hpp"
#include "Lib/imgui/.hpp"

#include "globals.hpp"
#include "renderer.hpp"
#include "game.hpp"

struct Editor : IRenderer
{
	Game & game;

	explicit Editor(Game & game) :
		game(game)
	{}

	std::string status;

	void metrics_window()
	{
		using namespace ImGui;

		SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Once);
		Begin("Metrics", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

		Text(
			"Application average %.3f ms/frame (%.1f FPS)",
			1000.f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate
		);

		Separator();

		Text("|> %s", status.c_str());

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

	bool visualize_attribute = false;
	bool visualize_texture = false;

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
			max_index = game.meshes.size() - 1;
			SliderScalar("Mesh Index", ImGuiDataType_U64, &mesh_index, &min_index, &max_index);
		}

		auto & mesh = game.meshes[mesh_index];

		LabelText("Name", "%s", mesh.name.c_str());

		vector<u32> vao_ids;
		for (auto & drawable: mesh.array_drawables)
			vao_ids.push_back(drawable.vao.id);
		for (auto & drawable: mesh.element_drawables)
			vao_ids.push_back(drawable.vao.id);
		for (auto & drawable: mesh.primitives)
			vao_ids.push_back(drawable.vertex_array.id);

		{
			std::stringstream ss;
			for (auto id : vao_ids)
				ss << id << ", ";
			LabelText("VAO ids", "%s", ss.str().data());
		}

		if (not mesh.primitives.empty())
		{
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
							primitive.load(program);
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


		NewLine();
		BulletText("Transform");
		SliderFloat3("Position", begin(mesh.position), -2, 2, "%.2f");
		SliderFloat3("Rotation", begin(mesh.rotation), 0, 360, "%.2f");
		SliderFloat("Scale", &mesh.scale, 0.001, 10, "%.2f");

		NewLine();
		BulletText("Material");

		vector<u32> material_indices;
		for (auto & drawable: mesh.array_drawables)
			material_indices.push_back(drawable.material_ptr.index);
		for (auto & drawable: mesh.element_drawables)
			material_indices.push_back(drawable.material_ptr.index);
		for (auto & drawable: mesh.primitives)
			material_indices.push_back(drawable.material_ptr.index);

		static i32 material_index;
		{
			std::stringstream ss;
			for (auto & i: material_indices)
				ss << i << '\0';
			Combo("Material Index", &material_index, ss.str().c_str());
		}

		auto const & material = game.materials[material_index];

		ColorEdit4("Base Color", begin(
			dynamic_cast<Render::Material_gltf_pbrMetallicRoughness*>(material.get())->base_color_factor
		));

		{
			static i32 current_item = 0;
			if (Checkbox("Visualize Attribute of Mesh", &visualize_attribute))
			{
				GL::glUseProgram(visualize_attribute ? attribute_visualizers[current_item].id : program.id);
			}

			if (visualize_attribute)
			{
				static auto const elements = []()
				{
					array<char const*, GL::ATTRIBUTE_LOCATION::SIZE> array;
					for (auto i = 0; i < GL::ATTRIBUTE_LOCATION::SIZE; ++i)
						array[i] = GL::ATTRIBUTE_LOCATION::ToString(i);
					return array;
				}();

				if (Combo("Attribute", &current_item, elements.data(), elements.size()))
				{
					GL::glUseProgram(attribute_visualizers[current_item].id);
				}
			}
		}

		End();
	}

	void assets_window()
	{
		using namespace ImGui;

		Begin("Assets");

		static u64 texture_index = 0, min_index = 0;
		u64 const max_index = game.textures.size() - 1;

		SliderScalar("Index", ImGuiDataType_U64, &texture_index, &min_index, &max_index);

		auto const id = game.textures[texture_index].id;
		LabelText("id", "%d", id);

		Image(
			reinterpret_cast<void*>(i64(id)),
			{240, 240}
		);

		End();
	}

	array<GL::ShaderProgram, GL::ATTRIBUTE_LOCATION::SIZE> attribute_visualizers;

	void load_attribute_visualizers()
	{
		// Limitation: only works with vectors
		auto const vert_source = File::LoadAsString(global_state.test_assets / "debug/single_attributes.vert.glsl");
		auto const frag_source = File::LoadAsString(global_state.test_assets / "debug/single_attribute.frag.glsl");

		GL::ShaderStage frag_shader;
		frag_shader.create(
			{
				.stage = GL::GL_FRAGMENT_SHADER,
				.sources = {
					GL::GLSL_VERSION_MACRO.c_str(),
					frag_source.c_str(),
				},
			}
		);

		for (auto i = 0; i < attribute_visualizers.size(); ++i)
		{
			auto const attribute_location = "#define ATTRIBUTE_LOCATION_VISUALIZE " + std::to_string(i) + "\n";

			GL::ShaderStage vert_shader;
			vert_shader.create(
				{
					.stage = GL::GL_VERTEX_SHADER,
					.sources = {
						GL::GLSL_VERSION_MACRO.c_str(),
						GL::GLSL_ATTRIBUTE_LOCATION_MACROS.c_str(),
						attribute_location.c_str(),
						vert_source.c_str(),
					},
				}
			);

			attribute_visualizers[i].create(
				{
					.shader_stages = {
						&vert_shader,
						&frag_shader
					}
				}
			);
		}
	}

	GL::ShaderProgram program;
	std::string program_info;

	void try_to_reload_shader()
	{
		program_info.clear();

		auto const vert_source = File::LoadAsString(global_state.test_assets / "gltf_pbrMetallicRoughness.vert.glsl");
		auto const frag_source = File::LoadAsString(global_state.test_assets / "gltf_pbrMetallicRoughness.frag.glsl");

		bool shader_stage_error = false;

		GL::ShaderStage vert_shader;
		vert_shader.create(
			{
				.stage = GL::GL_VERTEX_SHADER,
				.sources = {
					GL::GLSL_VERSION_MACRO.c_str(),
					GL::GLSL_ATTRIBUTE_LOCATION_MACROS.c_str(),
					vert_source.c_str(),
				},
			}
		);
		if (not vert_shader.is_compiled())
		{
			shader_stage_error = true;
			program_info += "Vert Shader: " + vert_shader.get_log();
		}

		GL::ShaderStage frag_shader;
		frag_shader.create(
			{
				.stage = GL::GL_FRAGMENT_SHADER,
				.sources = {
					GL::GLSL_VERSION_MACRO.c_str(),
					GL::GLSL_ATTRIBUTE_LOCATION_MACROS.c_str(),
					frag_source.c_str(),
				},
			}
		);
		if (not frag_shader.is_compiled())
		{
			shader_stage_error = true;
			program_info += "Frag Shader: " + frag_shader.get_log();
		}

		if (shader_stage_error)
		{
			program_info = "Compilation failed! " + program_info;
			return;
		}


		GL::ShaderProgram new_program;
		new_program.create(
			{
				.shader_stages = {
					&vert_shader,
					&frag_shader
				}
			}
		);

		if (not new_program.is_linked())
		{
			program_info = "Compilation failed! Linking Error:\n" + new_program.get_log();
			return;
		}

		program = move(new_program);

		program.update_interface_mapping();

		GL::glUseProgram(program.id);
	}

	void shader_window()
	{
		using namespace ImGui;

		Begin("Shader Info", nullptr, ImGuiWindowFlags_NoCollapse);

		if (Button("Reload Shader"))
			try_to_reload_shader();

		if (not program_info.empty())
			SameLine(), TextColored({255, 0, 0, 255}, "%s", program_info.c_str());

		if (BeginTable("uniform_mappings", 3, ImGuiTableFlags_BordersInnerH))
		{
			TableSetupColumn("Uniform Location");
			TableSetupColumn("Type");
			TableSetupColumn("Name");
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
			TableSetupColumn("Attribute Location");
			TableSetupColumn("Type");
			TableSetupColumn("Name");
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

	void create() final
	{
		load_attribute_visualizers();
		try_to_reload_shader();
	}

	void render(GLFW::Window const & window) final
	{
		metrics_window();
		game_window();
		game_settings_window();
		mesh_settings_window();
		assets_window();
		shader_window();
	}
};
