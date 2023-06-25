#pragma once

#include "book.hpp"

#include <asset_recipes/gltf/load.hpp>

struct Chef
{
	void prepare_gltf(Book const & book, GLTF::Desc const & desc);
};