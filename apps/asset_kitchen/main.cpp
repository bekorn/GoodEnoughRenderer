#include "book.hpp"
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
		fmt::print(stderr, "\"{}\" does not exist", assets_dir);
		return 1;
	}

	Book book(assets_dir);

	fmt::print("Ready to serve some assets to {}!\n", book.served_dir);

	Chef chef;

	for (auto & [name, gltf]: book.gltf)
		chef.prepare_gltf(book, gltf);

	return 0;
}