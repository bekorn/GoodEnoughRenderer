#pragma once

#include "Lib/core/index_ptr.hpp"
#include "Lib/opengl/vao.hpp"

#include "material.hpp"

namespace Render
{
	struct ArrayDrawable
	{
		GL::VAO_ArrayDraw vao;
		index_ptr<vector<unique_ptr<IMaterial>>> material_ptr;
	};

	struct ElementDrawable
	{
		GL::VAO_ElementDraw vao;
		index_ptr<vector<unique_ptr<IMaterial>>> material_ptr;
	};
}
