#pragma once

#include "Lib/core/named.hpp"
#include "Lib/file_management/.hpp"

#include "loader.hpp"

namespace GLSL::UniformBlock
{
	std::pair<Name, Description> Parse(File::JSON::JSONObj o, std::filesystem::path const & root_dir)
	{
		return {
			o.FindMember("name")->value.GetString(),
			{.path = root_dir / o.FindMember("path")->value.GetString()}
		};
	}
}