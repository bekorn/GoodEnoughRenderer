#pragma once
#pragma message("-- read ASSET/LAYOUT/load.Hpp --")

#include "Lib/core/core.hpp"
#include "Lib/geometry/core.hpp"

namespace GLSL::VertexLayout
{
struct Desc
{
	Geometry::Layout attributes;
};

Geometry::Layout Load(Desc const & desc);
}
