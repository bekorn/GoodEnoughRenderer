#pragma once

#include <core/geometry.hpp>
#include <opengl/vao.hpp>

#include "material.hpp"

namespace Render
{
struct Drawable
{
	Named<unique_one<IMaterial>> const named_material;
	GL::VertexArray vertex_array;

	Drawable(Named<unique_one<IMaterial>> const & named_material, Geometry::Primitive const & primitive) :
		named_material(named_material)
	{
		vertex_array.init(GL::VertexArray::Desc{
			.primitive = primitive
		});
	}
};
}
