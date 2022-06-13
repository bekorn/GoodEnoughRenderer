#pragma once

#include "Lib/core/named.hpp"
#include "Lib/file_management/.hpp"
#include "Lib/opengl/glsl.hpp"

#include "loader.hpp"

namespace GLSL::Program
{
	namespace Helpers
	{
		GL::GLenum StringToGLEnum(std::string_view stage)
		{
			if (stage == "vert") return GL::GL_VERTEX_SHADER;
			if (stage == "frag") return GL::GL_FRAGMENT_SHADER;
			if (stage == "geom") return GL::GL_GEOMETRY_SHADER;
		}
	}

	std::pair<Name, Description> Parse(File::JSON::JSONObj o, std::filesystem::path const & root_dir)
	{
		using namespace Helpers;

		auto name = o.FindMember("name")->value.GetString();
		GLSL::Program::Description description;

		for (auto & item : o.FindMember("stages")->value.GetObject())
			description.stages.push_back({
				.stage = StringToGLEnum(item.name.GetString()),
				.path = root_dir / item.value.GetString(),
			});

		for (auto & item: o.FindMember("include_paths")->value.GetArray())
			description.include_paths.push_back(root_dir / item.GetString());

		description.include_strings.push_back(GL::GLSL_VERSION_MACRO);
		for (auto & item: o.FindMember("include_strings")->value.GetArray())
			description.include_strings.emplace_back(item.GetString());

		return {name, description};
	}
}