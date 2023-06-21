#pragma once

#include "book.hpp"

#include <asset_recipes/gltf/load.hpp>

// Pattern: String into Geometry::Attribute::Key
Geometry::Key IntoAttributeKey(std::string_view name)
{
	using namespace Geometry;

	auto const IntoCommon = [](std::string_view attribute_name) -> optional<Key::Common>
	{
		using enum Key::Common;
		if (attribute_name == "POSITION") return POSITION;
		if (attribute_name == "NORMAL") return NORMAL;
		if (attribute_name == "TANGENT") return TANGENT;
		if (attribute_name == "TEXCOORD") return TEXCOORD;
		if (attribute_name == "COLOR") return COLOR;
		return nullopt;
	};

	static std::regex const attribute_pattern("(_)?(.*?)(_\\d+)?");

	std::cmatch match;
	regex_match(
		name.data(), name.data() + name.size(),
		match, attribute_pattern
	);

	Key key;

	if (match[1].matched) // is custom
	{
		key.name = match[2].str();
	}
	else
	{
		auto common_name = IntoCommon(std::string_view(match[2].first, match[2].second));
		if (common_name.has_value())
			key.name = common_name.value();
		else
			key.name = match[2].str();
	}

	key.layer = 0;
	if (match[3].matched) // has a layer
		std::from_chars(match[3].first + 1, match[3].second, key.layer);

	return key;
}

Geometry::Type IntoAttributeType(u32 type, bool is_normalized)
{
	// see spec section 3.6.2.2. Accessor Data Types
	using enum Geometry::Type::Value;

	if (is_normalized)
		switch (type)
		{
			case 5120: return {I8NORM};
			case 5121: return {U8NORM};
			case 5122: return {I16NORM};
			case 5123: return {U16NORM};
			case 5125: return {U32NORM};
		}
	else
		switch (type)
		{
			case 5120: return {I8};
			case 5121: return {U8};
			case 5122: return {I16};
			case 5123: return {U16};
			case 5125: return {U32};
			case 5126: return {F32};
		}

	// above values are all the alloved ones therefore,
	assert_enum_out_of_range();
}

f64 ConvertToF64(byte * src, Geometry::Type const & type)
{
	using enum Geometry::Type::Value;
	switch(type)
	{
		case F32:		return f64(*reinterpret_cast<f32*>(src));
		case I8:		return f64(*reinterpret_cast<i8*>(src));
		case I16:		return f64(*reinterpret_cast<i16*>(src));
		case I32:		return f64(*reinterpret_cast<i32*>(src));
		case I8NORM:	return f64(*reinterpret_cast<i8*>(src))  / std::numeric_limits<i8>::max();
		case I16NORM:	return f64(*reinterpret_cast<i16*>(src)) / std::numeric_limits<i16>::max();
		case I32NORM:	return f64(*reinterpret_cast<i32*>(src)) / std::numeric_limits<i32>::max();
		case U8:		return f64(*reinterpret_cast<u8*>(src));
		case U16:		return f64(*reinterpret_cast<u16*>(src));
		case U32:		return f64(*reinterpret_cast<u32*>(src));
		case U8NORM:	return f64(*reinterpret_cast<u8*>(src))  / std::numeric_limits<u8>::max();
		case U16NORM:	return f64(*reinterpret_cast<u16*>(src)) / std::numeric_limits<u16>::max();
		case U32NORM:	return f64(*reinterpret_cast<u32*>(src)) / std::numeric_limits<u32>::max();
	}
}

void ConvertFromF64(f64 src, Geometry::Type const & type, byte * dst)
{
	using enum Geometry::Type::Value;
	switch(type)
	{
		case F32:		*reinterpret_cast<f32*>(dst) = src; break;
		case I8:		*reinterpret_cast<i8*>(dst)  = src; break;
		case I16:		*reinterpret_cast<i16*>(dst) = src; break;
		case I32:		*reinterpret_cast<i32*>(dst) = src; break;
		case I8NORM:	*reinterpret_cast<i8*>(dst)  = src * std::numeric_limits<i8>::max();  break;
		case I16NORM:	*reinterpret_cast<i16*>(dst) = src * std::numeric_limits<i16>::max(); break;
		case I32NORM:	*reinterpret_cast<i32*>(dst) = src * std::numeric_limits<i32>::max(); break;
		case U8:		*reinterpret_cast<u8*>(dst)  = src; break;
		case U16:		*reinterpret_cast<u16*>(dst) = src; break;
		case U32:		*reinterpret_cast<u32*>(dst) = src; break;
		case U8NORM:	*reinterpret_cast<u8*>(dst)  = src * std::numeric_limits<u8>::max();  break;
		case U16NORM:	*reinterpret_cast<u16*>(dst) = src * std::numeric_limits<u16>::max(); break;
		case U32NORM:	*reinterpret_cast<u32*>(dst) = src * std::numeric_limits<u32>::max(); break;
	}
}

