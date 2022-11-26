#pragma once

#include "camera.hpp"

#include "Lib/asset_kitchen/assets.hpp"
#include "Lib/glfw/core.hpp"

namespace Render
{
struct GameBase
{
	Assets & assets;

	explicit GameBase(Assets & assets) :
		assets(assets)
	{}

	f32x4 clear_color{0.45f, 0.55f, 0.60f, 1.00f};
	f32 clear_depth = 1;
	i32x2 resolution{720, 720};
	GL::FrameBuffer framebuffer;
	GL::Texture2D framebuffer_depth_attachment;
	GL::Texture2D framebuffer_color_attachment;

	variant<PerspectiveCamera, OrthographicCamera> camera;

	virtual ~GameBase() = default;

	virtual void create() = 0;
	virtual void update(GLFW::Window const & window, FrameInfo const & frame_info) = 0;
	virtual void render(GLFW::Window const & window, FrameInfo const & frame_info) = 0;
};
}