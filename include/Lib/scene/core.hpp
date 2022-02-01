#pragma once

#include "Lib/core/core.hpp"

namespace Scene
{
	struct Transform
	{
		// Hierachy
		Transform * parent = nullptr;

		// Transform
		f32x3 position{0, 0, 0};
		f32x3 rotation{0, 0, 0};
		f32 scale{1};

		CTOR(Transform, default)
		COPY(Transform, default)
		MOVE(Transform, default)

		f32x4x4 calculate_transform() const
		{
			f32x4x4 transform(1);
			transform *= glm::translate(position);
			transform *= glm::orientate4(rotation);
			transform *= glm::scale(f32x3(scale));
			return transform;
		}
	};
}

