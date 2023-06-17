#pragma once
#pragma message("-- read ASSET/LAYOUT/convert.Hpp --")

#include "load.hpp"

#include <core/expected.hpp>
#include "Lib/file_management/core.hpp"
#include "Lib/file_management/json_utils.hpp"
#include "Lib/geometry/core.hpp"

namespace GLSL::VertexLayout
{
std::pair<Name, Desc> Parse(File::JSON::JSONObj o, std::filesystem::path const & root_dir);
}