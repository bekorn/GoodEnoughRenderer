#pragma once

#include ".pch.hpp"

#include <iostream>

// Spec: https://www.khronos.org/registry/glTF/specs/2.0/glTF-2.0.pdf

namespace GLTF
{
	using ::Buffer;

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
		gl::GLuint component_type;
		u32 component_size;
		u32 count;
	};

	struct Attribute
	{
		std::string name;
		u32 accessor_index;
	};

	struct GLTFData
	{
		std::vector<Buffer> buffers;
		std::vector<BufferView> buffer_views;
		std::vector<Accessor> accessors;
		std::vector<Attribute> attributes;
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

	// Limitation: Loads mesh[0].primitive[0] only
	GLTFData Load(std::filesystem::path const & file)
	{
		using namespace rapidjson;

		GLTFData gltf_data;

		auto content = LoadAsString(file);

		Document document;
		document.Parse(content.c_str());

		std::cout << "File contents of " << file << ":\n" << content << '\n';

		auto const file_dir = file.parent_path();
		// Parse buffers
		for (auto const & item: document["buffers"].GetArray())
		{
			Document::ConstObject const & buffer = item.GetObject();

			auto file_size = buffer["byteLength"].GetUint64();
			auto file_name = buffer["uri"].GetString();
			// Limitation: only loads separate file binaries
			gltf_data.buffers.emplace_back(LoadAsBytes(file_dir / file_name, file_size));
		}

		// Parse buffer views
		for (auto const & item: document["bufferViews"].GetArray())
		{
			Document::ConstObject const & buffer_view = item.GetObject();

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
			auto const type_size = [](std::string const & type) -> u32
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
					.component_type = accessor["componentType"].GetUint(),
					.component_size = type_size(accessor["type"].GetString()),
					.count = accessor["count"].GetUint(),
				}
			);
		}

		//		for (auto const & mesh : document["meshes"].GetArray())
		auto const & mesh = document["meshes"].GetArray()[0].GetObject();
		{
			std::cout << "Mesh[" << mesh["name"].GetString() << "]:\n";

			// Limitation: single primitive per mesh
			auto & primitive = mesh["primitives"][0];//.GetArray()[0];

			for (auto const & attribute: primitive["attributes"].GetObject())
			{
				gltf_data.attributes.push_back(
					{
						.name = attribute.name.GetString(),
						.accessor_index = attribute.value.GetUint(),
					}
				);
			}
		}

		return gltf_data;
	}
}
