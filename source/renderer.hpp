#pragma once

#include "Lib/glfw/window.hpp"
#include "Lib/opengl/.hpp"
#include "Lib/render/.hpp"

// TODO(bekorn): pass this without globals
struct GlobalState
{
	std::filesystem::path const test_assets = R"(E:\Users\Berk\Desktop\Projeler\Portfolio\GoodEnoughRenderer\test_assets)";
} global_state;

struct IRenderer
{
	virtual void create() = 0;

	virtual void render(GLFW::Window const & w) = 0;
};


struct MainRenderer : IRenderer
{
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

	vector<Mesh> meshes;

	f32x4 clear_color{0.45f, 0.55f, 0.60f, 1.00f};
	i32x2 scene_resolution{720, 720};
	GL::FrameBuffer scene_framebuffer;
	vector<GL::Texture2D> scene_framebuffer_attachments;

	void create_scene_framebuffer()
	{
		scene_framebuffer_attachments.resize(2);

		auto & color_attachment = scene_framebuffer_attachments[0];
		color_attachment.create(
			GL::Texture2D::AttachmentDescription{
				.size = scene_resolution,
				.internal_format = GL::GL_RGB,
				.format = GL::GL_RGBA,
			}
		);

		auto & depth_attachment = scene_framebuffer_attachments[1];
		depth_attachment.create(
			GL::Texture2D::AttachmentDescription{
				.size = scene_resolution,
				.internal_format = GL::GL_DEPTH_COMPONENT,
				.format = GL::GL_DEPTH_COMPONENT,
			}
		);

		scene_framebuffer.create(
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
	}

	void scene_render()
	{
		using namespace GL;

		glBindFramebuffer(GL_FRAMEBUFFER, scene_framebuffer.id);
		glViewport(0, 0, scene_resolution.x, scene_resolution.y);

		glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glEnable(GL_DEPTH_TEST);

		for (auto const & mesh: meshes)
		{
			for (auto const & [vao, material_index]: mesh.array_drawables)
			{
				mesh.materials[material_index]->set_uniforms();

				glBindVertexArray(vao.id);
				glDrawArrays(GL_TRIANGLES, 0, vao.vertex_count);
			}
			for (auto const & [vao, material_index]: mesh.element_drawables)
			{
				mesh.materials[material_index]->set_uniforms();

				glBindVertexArray(vao.id);
				glDrawElements(GL_TRIANGLES, vao.element_count, GL_UNSIGNED_SHORT, nullptr);
			}
		}
	}

	void scene_window()
	{
		using namespace ImGui;

		Begin("Scene");

		f32 scale = 0.5;
		Text("Resolution: %dx%d, Scale: %.2f", scene_resolution.x, scene_resolution.y, scale);
		Image(
			reinterpret_cast<void*>(i64(scene_framebuffer_attachments[0].id)),
			{f32(scene_resolution.x) * scale, f32(scene_resolution.y) * scale},
			{0, 1}, {1, 0} // because default is flipped on y-axis
		);

		End();
	}

	bool visualize_attribute = false;
	bool visualize_texture = false;

	void scene_settings_window()
	{
		using namespace ImGui;

		Begin("Scene Settings");

		ColorEdit3("Clear color", glm::value_ptr(clear_color));

		i32 current_program;
		GL::glGetIntegerv(GL::GL_CURRENT_PROGRAM, &current_program);
		Text("Current Program id: %d", current_program);

		ColorEdit4("Base Color", begin(
			dynamic_cast<Material_gltf_pbrMetallicRoughness*>(meshes[0].materials[0].get())->base_color_factor
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

		{
			static i32 current_item = 0;

			Checkbox("Visualize Texture of Mesh", &visualize_texture);

			if (visualize_texture)
			{
				auto const & mesh = meshes[0];

				static auto const elements = [&mesh]()
				{
					std::string elements_sep_by_zero;
					for (auto i = 0; i < mesh.textures.size(); ++i)
						elements_sep_by_zero += std::to_string(i) + '\0';
					return elements_sep_by_zero;
				}();

				Combo("index", &current_item, elements.data());

				auto const id = mesh.textures[current_item].id;
				LabelText("id", "%d", id);

				Image(
					reinterpret_cast<void*>(i64(id)),
					{240, 240}
				);
			}
		}

		End();
	}

	array<GL::ShaderProgram, GL::ATTRIBUTE_LOCATION::SIZE> attribute_visualizers;

	void load_attribute_visualizers()
	{
		// Limitation: only works with vectors
		auto const vert_source = LoadAsString(global_state.test_assets / "debug/single_attributes.vert.glsl");
		auto const frag_source = LoadAsString(global_state.test_assets / "debug/single_attribute.frag.glsl");

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

		auto const vert_source = LoadAsString(global_state.test_assets / "gltf_pbrMetallicRoughness.vert.glsl");
		auto const frag_source = LoadAsString(global_state.test_assets / "gltf_pbrMetallicRoughness.frag.glsl");

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

		// TODO(bekorn): This is probably not good.. Find a better wrapping for OpenGLObjects
		program.~ShaderProgram();
		program = move(new_program);

		program.update_interface_mapping();

		GL::glUseProgram(program.id);
	}

	void create() final
	{
		load_attribute_visualizers();

		create_scene_framebuffer();

//		auto const gltf_data = GLTF::Load(global_state.test_assets / "helmet/DamagedHelmet.gltf");
//		auto const gltf_data = GLTF::Load(global_state.test_assets / "avocado/Avocado.gltf");
//		auto const gltf_data = GLTF::Load(global_state.test_assets / "electric_guitar_fender_strat_plus/model.gltf");
		auto const gltf_data = GLTF::Load(global_state.test_assets / "sponza/Sponza.gltf");
		meshes.emplace_back(gltf_data, 0);

		try_to_reload_shader();
	}

	void shader_window()
	{
		using namespace ImGui;

		Begin("Shader Info");

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
				Text("%s", mapping.name.data());
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
				Text("%s", mapping.name.data());
			}
			EndTable();
		}

		End();
	}

	void render(GLFW::Window const & window) final
	{
		using namespace GL;

		// UI windows
		metrics_window();
		scene_settings_window();
		shader_window();
		scene_window();

		// Immediate rendering
		scene_render();
	}
};