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

	void init(Editor::Context const & ctx) override;
	void update(Editor::Context & ctx) override;
	void render(Editor::Context const & ctx) override;

	bool should_clear = false;
	void clear(Editor::Context const & ctx);

	bool should_compute = false;
	i32 fragment_multiplier = 2;
	i32x3 const voxels_res = i32x3(2*60);
	void voxelize(Editor::Context const & ctx);

	bool should_visualize_voxels = false;
	void visualize_voxels(Editor::Context const & ctx);
};