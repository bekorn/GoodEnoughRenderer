#pragma once

#include <opengl/utils.hpp>
#include <opengl/glsl.hpp>

namespace Imgui
{
struct Context
{
	Context() = default;

	struct Desc
	{
		GLFW::Window const & window;
	};

	void init(Desc const & desc)
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		//ImGuiIO& io = ImGui::GetIO();
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();

		// Setup Platform/Renderer backends
		ImGui_ImplGlfw_InitForOpenGL(desc.window, true);

		ImGui_ImplOpenGL3_Init(GL::GLSL_VERSION_MACRO.c_str());
	}

	~Context()
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}
};
}

