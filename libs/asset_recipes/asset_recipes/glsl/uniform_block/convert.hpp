#pragma once
#pragma message("-- read ASSET/BLOCK/convert.Hpp --")

#include "load.hpp"

#include <core/expected.hpp>
#include <file_io/core.hpp>
#include <file_io/json_utils.hpp>
#include <opengl/uniform_block.hpp>

namespace GLSL::UniformBlock
{
inline const char * const ASSET_NAME = "glsl_uniform_block";

Expected<GL::UniformBlock, std::string> Convert(LoadedData const & loaded);
std::pair<Name, Desc> Parse(File::JSON::ConstObj o, std::filesystem::path const & root_dir);
}