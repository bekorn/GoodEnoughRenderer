#pragma once

#include "Lib/glfw/core.hpp"

#include "frame_info.hpp"

struct IRenderer
{
	virtual void create() = 0;

	virtual void render(GLFW::Window const &, FrameInfo const &) = 0;
};