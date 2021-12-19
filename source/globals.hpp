#pragma once

#include <filesystem>

// TODO(bekorn): pass this without globals
struct GlobalState
{
	std::filesystem::path const test_assets = R"(E:\Users\Berk\Desktop\Projeler\Portfolio\GoodEnoughRenderer\test_assets)";
} global_state;