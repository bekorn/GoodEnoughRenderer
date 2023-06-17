#pragma once
#pragma message("-- read ASSET/PROGRAM/convert.Hpp --")

#include <core/core.hpp>
#include <core/expected.hpp>
#include "Lib/file_management/core.hpp"
#include "Lib/file_management/json_utils.hpp"
#include "Lib/opengl/glsl.hpp"
#include "Lib/opengl/shader.hpp"

#include "load.hpp"

namespace GLSL::Program
{
Expected<GL::ShaderProgram, std::string> Convert(LoadedData const & loaded, Managed<Geometry::Layout> const & vertex_layouts);

std::pair<Name, Desc> Parse(File::JSON::JSONObj o, std::filesystem::path const & root_dir);
}