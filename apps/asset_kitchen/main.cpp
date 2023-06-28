#include "chef.hpp"

int main(int argc, char * argv[])
{
	if (argc < 2)
	{
		fmt::print(stderr, "First parameter must be the asset directory");
		return 1;
	}
	auto assets_dir = std::filesystem::path(argv[1]);
	if (not std::filesystem::exists(assets_dir))
	{
		fmt::print(stderr, "{} does not exist", assets_dir);
		return 1;
	}

	Book book(assets_dir);

	Chef chef;
	chef.prepare_all_gltf(book);

	return 0;
}