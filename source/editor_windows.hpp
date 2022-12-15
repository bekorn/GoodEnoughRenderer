#pragma once

#include "Lib/editor/core.hpp"
#include "Lib/editor/core_windows.hpp"

struct GameSettingsWindow : Editor::WindowBase
{
	const char * get_name() override
	{ return "Game Settings"; }

	void update(Editor::Context & ctx) override;
};

struct MetricsWindow : Editor::MetricsWindow
{
	void update(Editor::Context & ctx) override;
};

struct MaterialWindow : Editor::WindowBase
{
	const char * get_name() override
	{ return "Material Window"; }

	void update(Editor::Context & ctx) override;
};

struct IblBakerWindow : Editor::WindowBase
{
	const char * get_name() override
	{ return "IBL Baker"; }

	Name selected_name;
	bool is_texture_changed = false;
	f32x2 texture_size;
	bool should_map_equirectangular_to_cubemap = false;
	Name cubemap_name;

	void update(Editor::Context & ctx) override;
	void render(Editor::Context const & ctx) override;
};