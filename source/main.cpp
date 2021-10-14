#include <iostream>
#include <vector>

#include "Lib/core/.hpp"
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

		auto renderer = gl::glGetString(gl::GL_RENDERER);
		std::clog << "Renderer: " << renderer << '\n';
		auto const glsl_version = (const char*)gl::glGetString(gl::GL_SHADING_LANGUAGE_VERSION);
		u32 major, minor;
		std::sscanf(glsl_version, "%d.%d", &major, &minor);
		std::clog << "GLSL version: " << major << '.' << minor << '\n';

		imgui_context.create({.window = window, .glsl_version = (major * 100 + minor)});

		std::clog << std::endl;
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

		// Render Imgui frame
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}

	return 0;
}
