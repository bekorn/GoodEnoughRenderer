#pragma once

#include "camera.hpp"

#include <asset_kitchen/assets.hpp>
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
	GL::FrameBuffer framebuffer;

	variant<PerspectiveCamera, OrthographicCamera> camera;

	virtual ~GameBase() = default;

	virtual void init() = 0;
	virtual void update(GLFW::Window const & window, FrameInfo const & frame_info) = 0;
	virtual void render(GLFW::Window const & window, FrameInfo const & frame_info) = 0;
};
}