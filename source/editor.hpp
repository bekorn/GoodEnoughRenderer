#pragma once

#include "renderer.hpp"
#include "game.hpp"

struct Editor final : IRenderer
{
	Assets & editor_assets;
	Game & game;

	explicit Editor(Assets & editor_assets, Game & game) :
		editor_assets(editor_assets), game(game)
	{}

	void metrics_window(FrameInfo const & frame_info);
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
	void create() final;
	void render(GLFW::Window const & window, FrameInfo const & frame_info) final;
};
