#pragma once

#include "Lib/opengl/vao.hpp"

#include "material.hpp"

namespace Render
{
	struct ArrayDrawable
	{
		GL::VAO_ArrayDraw vao;
		u32 material_index;
	};

	struct ElementDrawable
	{
		GL::VAO_ElementDraw vao;
		u32 material_index;
	};
}
