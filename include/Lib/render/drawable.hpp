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

	// TODO(bekorn): a drawable can be used by multiple shader programs,
	//  should load all the attributes that might be required but nothing more
	void load(span<GL::AttributeMapping const> attribute_mappings)
	{
		vertex_array.init(primitive, attribute_mappings);
	}

	void unload()
	{
		vertex_array = {};
	}
};
}
