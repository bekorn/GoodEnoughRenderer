#include "chef.hpp"

void Chef::prepare_all_gltf(Book const & book)
{
	for (auto & [name, desc]: book.gltf)
	{
		// skip already served assets
		if (exists(book.served_dir / desc.path))
		{
			auto relative_path = std::filesystem::relative(desc.path, book.assets_dir);

			std::error_code _;
			auto asset_last_change = last_write_time(book.assets_dir / relative_path, _);
			auto served_last_change = last_write_time(book.served_dir / relative_path, _);

			if (served_last_change > asset_last_change)
				continue;
		}

		GLTF::Serve(book, desc);
	}
}