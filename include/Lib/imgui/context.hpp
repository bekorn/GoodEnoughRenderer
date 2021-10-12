#pragma once

#include ".pch.hpp"
#include <Lib/opengl/util.hpp>

#include <format>

// Setup Dear ImGui context
struct ImguiContext
{
	ImguiContext() = default;

	struct Description
	{
		GLFW::Window const & window;

		u32 const & glsl_version;
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

		auto version_macro = "#version " + std::to_string(description.glsl_version);
		ImGui_ImplOpenGL3_Init(version_macro.c_str());
	}

	~ImguiContext()
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}
};
