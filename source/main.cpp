#include <iostream>
#include <vector>

#include "Lib/core/.hpp"
#include "Lib/glfw/.hpp"
#include "Lib/imgui/.hpp"

#include "Lib/file_management/.hpp"

#include "renderer.hpp"

i32 main(i32 argc, char** argv)
{
	global_state.test_assets = argv[1];


	GLFWContext glfw_context;
	GLFW::Window window;
	ImguiContext imgui_context;

	try
	{
		glfw_context.create();
		window.create({.title = "Good Enough Renderer", .vsync = true, .gl_major = 4, .gl_minor = 3});
		imgui_context.create({.window = window, .glsl_version = 430});
		std::cout.flush();
	}
	catch (std::runtime_error & error)
	{
		std::cerr << error.what();
		std::terminate();
	}


	std::vector<std::unique_ptr<IRenderer>> renderers;
	renderers.emplace_back(new MainRenderer);

	for (auto & renderer: renderers)
		renderer->create(global_state);


	while (not glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		// Start ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		for (auto & renderer: renderers)
			renderer->render(window);

		// Render Imgui frame
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}

	return 0;
}
