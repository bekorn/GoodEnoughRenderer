#pragma once

#include <asset_recipes/gltf/load.hpp>
#include <asset_recipes/gltf/convert.hpp>

namespace GLTF
{
inline const char * const NAME = "gltf";

struct BufferView
{
	u32 buffer_index;
	u32 offset;
	u32 length;
	optional<u32> stride;
};

struct Accessor
{
	u32 buffer_view_index;
	u32 byte_offset;
	u32 vector_data_type;
	u32 vector_dimension;
	u32 count;
	bool normalized;
};

struct Attribute
{
	std::string name;
	u32 accessor_index;
};

// Equivalent of a draw call
struct RawPrimitive
{
	std::string name;
	vector<Attribute> attributes;
	optional<u32> indices_accessor_index;
	optional<u32> material_index;
};

struct RawMesh
{
	std::string name;
	vector<RawPrimitive> primitives;
};


optional<std::string> Serve(LoadedData const & loaded_data, std::filesystem::path path);
};