#include <core/core.hpp>
#include <opengl/core.hpp>
#include <opengl/globals.hpp>
#include <opengl/use_dedicated_device_by_default.hpp>
#include <render/glfw.hpp>
#include <render/imgui.hpp>
#include <render/frame_info.hpp>
#include <asset_recipes/assets.hpp>
#include <editor/core.hpp>
#include <editor/core_windows.hpp>
#include <editor/envmap_baker.hpp>

#include "game.hpp"
#include "editor_windows.hpp"

i32 main(i32 argc, char** argv)
{
	if (argc < 2)
	{
		fmt::print(stderr, "First parameter must be the editor directory");
		std::exit(1);
	}
	auto editor_root = std::filesystem::path(argv[1]);
	if (not std::filesystem::exists(editor_root))
	{
		fmt::print(stderr, "Editor directory \"{}\" does not exist", editor_root);
		std::exit(1);
	}

	if (argc < 3)
	{
		fmt::print(stderr, "Second parameter must be the project directory");
		std::exit(1);
	}
	auto project_root = std::filesystem::path(argv[2]);
	if (not std::filesystem::exists(project_root))
	{
		fmt::print(stderr, "Project directory \"{}\" does not exist", project_root);
		std::exit(1);
	}

	GLFW::Context glfw_context;
	GLFW::Window window;
	Imgui::Context imgui_context;

	if (auto error = glfw_context.init())
	{
		fmt::print(stderr, "{}", error.value());
		std::exit(1);
	}

	if (auto error = window.init(
		{
			.title = "Good Enough Renderer",
			.size = {860, 860},
			.vsync = true,
			.gl_major = GL::VERSION_MAJOR, .gl_minor = GL::VERSION_MINOR,
		}
	))
	{
		fmt::print(stderr, "{}", error.value());
		std::exit(1);
	}

	imgui_context.init({.window = window});
	fmt::print("{}\n", GL::get_context_info());

	GL::init();
	GL::init_globals();

	// Project assets
	Descriptions descriptions;
	descriptions.init(project_root);
	Assets game_assets(descriptions);
	game_assets.init();

	// Editor assets
	Descriptions editor_descriptions;
	editor_descriptions.init(editor_root);
	Assets editor_assets(editor_descriptions);
	editor_assets.init();

	Game game(game_assets);
	game.init();

	Editor::Context editor_ctx(game, editor_assets, window);
	Editor::add_all_core_windows(editor_ctx);
	editor_ctx.add_window(make_unique_one<Editor::EnvmapBakerWindow>());
	editor_ctx.add_window(make_unique_one<GameSettingsWindow>());
	editor_ctx.add_window(make_unique_one<MaterialWindow>());
	editor_ctx.add_window(make_unique_one<Sdf3dWindow>());
	editor_ctx.init();

	Render::FrameInfo frame_info;
	Render::FrameInfo previous_frame_info{
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

		static f64 game_update_in_seconds = 0;
		static f64 game_render_in_seconds = 0;

		editor_ctx.update_windows(frame_info, game_update_in_seconds, game_render_in_seconds);

		auto before_game_update = glfwGetTime();
		game.update(window, frame_info);
		game_update_in_seconds = glfwGetTime() - before_game_update;

		auto before_game_render = glfwGetTime();
		if (editor_ctx.state.should_game_render)
			game.render(window, frame_info);
		game_render_in_seconds = glfwGetTime() - before_game_render;

		editor_ctx.render_windows();

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
