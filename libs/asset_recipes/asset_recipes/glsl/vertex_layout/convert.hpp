#pragma once
#pragma message("-- read ASSET/LAYOUT/convert.Hpp --")

#include "load.hpp"

#include <core/expected.hpp>
#include <core/geometry.hpp>
#include <file_io/core.hpp>
#include <file_io/json_utils.hpp>

namespace GLSL::VertexLayout
{
std::pair<Name, Desc> Parse(File::JSON::JSONObj o, std::filesystem::path const & root_dir);
}