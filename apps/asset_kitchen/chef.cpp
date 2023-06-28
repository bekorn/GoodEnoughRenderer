#include "chef.hpp"

void Chef::prepare_all_gltf(Book const & book)
{
	for (auto & [name, desc]: book.gltf)
		GLTF::Serve(book, desc);
}