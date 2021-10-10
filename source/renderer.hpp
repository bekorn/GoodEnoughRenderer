#pragma once

#include "Lib/glfw/window.hpp"
#include "Lib/opengl/.hpp"

// TODO(bekorn): pass this without globals
struct GlobalState
{
	std::filesystem::path test_assets;

} global_state;

struct IRenderer
{
	virtual void create(GlobalState & global_state) = 0;
	virtual void render(GLFW::Window const & w) = 0;
};


struct MainRenderer : IRenderer
{
	// Settings
	bool show_demo_window = false;

	f32x4 clear_color = f32x4(0.45f, 0.55f, 0.60f, 1.00f);
	i32 active_vao = 1;
	bool enable_depth_test = true;

//	std::string shader_name = "basic";
//
//	ShaderHandle shader_handles[ShaderHandle::STAGE_COUNT] = {
//		ShaderHandle(shader_name.data(), (ShaderHandle::Stage) 0),
//		ShaderHandle(shader_name.data(), (ShaderHandle::Stage) 1),
//		ShaderHandle(shader_name.data(), (ShaderHandle::Stage) 2)
//	};
//
//	void update_handles(std::string const & name)
//	{
//		for (auto & handle : shader_handles)
//			handle = ShaderHandle(name, handle.stage);
//	}
//
//	// Persistent resources
//	ShaderProgram program;
//
//	Texture2D render_target;
//	std::vector<std::shared_ptr<Buffer>> gl_buffers_handles;
//	VAO_ElementDraw quad_vao;
//	VAO_ArrayDraw dragon_vao;

	std::string status;

//	std::string gltf_file;

	void create(GlobalState & global_state) final
	{
		GLTF::Load(global_state.test_assets / "vertex colored cube.gltf");
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

		Checkbox("Demo Window", &show_demo_window); // Edit bools storing our window open/close state

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

	void realtime_settings_window()
	{
		using namespace ImGui;

		Begin("Realtime Settings");

		ColorEdit3("clear color", glm::value_ptr(clear_color));

		Checkbox("Enable Depth Test", &enable_depth_test);

		SliderInt("Active VAO", &active_vao, 0, 1);

		End();
	}

	void render(GLFW::Window const & window) final
	{
		using namespace gl43core;

		glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// UI windows
		metrics_window();
//		program_window();
		realtime_settings_window();
	}
};