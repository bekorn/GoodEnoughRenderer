#pragma once

#include "Lib/opengl/utils.hpp"

namespace Imgui
{
	struct Context
	{
		Context() = default;

		struct Description
		{
			GLFW::Window const & window;
		};

		void create(Description const & description)
		{
			IMGUI_CHECKVERSION();
			ImGui::CreateContext();
			//ImGuiIO& io = ImGui::GetIO();
			//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

			// Setup Dear ImGui style
			ImGui::StyleColorsDark();

			// Setup Platform/Renderer backends
			ImGui_ImplGlfw_InitForOpenGL(description.window, true);

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

