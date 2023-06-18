#include "book.hpp"

int main(int argc, char * argv[])
{
	if (argc < 2)
	{
		fmt::print(stderr, "First parameter must be the asset directory");
		return 1;
	}
	auto assets_root = std::filesystem::path(argv[1]);
	if (not std::filesystem::exists(assets_root))
	{
		fmt::print(stderr, "\"{}\" does not exist", assets_root);
		return 1;
	}

	Book book(assets_root);


	auto served_root = std::filesystem::path(assets_root).concat("__served");
	std::filesystem::create_directory(served_root);

	fmt::print("Ready to serve some assets to {}!\n", assets_root, served_root);

	return 0;
}