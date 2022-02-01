#pragma once

#include "Lib/core/core.hpp"
#include "Lib/scene/core.hpp"

#include "drawable.hpp"

namespace Render
{
	// Limitation: Only supports OpenGL
	// TODO(bekorn): This struct should not own the GLObjects, just reference them (drawables owns VAOs)
	// TODO(bekorn): Mesh should only reference geometry and material resources
	struct Mesh
	{
		Scene::Transform transform;
		// Vertex attributes + material index
		vector<Drawable> drawables;

		CTOR(Mesh, default)
		COPY(Mesh, delete)
		MOVE(Mesh, default)
	};
}