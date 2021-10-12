#pragma once

#include ".pch.hpp"

#include <iostream>

// Spec: https://www.khronos.org/registry/glTF/specs/2.0/glTF-2.0.pdf

namespace GLTF
{
	using ::ByteBuffer;

	struct BufferView
	{
		u32 buffer_index;
		u32 byte_offset;
		u32 byte_length;
	};

	struct Accessor
	{
		u32 buffer_view_index;
		u32 byte_offset;
		u32 vector_data_type; // stores GLenum value
		u32 vector_dimension;
//		u32 count;
	};

	struct Attribute
	{
		std::string name;
		u32 accessor_index;
	};

	// Equivalent of a draw call
	struct Primitive
	{
		std::vector<Attribute> attributes;
		u32 indices_accessor_index; // == u32(-1) if no indices

		bool has_indices() const
		{
			return indices_accessor_index != u32(-1);
		}
	};

	struct Mesh
	{
		std::vector<Primitive> primitives;
		std::string name;
	};

	struct GLTFData
	{
		std::vector<ByteBuffer> buffers;
		std::vector<BufferView> buffer_views;
		std::vector<Accessor> accessors;
		std::vector<Mesh> meshes;
	};


	u32 GetU32(rapidjson::Document::ConstObject const & obj, std::string_view const & key)
	{
		return obj[key.data()].GetUint();
	}

	u32 GetU32(rapidjson::Document::ConstObject const & obj, std::string_view const & key, u32 def_value)
	{
		auto member = obj.FindMember(key.data());
		if (member != obj.MemberEnd())
			return member->value.GetUint();
		else
			return def_value;
	}

	std::string
	GetString(rapidjson::Document::ConstObject const & obj, std::string_view const & key, std::string def_value)
	{
		auto member = obj.FindMember(key.data());
		if (member != obj.MemberEnd())
			return member->value.GetString();
		else
			return def_value;
	}

	// Limitation: Loads mesh[0].primitive[0] only
	GLTFData Load(std::filesystem::path const & file)
	{
		using namespace rapidjson;

		GLTFData gltf_data;

		auto content = LoadAsString(file);

		Document document;
		document.Parse(content.c_str());

//		std::cout << "File contents of " << file << ":\n" << content << '\n';

		auto const file_dir = file.parent_path();
		// Parse buffers
		for (auto const & item: document["buffers"].GetArray())
		{
			auto const & buffer = item.GetObject();

			auto file_size = buffer["byteLength"].GetUint64();
			auto file_name = buffer["uri"].GetString();
			// Limitation: only loads separate file binaries
			gltf_data.buffers.emplace_back(LoadAsBytes(file_dir / file_name, file_size));
		}

		// Parse buffer views
		for (auto const & item: document["bufferViews"].GetArray())
		{
			auto const & buffer_view = item.GetObject();

			// Limitation: no strided view
			gltf_data.buffer_views.push_back(
				{
					.buffer_index = GetU32(buffer_view, "buffer"),
					.byte_offset = GetU32(buffer_view, "byteOffset", 0),
					.byte_length = GetU32(buffer_view, "byteLength"),
				}
			);
		}

		// Parse accessors
		for (auto const & item: document["accessors"].GetArray())
		{
			auto const get_type_dimension = [](std::string const & type) -> u32
			{
				if (type == "SCALAR") return 1;
				if (type == "VEC2") return 2;
				if (type == "VEC3") return 3;
				if (type == "VEC4") return 4;
				if (type == "MAT2") return 4;
				if (type == "MAT3") return 9;
				/*if(type == "MAT4")*/ return 16;
			};

			auto const & accessor = item.GetObject();

			gltf_data.accessors.push_back(
				{
					.buffer_view_index = accessor["bufferView"].GetUint(),
					.vector_data_type = accessor["componentType"].GetUint(),
					.vector_dimension = get_type_dimension(accessor["type"].GetString()),
//					.count = accessor["count"].GetUint(),
				}
			);
		}

		for (auto const & item: document["meshes"].GetArray())
		{
			auto const & mesh = item.GetObject();
			std::cout << "Mesh[" << mesh["name"].GetString() << "]:\n";

			std::vector<Primitive> primitives;
			primitives.reserve(mesh["primitives"].Size());
			for (auto const & item: mesh["primitives"].GetArray())
			{
				auto const & primitive = item.GetObject();

				std::vector<Attribute> attributes;
				attributes.reserve(primitive["attributes"].MemberCount());
				for (auto const & attribute: primitive["attributes"].GetObject())
				{
					attributes.push_back(
						{
							.name = attribute.name.GetString(),
							.accessor_index = attribute.value.GetUint(),
						}
					);
				}

				primitives.push_back(
					{
						.attributes = attributes,
						.indices_accessor_index = GetU32(primitive, "indices", -1),
					}
				);
			}

			gltf_data.meshes.push_back(
				{
					.primitives = primitives,
					.name = GetString(mesh, "name", "<no-name>") // TODO(bekorn): find a default name
				}
			);
		}

		return gltf_data;
	}
}
