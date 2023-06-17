#pragma once
#pragma message("-- read ASSET/LAYOUT/load.Hpp --")

#include <core/core.hpp>
#include <core/geometry.hpp>

namespace GLSL::VertexLayout
{
struct Desc
{
	Geometry::Layout attributes;
};

Geometry::Layout Load(Desc const & desc);
}
