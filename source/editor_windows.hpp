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

struct Sdf3dWindow : Editor::WindowBase
{
	const char * get_name() override
	{ return "SDF 3D"; }

	Name volume_name = "voxels";

	void init(Editor::Context const & ctx) override;
	void update(Editor::Context & ctx) override;
	void render(Editor::Context const & ctx) override;

	bool should_clear = false;
	void clear(Editor::Context const & ctx);

	bool should_voxelize = false;
	i32 fragment_multiplier = 2;
	i32x3 const volume_res = i32x3(32);
	void voxelize(Editor::Context const & ctx);

	bool should_calculate_sdf = false;
	bool should_calculate_sdf_every_frame = false;
	void calculate_sdf(Editor::Context const & ctx);

	bool should_visualize_volume = false;
	void visualize_volume(Editor::Context const & ctx);

	bool should_visualize_voxels = false;
	void visualize_voxels(Editor::Context const & ctx);

	bool should_visualize_isosurface = false;
	f32 isosurface_value = 0;
	Name voxels_linear_view_name = "voxels_linear_view";
	void visualize_isosurface(Editor::Context const & ctx);
};