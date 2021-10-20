#pragma once

#include ".pch.hpp"

#include <filesystem>
#include <iostream>

namespace GLTF
{
	// Spec: https://www.khronos.org/registry/glTF/specs/2.0/glTF-2.0.pdf

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
		bool normalized;
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

	struct Image
	{
		ByteBuffer data;
		i32x2 dimensions;
		i32 channels;
	};

	struct Sampler
	{
		u32 min_filter;
		u32 mag_filter;
		u32 wrap_s;
		u32 wrap_t;
	};

	struct Texture
	{
		u32 image_index; // == u32(-1) if default
		u32 sampler_index; // == u32(-1) if default

		bool is_default_image() const
		{
			return image_index == u32(-1);
		}

		bool is_default_sampler() const
		{
			return sampler_index == u32(-1);
		}
	};

	struct GLTFData
	{
		std::vector<ByteBuffer> buffers;
		std::vector<BufferView> buffer_views;

		std::vector<Image> images;
		std::vector<Sampler> samplers;
		std::vector<Texture> textures;

		std::vector<Accessor> accessors;
		std::vector<Mesh> meshes;
	};

	namespace JSONHelpers
	{
		using JSONObj = rapidjson::Document::ConstObject const &;
		using Key = std::string_view const &;

		u32 GetU32(JSONObj obj, Key key)
		{
			return obj[key.data()].GetUint();
		}

		u32 GetU32(JSONObj obj, Key key, u32 def_value)
		{
			auto member = obj.FindMember(key.data());
			if (member != obj.MemberEnd())
				return member->value.GetUint();
			else
				return def_value;
		}

		std::string GetString(JSONObj obj, Key key, std::string const & def_value)
		{
			auto member = obj.FindMember(key.data());
			if (member != obj.MemberEnd())
				return member->value.GetString();
			else
				return def_value;
		}

		bool GetBool(JSONObj obj, Key key, bool def_value)
		{
			auto member = obj.FindMember(key.data());
			if (member != obj.MemberEnd())
				return member->value.GetBool();
			else
				return def_value;
		}
	}

	// Limitation: Loads mesh[0].primitive[0] only
	GLTFData Load(std::filesystem::path const & file)
	{
		using namespace rapidjson;
		using namespace JSONHelpers;

		GLTFData gltf_data;

		Document document;
		document.Parse(LoadAsString(file).c_str());

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

		// Parse images
		if (auto const member = document.FindMember("images"); member != document.MemberEnd())
		{
			for (auto const & item: member->value.GetArray())
			{
				auto const & image = item.GetObject();

				if (auto const member = image.FindMember("uri"); member != image.MemberEnd())
				{
					auto uri = member->value.GetString();

					if (uri[5] != ':') // check for "data:" (base64 encoded data a json string)
					{
						auto const file_data = LoadAsBytes(file_dir / uri);
						i32x2 dimensions;
						i32 channels;
						void* raw_pixel_data = stbi_load_from_memory(
							file_data.data_as<const unsigned char>(), file_data.size,
							&dimensions.x, &dimensions.y,
							&channels, 0
						);

						auto data = ByteBuffer(
							raw_pixel_data,
							dimensions.x * dimensions.y * channels
						);

						gltf_data.images.push_back(
							{
								.data = std::move(data),
								.dimensions = dimensions,
								.channels = channels,
							}
						);
						continue;
					}
				}

				throw std::runtime_error("images without a uri file path are not supported yet");
			}
		}

		// Parse samplers
		if (auto const member = document.FindMember("samplers"); member != document.MemberEnd())
		{
			for (auto const & item: member->value.GetArray())
			{
				auto const & sampler = item.GetObject();

				// Min/Mag filters have no default values in the spec, I picked the values
				gltf_data.samplers.push_back(
					Sampler{
						.min_filter = GetU32(sampler, "minFilter", 9729), // def is LINEAR
						.mag_filter = GetU32(sampler, "magFilter", 9729), // def is LINEAR
						.wrap_s = GetU32(sampler, "wrapS", 10497), // def is REPEAT
						.wrap_t = GetU32(sampler, "wrapT", 10497), // def is REPEAT
					}
				);
			}
		}

		// Parse textures
		if (auto const member = document.FindMember("textures"); member != document.MemberEnd())
		{
			for (auto const & item: member->value.GetArray())
			{
				auto const & texture = item.GetObject();

				gltf_data.textures.push_back(
					{
						.image_index = GetU32(texture, "source", u32(-1)),
						.sampler_index = GetU32(texture, "sampler", u32(-1)),
					}
				);
			}
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
					.normalized = GetBool(accessor, "normalized", false),
				}
			);
		}

		// Parse meshes
		for (auto const & item: document["meshes"].GetArray())
		{
			auto const & mesh = item.GetObject();

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
