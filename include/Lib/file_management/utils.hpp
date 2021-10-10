#pragma once

#include <filesystem>
#include <fstream>

#include "core_types.hpp"

/*
Buffer<byte> LoadAsBytes(std::filesystem::path const & path)
{
	assert(std::filesystem::exists(path));
	std::basic_ifstream<byte> file(path, std::ios::in | std::ios::binary | std::ios::ate);

	usize file_size = file.tellg();
	Buffer<byte> buffer(file_size);

	file.seekg(0);
	file.read(buffer.data.get(), file_size);

	return buffer;
}

Buffer<byte> LoadAsBytes(std::filesystem::path const & path, usize file_size)
{
	assert(std::filesystem::exists(path));
	std::basic_ifstream<byte> file(path, std::ios::in | std::ios::binary);

	Buffer<byte> buffer(file_size);
	file.read(buffer.data.get(), file_size);

	return buffer;
}

Buffer<char> LoadAsChars(std::filesystem::path const & path)
{
	assert(std::filesystem::exists(path));
	std::ifstream file(path, std::ios::in | std::ios::binary | std::ios::ate);

	usize file_size = file.tellg();
	Buffer<char> buffer(file_size / sizeof(char));

	file.seekg(0);
	file.read(buffer.data.get(), file_size);

	return buffer;
}
*/

Buffer LoadAsBytes(std::filesystem::path const & path)
{
	assert(std::filesystem::exists(path));
	std::basic_ifstream<byte> file(path, std::ios::in | std::ios::binary | std::ios::ate);

	usize file_size = file.tellg();
	Buffer buffer(file_size);

	file.seekg(0);
	file.read(buffer.data.get(), file_size);

	return buffer;
}


Buffer LoadAsBytes(std::filesystem::path const & path, usize file_size)
{
	assert(std::filesystem::exists(path));
	std::basic_ifstream<byte> file(path, std::ios::in | std::ios::binary);

	Buffer buffer(file_size);
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
