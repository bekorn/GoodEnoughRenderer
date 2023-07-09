#pragma once
#pragma message("-- read ASSET/LAYOUT/convert.Hpp --")

#include <core/expected.hpp>
#include <core/geometry.hpp>
#include <file_io/core.hpp>
#include <file_io/json_utils.hpp>

namespace AttribLayout
{
inline constexpr const char * ASSET_NAME = "attrib_layout";

std::pair<Name, Geometry::Layout> Parse(File::JSON::ConstObj o, std::filesystem::path const & root_dir);
}