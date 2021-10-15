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
	// Settings
	f32x4 clear_color = f32x4(0.45f, 0.55f, 0.60f, 1.00f);

	//	Texture2D render_target;

	std::vector<Mesh> meshes;

	std::string status;


	std::array<GL::ShaderProgram, GL::ATTRIBUTE_LOCATION::SIZE> attribute_visualizers;

	void load_attribute_visualizers()
	{
		// Limitation: only works with vectors
		auto const vert_source = LoadAsString(global_state.test_assets / "debug/single_attributes.vert.glsl");
		auto const frag_source = LoadAsString(global_state.test_assets / "debug/single_attribute.frag.glsl");

		auto const & glsl_version = GL::GLSL_VERSION_MACRO;
		auto const layout_convention = GL::GET_GLSL_ATTRIBUTE_LOCATION_MACROS();

		GL::ShaderStage frag_shader;
		frag_shader.create(
			GL::ShaderStage::Description{
				.stage = GL::GL_FRAGMENT_SHADER,
				.sources = {
					glsl_version,
					frag_source.c_str(),
				},
			}
		);

		for (auto i = 0; i < attribute_visualizers.size(); ++i)
		{
			auto const attribute_location = "#define ATTRIBUTE_LOCATION_VISUALIZE " + std::to_string(i) + "\n";

			GL::ShaderStage vert_shader;
			vert_shader.create(
				GL::ShaderStage::Description{
					.stage = GL::GL_VERTEX_SHADER,
					.sources = {
						glsl_version,
						layout_convention.c_str(),
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

		auto const vert_source = LoadAsString(global_state.test_assets / "test.vert.glsl");
		auto const frag_source = LoadAsString(global_state.test_assets / "test.frag.glsl");

		auto const & glsl_version = GL::GLSL_VERSION_MACRO;
		auto const layout_convention = GL::GET_GLSL_ATTRIBUTE_LOCATION_MACROS();

		bool shader_stage_error = false;

		GL::ShaderStage vert_shader;
		vert_shader.create(
			GL::ShaderStage::Description{
				.stage = GL::GL_VERTEX_SHADER,
				.sources = {
					glsl_version,
					layout_convention.c_str(),
					vert_source.c_str(),
				},
			}
		);
		if (not vert_shader.is_compiled())
		{
			shader_stage_error = true;
			program_info += "Vert Shader Error:\n" + vert_shader.get_log();
		}

		GL::ShaderStage frag_shader;
		frag_shader.create(
			GL::ShaderStage::Description{
				.stage = GL::GL_FRAGMENT_SHADER,
				.sources = {
					glsl_version,
					layout_convention.c_str(),
					frag_source.c_str(),
				},
			}
		);
		if (not frag_shader.is_compiled())
		{
			shader_stage_error = true;
			program_info += "Frag Shader Error:\n" + frag_shader.get_log();
		}

		if (shader_stage_error)
			return;


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
			program_info += "Linking Error:\n" + new_program.get_log();
			return;
		}

		// TODO(bekorn): This is probably not good.. Find a better wrapping for OpenGLObjects
		program.~ShaderProgram();

		program = std::move(new_program);
		GL::glUseProgram(program.id);

		program_info += "Shader id: " + std::to_string(program.id) + "\n\n";
		program_info += program.get_active_uniforms() + '\n';
		program_info += program.get_active_attributes();
	}

	void create() final
	{
		load_attribute_visualizers();

		//		auto const gltf_data = GLTF::Load(global_state.test_assets / "vertex colored cube.gltf");
		auto const gltf_data = GLTF::Load(global_state.test_assets / "axis.gltf");
		meshes.emplace_back(gltf_data, 0);

		try_to_reload_shader();
	}

	void metrics_window()
	{
		using namespace ImGui;

		SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Once);
		SetNextWindowSize(ImVec2(600, 160), ImGuiCond_Once);
		Begin("Metrics", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

		Text(
			"Application average %.3f ms/frame (%.1f FPS)",
			1000.f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate
		);

		Separator();

		Text("|> %s", status.c_str());

		End();
	}

	/*
	void program_window()
	{
		using namespace ImGui;

		Begin("Program Controls"); // Create a window called "Hello, world!" and append into it.

		if (Text("Name"), SameLine(), InputText("", state.shader_name.data(), state.shader_name.size()))
			state.update_handles(state.shader_name.data());


		{
			auto & vert = state.shader_handles[ShaderHandle::VERT];
			Text("vert = %s", vert.path.string().c_str());
			bool value = vert.is_file();
			SameLine(), ImGui::Checkbox("Exists", &value);
		}
		{
			auto & frag = state.shader_handles[ShaderHandle::FRAG];
			Text("frag = %s", frag.path.string().c_str());
			bool value = frag.is_file();
			SameLine(), ImGui::Checkbox("Exists", &value);
		}
		{
			auto & geom = state.shader_handles[ShaderHandle::GEOM];
			Text("geom = %s", geom.path.string().c_str());
			bool value = geom.is_file();
			SameLine(), ImGui::Checkbox("Exists", &value);
		}

		if (Button("Compile", {100, GetTextLineHeight() * 1.5f}))
		{
			if (state.program.id != 0)
			{
				glDeleteProgram(state.program.id);
				state.program.id = 0;
			}

			state.program.create(
				{
					.name = state.shader_name.data()
				}
			);

			if (state.program.id != 0)
			{
				std::clog << (state.status = "Compiled successfully") << '\n';
			}
			else
			{
				GLint maxLength = 0;
				glGetShaderiv(state.program.id, GL_INFO_LOG_LENGTH, &maxLength);

				// The maxLength includes the NULL character
				std::vector<GLchar> errorLog(maxLength);
				glGetShaderInfoLog(state.program.id, maxLength, &maxLength, errorLog.data());

				std::cerr << (state.status = "Compilation failed :(\n"s + errorLog.data()) << '\n';
			}
		}

		NewLine();
		Text("Program ID: %i", state.program.id);

		End();
	}
	*/

	void shader_window()
	{
		using namespace ImGui;

		Begin("Shader stuff");

		if (Button("Reload Shader"))
		{
			try_to_reload_shader();
		}

		Text("%s", program_info.c_str());

		End();
	}

	bool visualize_attribute = false;

	void realtime_settings_window()
	{
		using namespace ImGui;

		Begin("Realtime Settings");

		ColorEdit3("clear color", glm::value_ptr(clear_color));

		//		Checkbox("Enable Depth Test", &enable_depth_test);

		i32 current_program;
		GL::glGetIntegerv(GL::GL_CURRENT_PROGRAM, &current_program);
		Text("Current Program id: %d", current_program);

		static i32 current_item = 0;
		if (Checkbox("Visualize", &visualize_attribute))
		{
			GL::glUseProgram(visualize_attribute ? attribute_visualizers[current_item].id : program.id);
		}

		if (visualize_attribute)
		{
			auto const & elements = GL::ATTRIBUTE_LOCATION::AsArray();
			if (Combo("Attribute", &current_item, elements.data(), elements.size()))
			{
				GL::glUseProgram(attribute_visualizers[current_item].id);
			}
		}

		End();
	}

	void render(GLFW::Window const & window) final
	{
		using namespace GL;

		// UI windows
		metrics_window();
		//		program_window();
		realtime_settings_window();

		shader_window();


		glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glEnable(GL_DEPTH_TEST);

		for (auto const & mesh: meshes)
		{
			for (auto const & vao: mesh.array_vaos)
			{
				glBindVertexArray(vao->id);
				glDrawArrays(GL_TRIANGLES, 0, vao->vertex_count);
			}
			for (auto const & vao: mesh.element_vaos)
			{
				glBindVertexArray(vao->id);
				glDrawElements(GL_TRIANGLES, vao->element_count, GL_UNSIGNED_SHORT, nullptr);
			}
		}
	}
};