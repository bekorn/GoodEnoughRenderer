#pragma once

#include "material.hpp"
#include "Lib/opengl/vao.hpp"

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