struct Chef
{
	void prepare_gltf(Book const & book, GLTF::Desc const & desc)
	{
		using namespace rapidjson;
		using namespace File;
		using namespace File::JSON;

		GLTF::LoadedData loaded;

		Document document;
		document.Parse(LoadAsString(desc.path).c_str());

		auto const file_dir = desc.path.parent_path();

		/// Parse
		// Parse buffers
		for (auto const & item: document["buffers"].GetArray())
		{
			auto const & buffer = item.GetObject();

			auto file_size = buffer["byteLength"].GetUint64();
			auto file_name = buffer["uri"].GetString();
			// Limitation: only loads separate file binaries
			loaded.buffers.emplace_back(LoadAsBytes(file_dir / file_name, file_size));
		}

		// Parse buffer views
		for (auto const & item: document["bufferViews"].GetArray())
		{
			auto const & buffer_view = item.GetObject();

			loaded.buffer_views.push_back({
				.buffer_index = GetU32(buffer_view, "buffer"),
				.offset = GetU32(buffer_view, "byteOffset", 0),
				.length = GetU32(buffer_view, "byteLength"),
				.stride = GetOptionalU32(buffer_view, "byteStride"),
			});
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

			loaded.accessors.push_back(
				{
					.buffer_view_index = accessor["bufferView"].GetUint(),
					.byte_offset = GetU32(accessor, "byteOffset", 0),
					.vector_data_type = accessor["componentType"].GetUint(),
					.vector_dimension = get_type_dimension(accessor["type"].GetString()),
					.count = accessor["count"].GetUint(),
					.normalized = GetBool(accessor, "normalized", false),
				}
			);
		}

		// Parse meshes
		NameGenerator mesh_name_generator{.prefix = desc.name + ":mesh:"};
		NameGenerator primitive_name_generator{.prefix = desc.name + ":primitive:"};
		for (auto const & item: document["meshes"].GetArray())
		{
			auto const & mesh = item.GetObject();

			vector<GLTF::Primitive> primitives;
			primitives.reserve(mesh["primitives"].Size());
			for (auto const & item: mesh["primitives"].GetArray())
			{
				auto const & primitive = item.GetObject();

				vector<GLTF::Attribute> attributes;
				attributes.reserve(primitive["attributes"].MemberCount());
				for (auto const & attribute: primitive["attributes"].GetObject())
					attributes.push_back({
						.name = attribute.name.GetString(),
						.accessor_index = attribute.value.GetUint(),
					});

				primitives.push_back({
					.name = primitive_name_generator.get(primitive, "name"),
					.attributes = attributes,
					.indices_accessor_index = GetOptionalU32(primitive, "indices"),
					.material_index = GetOptionalU32(primitive, "material"),
				});
			}

			loaded.meshes.push_back({
				.name = mesh_name_generator.get(mesh, "name"),
				.primitives = primitives,
			});
		}


		/// Convert primitives
		// TODO(bekorn): the layout is same for the whole gltf file, it should be more granular, per material perhaps
		assert(desc.layout_name.has_value(), "Default layout not supported yet");
		auto & layout = book.attrib_layouts.get(desc.layout_name.value());

		std::unordered_map<std::string, vector<Geometry::Primitive>> mesh_to_prim;
		mesh_to_prim.reserve(5);

		vector<Geometry::Key> loaded_attrib_keys;
		loaded_attrib_keys.reserve(Geometry::ATTRIBUTE_COUNT);
		for (auto & loaded_mesh: loaded.meshes)
		{
			auto & primitives = mesh_to_prim[loaded_mesh.name];

			for (auto & loaded_primitive: loaded_mesh.primitives)
			{
				primitives.emplace_back();
				auto & primitive = primitives.back();

				primitive.layout = &layout;

				assert(loaded_primitive.attributes.size() < Geometry::ATTRIBUTE_COUNT, "Primitive has too many attributes");
				loaded_attrib_keys.clear();
				for (auto & attrib: loaded_primitive.attributes)
					loaded_attrib_keys.emplace_back(IntoAttributeKey(attrib.name));

				for (auto i = 0; i < Geometry::ATTRIBUTE_COUNT; ++i)
				{
					auto & layout_attrib = primitive.layout->attributes[i];
					if (not layout_attrib.is_used()) continue;

					auto attrib_key_iter = std::ranges::find(loaded_attrib_keys, layout_attrib.key);
					assert(attrib_key_iter != loaded_attrib_keys.end(), "Primitive is missing a layout attribute");

					auto attrib_idx = attrib_key_iter - loaded_attrib_keys.begin();
					auto & attrib = loaded_primitive.attributes[attrib_idx];
					auto & accessor = loaded.accessors[attrib.accessor_index];
					auto & buffer_view = loaded.buffer_views[accessor.buffer_view_index];

					Geometry::Vector vec(
						IntoAttributeType(accessor.vector_data_type, accessor.normalized),
						accessor.vector_dimension
					);
					auto attrib_location = layout_attrib.location; // also i
					auto & buffer = primitive.data.buffers[attrib_location];
					u32 buffer_stride = vec.size();
					buffer = ByteBuffer(buffer_stride * accessor.count);

					if (buffer_view.stride.has_value())
					{
						// strided access
						auto source_stride = buffer_view.stride.value();
						auto source = loaded.buffers[buffer_view.buffer_index]
							.span_as<byte>(buffer_view.offset + accessor.byte_offset, source_stride * accessor.count);

						auto source_ptr = source.data();
						auto buffer_ptr = buffer.begin();
						auto buffer_end = buffer.end();
						while (buffer_ptr != buffer_end)
						{
							std::memcpy(buffer_ptr, source_ptr, buffer_stride);
							source_ptr += source_stride;
							buffer_ptr += buffer_stride;
						}
					}
					else
					{
						// contiguous access (tightly packed data)
						auto source = loaded.buffers[buffer_view.buffer_index]
							.span_as<byte>(buffer_view.offset + accessor.byte_offset, buffer.size);

						std::memcpy(buffer.begin(), source.data(), buffer.size);
					}

					// Convert data if it doesn't match with the layout
					if (vec != layout_attrib.vec)
					{
						fmt::print(
							"!! mesh[{}]:prim[{}] attrib[{}] is {}, expected {} from layout[{}]; converting data...\n",
							loaded_mesh.name, &loaded_primitive - loaded_mesh.primitives.data(), i,
							vec, layout_attrib.vec, desc.layout_name.value().string
						);

						auto & converted_vec = layout_attrib.vec;
						ByteBuffer converted_buffer(converted_vec.size() * accessor.count);

						auto src_ptr = buffer.data_as<byte>();
						auto src_size = vec.type.size();
						auto dst_ptr = converted_buffer.data_as<byte>();
						auto dst_size = converted_vec.type.size();

						auto copy_count = glm::min(vec.dimension, converted_vec.dimension);
						auto fill_count = glm::max(0, converted_vec.dimension - vec.dimension);
						f64 fill_vec[] = {0, 0, 0, 0}; // TODO(bekorn): read from attrib layout
						auto skip_count = glm::max(0, vec.dimension - converted_vec.dimension);

						for (auto _ = 0; _ < accessor.count; ++_)
						{
							for (auto _ = 0; _ < copy_count; ++_)
							{
								f64 value = ConvertToF64(src_ptr, vec.type);
								src_ptr += src_size;

								ConvertFromF64(value, converted_vec.type, dst_ptr);
								dst_ptr += dst_size;
							}
							for (auto i = 0; i < fill_count; ++i)
							{
								ConvertFromF64(fill_vec[i], converted_vec.type, dst_ptr);
								dst_ptr += dst_size;
							}
							src_ptr += src_size * skip_count;
						}

						buffer = move(converted_buffer);
					}
				}

				if (loaded_primitive.indices_accessor_index.has_value())
				{
					auto & accessor = loaded.accessors[loaded_primitive.indices_accessor_index.value()];
					auto & buffer_view = loaded.buffer_views[accessor.buffer_view_index];

					auto index_type = IntoAttributeType(accessor.vector_data_type, accessor.normalized);

					// buffers other than vertex attributes are always tightly packed
					// see spec section 3.6.2.1. Overview, paragraph 2
					if (index_type == Geometry::Type::U8)
					{
						auto source = loaded.buffers[buffer_view.buffer_index]
							.span_as<u8>(accessor.byte_offset + buffer_view.offset, accessor.count * index_type.size());

						primitive.indices.reserve(source.size());
						for (auto index: source)
							primitive.indices.emplace_back(index);
					}
					else if (index_type == Geometry::Type::U16)
					{
						auto source = loaded.buffers[buffer_view.buffer_index]
							.span_as<u16>(accessor.byte_offset + buffer_view.offset, accessor.count * index_type.size());

						primitive.indices.reserve(source.size());
						for (auto index: source)
							primitive.indices.emplace_back(index);
					}
					else if (index_type == Geometry::Type::U32)
					{
						auto source = loaded.buffers[buffer_view.buffer_index]
							.span_as<u32>(accessor.byte_offset + buffer_view.offset, accessor.count * index_type.size());

						primitive.indices.reserve(source.size());
						for (auto index: source)
							primitive.indices.emplace_back(index);
					}
				}
				else
				{
					// assuming all the attributes have the same count
					auto & accessor = loaded.accessors[loaded_primitive.attributes[0].accessor_index];
					auto vertex_count = accessor.count;

					primitive.indices.resize(vertex_count);
					for (u32 i = 0; i < vertex_count; ++i)
						primitive.indices[i] = i;
				}
			}
		}

		/// Prepare attribute buffers
		for (const auto & mesh: loaded.meshes)
		{
			for (const auto & prim: mesh.primitives)
			{
//				prim.
				fmt::print("Mesh: {}, prim_count: {}\n", mesh.name, mesh.primitives.size());

			}
		}
	}
};