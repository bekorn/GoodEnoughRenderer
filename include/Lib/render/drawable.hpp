#pragma once

#include "Lib/core/index_ptr.hpp"
#include "Lib/opengl/vao.hpp"
#include "Lib/geometry/core.hpp"

#include "material.hpp"

namespace Render
{
	struct Primitive
	{
		index_ptr<vector<Geometry::Primitive>> primitive_ptr;
		index_ptr<vector<unique_ptr<IMaterial>>> material_ptr;

		GL::VertexArray vertex_array;

		bool is_loaded() const
		{
			return vertex_array.id != 0;
		}

		// TODO(bekorn): ShaderProgram should be accessible from the material
		void load(GL::ShaderProgram const & shader)
		{
			vertex_array.create(*primitive_ptr, shader);
		}

		void unload()
		{
			vertex_array = {};
		}
	};
}
