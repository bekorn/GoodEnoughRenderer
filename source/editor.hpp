#pragma once

#include "Lib/glfw/core.hpp"

#include "frame_info.hpp"

struct Game;
struct Assets;

struct Editor
{
	Assets & editor_assets;
	Game & game;

	explicit Editor(Assets & editor_assets, Game & game) :
		editor_assets(editor_assets), game(game)
	{}

	void metrics_window(FrameInfo const & frame_info, f64 seconds_since_game_render);
	void game_window();
	void game_settings_window();
	void node_settings_window();
	void mesh_settings_window();
	void material_settings_window();
	void textures_window();
	void program_window();
	void uniform_buffer_window();
	void camera_window();
	void workspaces();
	void create();
	void render(GLFW::Window const & window, FrameInfo const & frame_info, f64 seconds_since_game_render);
};
