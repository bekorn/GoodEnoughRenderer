#pragma once
#pragma message("-- read ASSET/PROGRAM/convert.Hpp --")

#include <core/core.hpp>
#include <core/expected.hpp>
#include <file_io/core.hpp>
#include <file_io/json_utils.hpp>
#include <opengl/glsl.hpp>
#include <opengl/shader.hpp>

#include "load.hpp"

namespace GLSL::Program
{
inline const char * const ASSET_NAME = "glsl_program";

Expected<GL::ShaderProgram, std::string> Convert(LoadedData const & loaded, Managed<Geometry::Layout> const & attrib_layouts);
std::pair<Name, Desc> Parse(File::JSON::JSONObj o, std::filesystem::path const & root_dir);
}