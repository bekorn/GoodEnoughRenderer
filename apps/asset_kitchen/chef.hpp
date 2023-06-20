#pragma once

#include "book.hpp"

#include <asset_recipes/gltf/load.hpp>

struct Chef
{
	void prepare_gltf(Book const & book, GLTF::Desc const & desc)
	{
		fmt::print("Preparing \"{}\"\n", desc.name);

		GLTF::LoadedData loaded_data = GLTF::Load(desc);

		// TODO(bekorn): do meaningful changes here

		auto serve_path = book.served_dir / std::filesystem::relative(desc.path, book.assets_dir);
		fmt::print("Serving gltf to {}\n", serve_path);
		if (auto error = GLTF::Serve(loaded_data, serve_path))
			fmt::print(stderr, "Error serving gltf: {}\n", error.value());
	}
};