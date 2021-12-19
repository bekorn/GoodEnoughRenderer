#pragma once

#include <iostream>
#include <execution>

#include "Lib/file_management/.hpp"

#include "core.hpp"

namespace GLTF
{
	GLTFData Load(std::filesystem::path const & file)
	{
		using namespace rapidjson;
		using namespace File;
		using namespace File::JSON;

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
					.offset = GetU32(buffer_view, "byteOffset", 0),
					.length = GetU32(buffer_view, "byteLength"),
					.stride = GetOptionalU32(buffer_view, "byteStride"),
				}
			);
		}

		// Parse images
		if (auto const member = document.FindMember("images"); member != document.MemberEnd())
		{
			auto const & items = member->value.GetArray();
			gltf_data.images.resize(items.Size());
			std::transform(
				std::execution::par_unseq,
				items.Begin(), items.End(),
				gltf_data.images.data(),
				[&file_dir](Document::Array::ValueType const & item) -> GLTF::Image
				{
					auto const & image = item.GetObject();

					auto const member = image.FindMember("uri");
					if (member == image.MemberEnd())
						throw std::runtime_error("images without a uri file path are not supported yet");

					auto uri = member->value.GetString();
					if (uri[5] == ':') // check for "data:" (base64 encoded data as a json string)
						throw std::runtime_error("images without a uri file path are not supported yet");

					auto const file_data = LoadAsBytes(file_dir / uri);
					i32x2 dimensions;
					i32 channels;
					void* raw_pixel_data = stbi_load_from_memory(
						file_data.data_as<const unsigned char>(), file_data.size,
						&dimensions.x, &dimensions.y,
						&channels, 0
					);

					auto data = ByteBuffer(
						move(raw_pixel_data),
						dimensions.x * dimensions.y * channels
					);

					return {
						.data = move(data),
						.dimensions = dimensions,
						.channels = channels,
					};
				}
			);
		}

		// Parse samplers
		if (auto const member = document.FindMember("samplers"); member != document.MemberEnd())
		{
			for (auto const & item: member->value.GetArray())
			{
				auto const & sampler = item.GetObject();

				// Min/Mag filters have no default values in the spec, I picked the values
				gltf_data.samplers.push_back(
					{
						.min_filter = GetU32(sampler, "minFilter", SamplerDefault.min_filter),
						.mag_filter = GetU32(sampler, "magFilter", SamplerDefault.mag_filter),
						.wrap_s = GetU32(sampler, "wrapS", SamplerDefault.wrap_s),
						.wrap_t = GetU32(sampler, "wrapT", SamplerDefault.wrap_t),
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
						.image_index = GetOptionalU32(texture, "source"),
						.sampler_index = GetOptionalU32(texture, "sampler"),
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
					.count = accessor["count"].GetUint(),
					.normalized = GetBool(accessor, "normalized", false),
				}
			);
		}

		// Parse meshes
		for (auto const & item: document["meshes"].GetArray())
		{
			auto const & mesh = item.GetObject();

			vector<Primitive> primitives;
			primitives.reserve(mesh["primitives"].Size());
			for (auto const & item: mesh["primitives"].GetArray())
			{
				auto const & primitive = item.GetObject();

				vector<Attribute> attributes;
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
						.indices_accessor_index = GetOptionalU32(primitive, "indices"),
						.material_index = GetOptionalU32(primitive, "material"),
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

		// Parse materials
		for (auto const & item: document["materials"].GetArray())
		{
			auto const get_tex_info = [](JSONObj material, Key key) -> optional<Material::TexInfo>
			{
				auto member = material.FindMember(key.data());
				if (member != material.MemberEnd())
				{
					auto tex_info = member->value.GetObject();
					return Material::TexInfo{
						.texture_index = GetU32(tex_info, "index"),
						.texcoord_index = GetU32(tex_info, "texCoord", 0),
					};
				}
				else
					return {};
			};

			auto const & material = item.GetObject();
			Material mat{
				.normal_texture = get_tex_info(material, "normalTexture"),

				.occlusion_texture = get_tex_info(material, "occlusionTexture"),

				.emissive_texture = get_tex_info(material, "emissiveTexture"),
				.emissive_factor = GetF32x3(material, "emissiveFactor", {0, 0, 0}),

				.alpha_mode = GetString(material, "alphaMode", "OPAQUE"),
				.alpha_cutoff = GetF32(material, "alphaCutoff", 0.5),

				.double_sided = GetBool(material, "doubleSided", false),
			};

			if (auto member = material.FindMember("pbrMetallicRoughness"); member != material.MemberEnd())
			{
				auto const & pbrMetallicRoughness = member->value.GetObject();
				mat.pbr_metallic_roughness = {
					.base_color_factor = GetF32x4(pbrMetallicRoughness, "baseColorFactor", {1, 1, 1, 1}),
					.base_color_texture = get_tex_info(pbrMetallicRoughness, "baseColorTexture"),
					.metallic_factor = GetF32(pbrMetallicRoughness, "metallicFactor", 1),
					.roughness_factor = GetF32(pbrMetallicRoughness, "roughnessFactor", 1),
					.metallic_roughness_texture = get_tex_info(pbrMetallicRoughness, "metallicRoughnessTexture"),
				};
			}

			gltf_data.materials.push_back(mat);
		}

		return gltf_data;
	}
}
