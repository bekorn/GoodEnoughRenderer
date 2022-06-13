#pragma once

#include "Lib/core/named.hpp"
#include "Lib/file_management/.hpp"

#include "loader.hpp"

namespace GLTF
{
	std::pair<Name, Description> Parse(File::JSON::JSONObj o, std::filesystem::path const & root_dir)
	{
		auto name = o.FindMember("name")->value.GetString();
		return {
			name,
			{.name = name, .path = root_dir / o.FindMember("path")->value.GetString()}
		};
	}
}