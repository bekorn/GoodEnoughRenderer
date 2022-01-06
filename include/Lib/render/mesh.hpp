#pragma once

#include "Lib/core/core.hpp"
#include "Lib/opengl/.hpp"

#include "material.hpp"
#include "drawable.hpp"

namespace Render
{
	// Limitation: Only supports OpenGL
	// TODO(bekorn): This struct should not own the GLObjects, just reference them (drawables owns VAOs)
	// TODO(bekorn): Mesh should only reference geometry and material resources
	struct Mesh
	{
		// Vertex attributes + material index
		vector<Drawable> drawables;

		// Transform data
		f32x3 position{0, 0, 0};
		f32x3 rotation{0, 0, 0};
		f32 scale{1};

		CTOR(Mesh, default)
		COPY(Mesh, delete)
		MOVE(Mesh, default)

		f32x4x4 CalculateTransform() const
		{
			f32x4x4 transform(1);
			transform *= glm::translate(position);
			transform *= glm::orientate4(rotation);
			transform *= glm::scale(f32x3(scale));
			return transform;
		}
	};
}