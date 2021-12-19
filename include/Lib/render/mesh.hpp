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
		std::string name; // just for debug purposes

		// Vertex attributes + material index
		vector<ArrayDrawable> array_drawables;
		vector<ElementDrawable> element_drawables;

		// Transform data
		f32x3 position{0, 0, 0};
		f32x3 rotation{0, 0, 0};
		f32 scale{1};

		Mesh() noexcept = default;
		Mesh(Mesh const &) noexcept = delete;
		Mesh(Mesh &&) noexcept = default;

		glm::mat4x4 CalculateTransform() const
		{
			glm::mat4x4 transform(1);
			transform *= glm::translate(position);
			transform *= glm::orientate4(glm::radians(rotation));
			transform *= glm::scale(f32x3(scale));
			return transform;
		}
	};
}