#pragma once
#pragma message("-- read ASSET/PROGRAM/load.Hpp --")

#include <core/core.hpp>
#include <core/named.hpp>
#include "Lib/opengl/core.hpp"

namespace GLSL::Program
{
struct Desc
{
	Name layout_name;
	struct Stage
	{
		GL::GLenum stage;
		std::filesystem::path path;
	};
	vector<Stage> stages;
	// TODO(bekorn): The order will be important in a case in the future and this api will fail then :/
	vector<std::string_view> include_strings;
	vector<std::filesystem::path> include_paths;
};

struct LoadedData
{
	Name layout_name;
	struct Stage
	{
		GL::GLenum type;
		std::string source;
	};
	vector<Stage> stages;
	vector<std::string> includes;
};

LoadedData Load(Desc const & desc);
}
