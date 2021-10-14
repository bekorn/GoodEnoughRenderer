#pragma once

#include ".pch.hpp"

namespace GL
{
	namespace ATTRIBUTE_LOCATION
	{
		enum Type
		{
			POSITION = 0,
			NORMAL,
			TANGENT,

			COLOR_0,
			COLOR_1,
			COLOR_2,
			COLOR_3,

			TEXCOORD_0,
			TEXCOORD_1,
			TEXCOORD_2,
			TEXCOORD_3,

			JOINTS_0,
			JOINTS_1,
			JOINTS_2,
			JOINTS_3,

			WEIGHTS_0,
			WEIGHTS_1,
			WEIGHTS_2,
			WEIGHTS_3,
		};
	}
}
