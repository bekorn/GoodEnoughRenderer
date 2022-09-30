#pragma once
#pragma message("----Read ASSET/PROGRAM/convert.Hpp----")

#include "Lib/core/core.hpp"
#include "Lib/core/expected.hpp"
#include "Lib/file_management/core.hpp"
#include "Lib/file_management/json_utils.hpp"
#include "Lib/opengl/glsl.hpp"
#include "Lib/opengl/shader.hpp"

#include "load.hpp"

namespace GLSL::Program
{
	Expected<GL::ShaderProgram, std::string> Convert(LoadedData const & loaded);

	std::pair<Name, Description> Parse(File::JSON::JSONObj o, std::filesystem::path const & root_dir);
}