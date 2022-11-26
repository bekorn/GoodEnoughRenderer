#pragma once

#include "core.hpp"
#include "utils.hpp"

namespace Editor
{
struct GameWindow : WindowBase
{
	const char * get_name() override
	{ return "Game"; }

	i32x2 resolution;
	GL::FrameBuffer framebuffer;
	GL::Texture2D depth_attachment;
	GL::Texture2D color_attachment;

	void create(Context const & ctx) override;
	void update(Context & ctx) override;
	void render(Context const & ctx) override;
};

struct MetricsWindow : WindowBase
{
	const char * get_name() override
	{ return "Metrics "; }

	void update(Context & ctx) override;
};

struct UniformBufferWindow : WindowBase
{
	const char * get_name() override
	{ return "UniformBuffer"; }

	void update(Context & ctx) override;
};

struct ProgramWindow : WindowBase
{
	const char * get_name() override
	{ return "Program"; }

	void update(Context & ctx) override;
};

struct TextureWindow : WindowBase
{
	const char * get_name() override
	{ return "Texture"; }

	GL::Texture2D texture_view;

	void update(Context & ctx) override;
};

struct CubemapWindow : WindowBase
{
	const char * get_name() override
	{ return "Cubemap"; }

	bool should_render;
	GL::TextureCubemap view;
	GL::FrameBuffer framebuffer;
	GL::Texture2D framebuffer_color_attachment;

	void create(Context const & ctx) override;
	void update(Context & ctx) override;
	void render(Context const & ctx) override;
};

struct MeshWindow : WindowBase
{
	const char * get_name() override
	{ return "Mesh"; }

	void update(Context & ctx) override;
};

struct NodeEditor : WindowBase
{
	const char * get_name() override
	{ return "Node Editor"; }

	void update(Context & ctx) override;
};

struct CameraWindow : WindowBase
{
	const char * get_name() override
	{ return "Camera"; }

	void update(Context & ctx) override;
};

struct GameSettingsWindow : WindowBase
{
	const char * get_name() override
	{ return "Game Settings"; }

	void update(Context & ctx) override;
};

struct MaterialWindow : WindowBase
{
	const char * get_name() override
	{ return "Material Window"; }

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
	ctx.add_window(make_unique_one<GameSettingsWindow>());
	ctx.add_window(make_unique_one<MaterialWindow>());
}

}