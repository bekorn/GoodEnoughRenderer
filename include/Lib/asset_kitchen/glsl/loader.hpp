#pragma once

#include "Lib/file_management/core.hpp"

#include "core.hpp"

namespace GLSL
{
	struct Description
	{
		struct Stage
		{
			GL::GLenum stage;
			std::filesystem::path path;
		};
		vector<Stage> stages;
		// TODO(bekorn): The order will be important in a case in the future and this api will fail then :/
		vector<std::filesystem::path> include_paths;
		vector<std::string_view> include_strings;
	};

	Data Load(Description const & description)
	{
		Data data;

		data.stages.reserve(description.stages.size());
		for (auto const & [stage, path]: description.stages)
			data.stages.emplace_back(Stage{
				.type = stage,
				.source = File::LoadAsString(path),
			});

		data.includes.reserve(description.include_paths.size() + description.include_strings.size());
		for (auto const & path: description.include_paths)
			data.includes.emplace_back(File::LoadAsString(path));
		for (auto const & sv: description.include_strings)
			data.includes.emplace_back(sv);

		return data;
	}
}
