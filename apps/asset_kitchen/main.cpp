#include "chef.hpp"

int main(int argc, char * argv[])
{
	if (argc < 2)
	{
		fmt::print(stderr, "First parameter must be the asset directory");
		return 1;
	}

	File::Path assets_abs_dir_path;
	{
		auto path = File::Path(argv[1]).make_preferred();

		if (not exists(path))
		{
			fmt::print(stderr, "{} does not exist", path);
			return 1;
		}

		if (path.is_relative())
			path = absolute(path);

		while (path.filename().empty())
			path = path.parent_path();

		assets_abs_dir_path = path;
	}

	Book book(assets_abs_dir_path);
	fmt::print("Assets Directory: {}\n", book.assets_abs_dir_path.string());
	fmt::print("Served Directory: {}\n", book.served_abs_dir_path.string());

	Chef chef;
	chef.prepare_all_gltf(book);

	return 0;
}