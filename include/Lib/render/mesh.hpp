#pragma once

#include "Lib/core/core.hpp"

#include "drawable.hpp"

namespace Render
{
	struct Mesh
	{
		vector<Drawable> drawables;

		CTOR(Mesh, default)
		COPY(Mesh, delete)
		MOVE(Mesh, default)
	};
}