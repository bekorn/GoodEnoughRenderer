#include <iostream>
#include <vector>

#include "Lib/core/.hpp"
#include "Lib/opengl/util.hpp"
#include "Lib/glfw/.hpp"
#include "Lib/imgui/.hpp"
#include "Lib/file_management/.hpp"

#include "renderer.hpp"

i32 main(i32 argc, char** argv)
{
	GLFWContext glfw_context;
	GLFW::Window window;
	ImguiContext imgui_context;

	try
	{
		glfw_context.create();
		window.create({.title = "Good Enough Renderer", .vsync = true, .gl_major = 4, .gl_minor = 3});
		imgui_context.create({.window = window});

		std::clog << GL::GetContextInfo() << std::endl;
	}
	catch (std::runtime_error & error)
	{
		std::cerr << error.what();
		std::exit(1);
	}


	std::vector<std::unique_ptr<IRenderer>> renderers;
	renderers.emplace_back(new MainRenderer);

	for (auto & renderer: renderers)
		renderer->create();


	while (not glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		// Start ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		for (auto & renderer: renderers)
			renderer->render(window);

		// Reset any modified settings
		{
			using namespace GL;

			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			i32 w, h;
			glfwGetWindowSize(window, &w, &h);
			glViewport(0, 0, w, h);

			glClearColor(0, 0, 0, 1);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glEnable(GL_DEPTH_TEST);
		}

		// Render Imgui frame
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}

	return 0;
}
