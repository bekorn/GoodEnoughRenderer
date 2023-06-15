#pragma once

#include "Lib/opengl/vao.hpp"
#include "Lib/geometry/core.hpp"

#include "material.hpp"

namespace Render
{
struct Drawable
{
	Geometry::Primitive const & primitive;
	Named<unique_one<IMaterial>> const named_material;

	GL::VertexArray vertex_array;

	bool is_loaded() const
	{
		return vertex_array.id != 0;
	}

	void load(Geometry::Layout const & vertex_layout)
	{
		vertex_array.init(GL::VertexArray::Desc{
			.geometry = primitive,
			.vertex_layout = vertex_layout,
		});
	}

	void unload()
	{
		vertex_array = {};
	}
};
}
