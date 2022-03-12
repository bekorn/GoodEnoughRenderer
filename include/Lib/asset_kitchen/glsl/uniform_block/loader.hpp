#pragma once

namespace GLSL::UniformBlock
{
	struct Description
	{
		std::filesystem::path path;
	};

	LoadedData Load(Description const & description)
	{
		return LoadedData{
			.source = File::LoadAsString(description.path),
		};
	}
}