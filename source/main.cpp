#include <iostream>

#include "Lib/core/.hpp"
#include "Lib/opengl/.hpp"
#include "Lib/glfw/.hpp"
#include "Lib/imgui/.hpp"
#include "Lib/asset_kitchen/assets.hpp"

#include "game.hpp"
#include "editor.hpp"

i32 main(i32 argc, char** argv)
{
	if (argc < 2)
	{
		std::cerr << "First parameter must be the project directory";
		std::exit(1);
	}
	auto project_root = std::filesystem::path(argv[1]);
	if (not std::filesystem::exists(project_root))
	{
		std::cerr << "Given directory does not exist";
		std::exit(1);
	}

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

	Descriptions descriptions;
	descriptions.create(project_root);

	Assets assets(descriptions);
	assets.create();

	vector<unique_one<IRenderer>> renderers;
	{
		auto game = make_unique_one<Game>(assets);
		auto editor = make_unique_one<Editor>(assets, *game);

		renderers.emplace_back(move(game));
		renderers.emplace_back(move(editor));
	}

	for (auto & renderer: renderers)
		renderer->create();

	FrameInfo frame_info;
	FrameInfo previous_frame_info{
		.idx = 0,
		.seconds_since_start = glfwGetTime(),
		.seconds_since_last_frame = 0,
	};

	while (not glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		frame_info.seconds_since_start = glfwGetTime();
		frame_info.seconds_since_last_frame = frame_info.seconds_since_start - previous_frame_info.seconds_since_start;
		frame_info.idx = previous_frame_info.idx + 1;

		// Start ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		for (auto & renderer: renderers)
			renderer->render(window, frame_info);

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

		previous_frame_info = frame_info;
	}

	return 0;
}
