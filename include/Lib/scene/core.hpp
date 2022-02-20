#pragma once

#include "Lib/core/core.hpp"
#include "Lib/render/mesh.hpp"

namespace Scene
{
	struct Transform
	{
		f32x3 position{0, 0, 0};
		f32quat rotation{1, 0, 0, 0};
		f32x3 scale{1, 1, 1};

		f32x4x4 calculate_transform() const
		{
			f32x4x4 transform(1);
			transform *= glm::translate(position);
			transform *= glm::mat4_cast(rotation);
			transform *= glm::scale(scale);
			return transform;
		}
	};

	struct Node
	{
		Node * parent = nullptr;
		Transform transform;
		Render::Mesh * mesh;
	};
}

