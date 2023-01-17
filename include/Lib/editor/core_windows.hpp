#pragma once

#include "core.hpp"
#include "utils.hpp"

namespace Editor
{
struct GameWindow : WindowBase
{
	const char * get_name() override
	{ return "Game"; }

	GL::FrameBuffer framebuffer;

	bool render_single_frame = false;
	f32 scale = 0.5;

	void create(Context const & ctx) override;
	void update(Context & ctx) override;
	void render(Context const & ctx) override;
};

struct MetricsWindow : WindowBase
{
	const char * get_name() override
	{ return "Metrics "; }

	template<usize FrameCount>
	struct moving_average
	{
		array<f64, FrameCount> samples{};
		f64 sum = 0;

		f64 add(u64 idx, f64 sample)
		{
			auto sample_idx = idx % FrameCount;
			sum -= samples[sample_idx];
			sum += sample;
			samples[sample_idx] = sample;
			return sum / FrameCount;
		}
	};

	moving_average<30> average_frame;
	moving_average<30> average_game_update;
	moving_average<30> average_game_render;

	void update(Context & ctx) override;
};

struct UniformBufferWindow : WindowBase
{
	const char * get_name() override
	{ return "UniformBuffer"; }

	Name selected_name;

	void update(Context & ctx) override;
};

struct ProgramWindow : WindowBase
{
	const char * get_name() override
	{ return "Program"; }

	Name selected_name;
	Assets * assets;

	void create(const Context & ctx) override;
	void update(Context & ctx) override;
};

struct TextureWindow : WindowBase
{
	const char * get_name() override
	{ return "Texture"; }

	Name selected_name;

	GL::Texture2D view;
	bool is_texture_changed = false;
	bool is_level_changed = false;
	i32 texture_levels = 0, current_level = 0;
	f32x2 texture_size, view_size;

	void update(Context & ctx) override;
};

struct CubemapWindow : WindowBase
{
	const char * get_name() override
	{ return "Cubemap"; }

	GL::TextureCubemap view;
	GL::FrameBuffer framebuffer;

	Name selected_name;

	bool should_render;
	bool is_changed = false;
	bool is_level_changed = false;
	i32 levels = 0, current_level = 0;
	f32x2 size, view_size;

	void create(Context const & ctx) override;
	void update(Context & ctx) override;
	void render(Context const & ctx) override;
};

struct MeshWindow : WindowBase
{
	const char * get_name() override
	{ return "Mesh"; }

	Name selected_name;
	u64 drawable_index;
	u64 min_index = 0, max_index;

	void update(Context & ctx) override;
};

struct NodeEditor : WindowBase
{
	const char * get_name() override
	{ return "Node Editor"; }

	Name node_name;
	f32x3 mesh_orientation;

	void update(Context & ctx) override;
};

struct CameraWindow : WindowBase
{
	const char * get_name() override
	{ return "Camera"; }

	void update(Context & ctx) override;
};

inline void add_all_core_windows(Context & ctx)
{
	ctx.add_window(make_unique_one<GameWindow>());
	ctx.add_window(make_unique_one<MetricsWindow>());
	ctx.add_window(make_unique_one<UniformBufferWindow>());
	ctx.add_window(make_unique_one<ProgramWindow>());
	ctx.add_window(make_unique_one<TextureWindow>());
	ctx.add_window(make_unique_one<CubemapWindow>());
	ctx.add_window(make_unique_one<MeshWindow>());
	ctx.add_window(make_unique_one<NodeEditor>());
	ctx.add_window(make_unique_one<CameraWindow>());
}

}