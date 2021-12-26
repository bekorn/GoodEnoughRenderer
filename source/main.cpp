#include <iostream>

#include "Lib/core/.hpp"
#include "Lib/opengl/.hpp"
#include "Lib/glfw/.hpp"
#include "Lib/imgui/.hpp"

#include "assets.hpp"
#include "game.hpp"
#include "editor.hpp"

i32 main(i32 argc, char** argv)
{
	GLFW::Context glfw_context;
	GLFW::Window window;
	Imgui::Context imgui_context;

	if (auto error = glfw_context.create())
	{
		std::cerr << error.value();
		std::exit(1);
	}

	if (auto error = window.create(
		{
			.title = "Good Enough Renderer",
			.size = {860, 860},
			.vsync = true,
			.gl_major = GL::VERSION_MAJOR, .gl_minor = GL::VERSION_MINOR,
		}
	))
	{
		std::cerr << error.value();
		std::exit(1);
	}

	imgui_context.create({.window = window});
	std::clog << GL::GetContextInfo() << std::endl;

	Assets assets;

	vector<unique_ptr<IRenderer>> renderers;
	{
		auto game = make_unique<Game>(assets);
		auto editor = make_unique<Editor>(assets, *game);

		renderers.emplace_back(move(game));
		renderers.emplace_back(move(editor));
	}

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

		{
			using namespace GL;

			// Reset any modified settings
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			i32 w, h;
			glfwGetWindowSize(window, &w, &h);
			glViewport(0, 0, w, h);

			glEnable(GL_DEPTH_TEST);

			// clear
			f32x4 clear_color(0, 0, 0, 1);
			glClearNamedFramebufferfv(0, GL::GL_COLOR, 0, begin(clear_color));
			f32 clear_depth = 0;
			glClearNamedFramebufferfv(0, GL::GL_DEPTH, 0, &clear_depth);
		}

		// Render Imgui frame
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}

	return 0;
}
