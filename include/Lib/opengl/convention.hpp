#pragma once

#include "core.hpp"

namespace GL
{
	namespace ATTRIBUTE_LOCATION
	{
		// TODO(bekorn): This is not scalable :(
		//  OpenGL guarantees minimum 16 vertex attribute locations however, there are more than 16.
		//  If a shader uses, say WEIGHTS_1, it will not compile. The limit is GL_MAX_VERTEX_ATTRIBS.
		//  Read: Uniform Buffers
		//  .
		//  Temporary Solution: Disabled some attributes for now

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
//			JOINTS_2,
//			JOINTS_3,

			WEIGHTS_0,
			WEIGHTS_1,
//			WEIGHTS_2,
//			WEIGHTS_3,
		};

		// TODO(bekorn): ... this is horrible
		//  maybe utilize constexpr{array, string, vector} of cpp20
		// Update this with the last element
		auto const SIZE = WEIGHTS_1 + 1;

		char const* ToString(Type type)
		{
			switch (type)
			{
			case POSITION: return "POSITION";
			case NORMAL: return "NORMAL";
			case TANGENT: return "TANGENT";

			case COLOR_0: return "COLOR_0";
			case COLOR_1: return "COLOR_1";
			case COLOR_2: return "COLOR_2";
			case COLOR_3: return "COLOR_3";

			case TEXCOORD_0: return "TEXCOORD_0";
			case TEXCOORD_1: return "TEXCOORD_1";
			case TEXCOORD_2: return "TEXCOORD_2";
			case TEXCOORD_3: return "TEXCOORD_3";

			case JOINTS_0: return "JOINTS_0";
			case JOINTS_1: return "JOINTS_1";
//			case JOINTS_2: return "JOINTS_2";
//			case JOINTS_3: return "JOINTS_3";

			case WEIGHTS_0: return "WEIGHTS_0";
			case WEIGHTS_1: return "WEIGHTS_1";
//			case WEIGHTS_2: return "WEIGHTS_2";
//			case WEIGHTS_3: return "WEIGHTS_3";
			}
		}

		inline char const* ToString(u32 type)
		{
			return ToString(Type(type));
		}
	}
}
