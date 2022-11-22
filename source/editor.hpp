#pragma once

#include "Lib/asset_kitchen/assets.hpp"
#include "Lib/glfw/core.hpp"
#include "Lib/opengl/framebuffer.hpp"

#include "frame_info.hpp"
#include "game.hpp"

struct Editor
{
	Assets & editor_assets;
	Game & game;

	bool should_game_render = true;
	bool has_program_errors;

	explicit Editor(Assets & editor_assets, Game & game) :
		editor_assets(editor_assets), game(game)
	{}

	i32x2 resolution;
	GL::FrameBuffer framebuffer;
	GL::Texture2D framebuffer_depth_attachment;
	GL::Texture2D framebuffer_color_attachment;
	void create_framebuffer();
	void create_cubemap_framebuffer();
	void create_selection_framebuffer();
	void create();

	void metrics_window(FrameInfo const & frame_info, f64 game_update_in_seconds, f64 game_render_in_seconds);
	void game_window();
	void game_settings_window();
	// TODO(bekorn): framebuffers can keep their resolution
	// TODO(bekorn): a specilization for double buffered framebuffers might be handy
	i32x2 selection_framebuffer_resolution;
	GL::FrameBuffer selection_framebuffers[2];
	GL::Texture2D selection_framebuffer_color_attachments[2];
	Name selected_node_name;
	void node_settings_window();
	void mesh_settings_window();
	void material_settings_window();
	GL::Texture2D texture_view;
	void texture_2ds_window();
	GL::Texture2D cubemap_view;
	GL::FrameBuffer cubemap_framebuffer;
	GL::Texture2D cubemap_framebuffer_color_attachment;
	void texture_cubemaps_window();
	void program_window();
	void uniform_buffer_window();
	void camera_window();
	void workspaces();
	void update(GLFW::Window const & window, FrameInfo const & frame_info, f64 game_update_in_seconds, f64 game_render_in_seconds);

	void render(GLFW::Window const & window, FrameInfo const & frame_info);
};
