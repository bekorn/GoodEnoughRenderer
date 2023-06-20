#pragma once

#include <asset_recipes/gltf/load.hpp>
#include <asset_recipes/gltf/convert.hpp>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace GLTF
{
inline const char * const NAME = "gltf";

optional<std::string> Serve(LoadedData const & loaded_data, std::filesystem::path path)
{
	using namespace rapidjson;
	using namespace File;
	using namespace File::JSON;

	Document document;
	auto & alloc = document.GetAllocator();
	document.SetObject();


	/// Serialize GLTF
	document.AddMember("testing", 123, alloc);
	// TODO(bekorn): implement gltf serializing


	/// Write to file
	// see: https://rapidjson.org/md_doc_stream.html#FileWriteStream
	StringBuffer buffer;
	Writer writer(buffer);
	document.Accept(writer);

	std::filesystem::create_directories(path.parent_path());
	File::WriteString(path, {buffer.GetString(), buffer.GetSize()});

	return {};
}
};