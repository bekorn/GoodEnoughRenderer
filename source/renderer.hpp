#pragma once

#include "Lib/glfw/core.hpp"

#include "frame_info.hpp"

struct IRenderer
{
	virtual void create() = 0;

	virtual void render(GLFW::Window const & w, FrameInfo const & frame_data) = 0;
};