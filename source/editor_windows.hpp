#pragma once

#include "Lib/editor/core.hpp"

struct GameSettingsWindow : Editor::WindowBase
{
	const char * get_name() override
	{ return "Game Settings"; }

	void update(Editor::Context & ctx) override;
};

struct MaterialWindow : Editor::WindowBase
{
	const char * get_name() override
	{ return "Material Window"; }

	void update(Editor::Context & ctx) override;
};

struct Voxelizer : Editor::WindowBase
{
	const char * get_name() override
	{ return "Voxelizer"; }

	Name voxels_name = "voxels";
	bool should_compute = false;
	bool should_clear = false;
	i32x3 const voxels_res = i32x3(60);
	i32 fragment_multiplier = 2;

	void init(Editor::Context const & ctx) override;
	void update(Editor::Context & ctx) override;
	void render(Editor::Context const & ctx) override;
};