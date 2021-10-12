#pragma once

#include <filesystem>
#include <fstream>

#include "Lib/core/core_types.hpp"

ByteBuffer LoadAsBytes(std::filesystem::path const & path)
{
	assert(std::filesystem::exists(path));
	std::basic_ifstream<byte> file(path, std::ios::in | std::ios::binary | std::ios::ate);

	usize file_size = file.tellg();
	ByteBuffer buffer(file_size);

	file.seekg(0);
	file.read(buffer.data.get(), file_size);

	return buffer;
}

ByteBuffer LoadAsBytes(std::filesystem::path const & path, usize file_size)
{
	assert(std::filesystem::exists(path));
	std::basic_ifstream<byte> file(path, std::ios::in | std::ios::binary);

	ByteBuffer buffer(file_size);
	file.read(buffer.data.get(), file_size);

	return buffer;
}

std::string LoadAsString(std::filesystem::path const & path)
{
	assert(std::filesystem::exists(path));
	std::basic_ifstream<char> file(path, std::ios::in | std::ios::binary | std::ios::ate);

	usize file_size = file.tellg();
	std::string buffer(file_size, '\0');

	file.seekg(0);
	file.read(buffer.data(), file_size);

	return buffer;
}
