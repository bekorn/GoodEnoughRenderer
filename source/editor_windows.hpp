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

// TODO(bekorn): there should be a mechanism to encapsulate this window into a package/plugin to load dynamically
struct EnvmapBakerWindow : Editor::WindowBase
{
	const char * get_name() override
	{ return "Envmap Baker"; }

	Name selected_name;
	bool is_texture_changed = false;
	f32x2 texture_size;
	bool should_generate_envmap = false;
	bool should_generate_brdf_lut = false;

	Name cubemap_name;
	void update(Editor::Context & ctx) override;
	void render(Editor::Context const & ctx) override;
	void generate_brdf_lut(const Editor::Context & ctx) const;
	void generate_envmap(Editor::Context const & ctx) const;
};