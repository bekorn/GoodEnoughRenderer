#pragma once

#include <core/core.hpp>
#include <core/geometry.hpp>
#include <core/named.hpp>
#include <file_io/json_utils.hpp>

namespace AttribLayout
{
inline constexpr const char * NAME = "attrib_layout";
std::pair<Name, Geometry::Layout> Parse(File::JSON::JSONObj o);
}