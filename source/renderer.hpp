#pragma once

#include "Lib/glfw/core.hpp"

struct IRenderer
{
	virtual void create() = 0;

	virtual void render(GLFW::Window const & w) = 0;
};