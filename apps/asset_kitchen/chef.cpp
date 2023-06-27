#include "chef.hpp"

#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>

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
	assert_enum_out_of_range();
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

ByteBuffer ConvertAttrib(ByteBuffer const & src, Geometry::Vector const & src_vec, Geometry::Vector const & dst_vec)
{
	auto const vertex_count = src.size / src_vec.size();
	ByteBuffer dst_buffer(dst_vec.size() * vertex_count);

	auto src_ptr = src.data.get();
	auto src_size = src_vec.type.size();
	auto dst_ptr = dst_buffer.data.get();
	auto dst_size = dst_vec.type.size();

	auto copy_count = glm::min(src_vec.dimension, dst_vec.dimension);
	auto fill_count = glm::max(0, dst_vec.dimension - src_vec.dimension);
	f64 fill_vec[] = {0, 0, 0, 0}; // TODO(bekorn): read from attrib layout
	auto skip_count = glm::max(0, src_vec.dimension - dst_vec.dimension);

	for (auto _ = 0; _ < vertex_count; ++_)
	{
		for (auto _ = 0; _ < copy_count; ++_)
		{
			f64 value = ConvertToF64(src_ptr, src_vec.type);
			src_ptr += src_size;

			ConvertFromF64(value, dst_vec.type, dst_ptr);
			dst_ptr += dst_size;
		}
		for (auto i = 0; i < fill_count; ++i)
		{
			ConvertFromF64(fill_vec[i], dst_vec.type, dst_ptr);
			dst_ptr += dst_size;
		}
		src_ptr += src_size * skip_count;
	}

	return dst_buffer;
}

void Chef::prepare_gltf(Book const & book, GLTF::Desc const & desc)
{
	using namespace GLTF;
	using namespace rapidjson;
	using namespace File;
	using namespace File::JSON;

	Document document;
	document.Parse(LoadAsString(desc.path).c_str());


	/// Parse
	vector<ByteBuffer> loaded_buffers;
	vector<BufferView> loaded_buffer_views;
	vector<Accessor> loaded_accessors;
	vector<Mesh> loaded_meshes;
	{
		// Parse buffers
		auto const file_dir = desc.path.parent_path();
		for (auto const & item: document["buffers"].GetArray())
		{
			auto const & buffer = item.GetObject();

			auto file_size = buffer["byteLength"].GetUint64();
			auto file_name = buffer["uri"].GetString();
			// Limitation: only loads separate file binaries
			loaded_buffers.emplace_back(LoadAsBytes(file_dir / file_name, file_size));
		}

		// Parse buffer views
		for (auto const & item: document["bufferViews"].GetArray())
		{
			auto const & buffer_view = item.GetObject();

			loaded_buffer_views.push_back(
				{
					.buffer_index = GetU32(buffer_view, "buffer"),
					.offset = GetU32(buffer_view, "byteOffset", 0),
					.length = GetU32(buffer_view, "byteLength"),
					.stride = GetOptionalU32(buffer_view, "byteStride"),
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

			loaded_accessors.push_back(
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
					attributes.push_back({
						.name = attribute.name.GetString(),
						.accessor_index = attribute.value.GetUint(),
					});

				primitives.push_back({
					.attributes = attributes,
					.indices_accessor_index = GetOptionalU32(primitive, "indices"),
					.material_index = GetOptionalU32(primitive, "material"),
				});
			}

			loaded_meshes.push_back({
				.primitives = primitives,
			});
		}
	}


	/// Load primitives
	// TODO(bekorn): the layout is same for the whole gltf file, it should be more granular, per material perhaps
	assert(desc.layout_name.has_value(), "Default layout not supported yet");
	auto & layout = book.attrib_layouts.get(desc.layout_name.value());

	vector<vector<Geometry::Primitive>> mesh_to_prim;
	mesh_to_prim.reserve(loaded_meshes.size());
	{
		vector<Geometry::Key> loaded_attrib_keys;
		loaded_attrib_keys.reserve(Geometry::ATTRIBUTE_COUNT);
		for (auto & loaded_mesh: loaded_meshes)
		{
			mesh_to_prim.emplace_back();
			auto & primitives = mesh_to_prim.back();
			primitives.reserve(loaded_mesh.primitives.size());

			for (auto & loaded_primitive: loaded_mesh.primitives)
			{
				primitives.emplace_back();
				auto & primitive = primitives.back();

				primitive.layout = &layout;

				auto vertex_count = loaded_accessors[loaded_primitive.attributes[0].accessor_index].count;
				primitive.vertices.init(*primitive.layout, vertex_count);

				auto vertex_offset = 0;

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
					auto & accessor = loaded_accessors[attrib.accessor_index];
					auto & buffer_view = loaded_buffer_views[accessor.buffer_view_index];
					assert(accessor.count == vertex_count, "Attribute has wrong count");

					auto vec = Geometry::Vector(
						IntoAttributeType(accessor.vector_data_type, accessor.normalized),
						accessor.vector_dimension
					);
					auto attrib_location = layout_attrib.location; // also i
					u32 buffer_stride = vec.size();
					auto loaded_buffer = ByteBuffer(buffer_stride * accessor.count);

					if (buffer_view.stride.has_value())
					{
						// strided access
						auto source_stride = buffer_view.stride.value();
						auto source = loaded_buffers[buffer_view.buffer_index]
							.span_as<byte>(buffer_view.offset + accessor.byte_offset, source_stride * accessor.count);

						auto source_ptr = source.data();
						auto buffer_ptr = loaded_buffer.begin();
						auto buffer_end = loaded_buffer.end();
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
						auto source = loaded_buffers[buffer_view.buffer_index]
							.span_as<byte>(buffer_view.offset + accessor.byte_offset, loaded_buffer.size);

						std::memcpy(loaded_buffer.begin(), source.data(), loaded_buffer.size);
					}

					// Convert data if it doesn't match with the layout
					if (vec != layout_attrib.vec)
					{
						fmt::print(
							"!! mesh[{}]:prim[{}] attrib[{}] is {}, expected {} from layout[{}]; converting data...\n",
							loaded_mesh.name, &loaded_primitive - loaded_mesh.primitives.data(), i,
							vec, layout_attrib.vec, desc.layout_name.value().string
						);
						loaded_buffer = ConvertAttrib(loaded_buffer, vec, layout_attrib.vec);
					}

					// Copy to vertex_buffer
					std::memcpy(primitive.vertices.buffer.data.get() + vertex_offset, loaded_buffer.data.get(), loaded_buffer.size);
					vertex_offset += loaded_buffer.size;
				}

				if (loaded_primitive.indices_accessor_index.has_value())
				{
					auto & accessor = loaded_accessors[loaded_primitive.indices_accessor_index.value()];
					auto & buffer_view = loaded_buffer_views[accessor.buffer_view_index];

					auto index_type = IntoAttributeType(accessor.vector_data_type, accessor.normalized);

					// buffers other than vertex attributes are always tightly packed
					// see spec section 3.6.2.1. Overview, paragraph 2
					auto const & buffer = loaded_buffers[buffer_view.buffer_index];
					auto const offset = accessor.byte_offset + buffer_view.offset;
					auto const size = accessor.count * index_type.size();

					primitive.indices.init(accessor.count);
					auto * dst_ptr = primitive.indices.as_span().data();
					if (index_type == Geometry::Type::U8)
					{
						for (auto idx: buffer.span_as<u8>(offset, size))
							*dst_ptr++ = idx;
					}
					else if (index_type == Geometry::Type::U16)
					{
						for (auto idx: buffer.span_as<u16>(offset, size))
							*dst_ptr++ = idx;
					}
					else if (index_type == Geometry::Type::U32)
					{
						for (auto idx: buffer.span_as<u32>(offset, size))
							*dst_ptr++ = idx;
					}
				}
				else
				{
					primitive.indices.init(vertex_count);
					for (auto i = 0; auto & index : primitive.indices.as_span())
						index = i++;
				}
			}
		}
	}


	/// Convert
	ByteBuffer converted_buffer;
	{
		auto & alloc = document.GetAllocator();

		/// Buffer
		{
			auto total_size = 0;
			for (const auto & primitives: mesh_to_prim)
				for (const auto & prim: primitives)
					total_size += prim.vertices.buffer.size + prim.indices.buffer.size;
			converted_buffer = ByteBuffer(total_size);

			Value buffer(kObjectType);
			buffer.AddMember("byteLength", converted_buffer.size, alloc);
			auto uri = desc.path.filename().replace_extension("bin");
			buffer.AddMember("uri", Value(uri.string().c_str(), alloc), alloc);

			Value buffers(kArrayType);
			buffers.PushBack(buffer, alloc);

			document["buffers"] = buffers;
		}


		/// Rest
		// Each mesh will have 2 buffer views for VertexBuffer + IndexBuffer
		// IndexBuffer will have 1 accessors, VertexBuffer will have N for each attribute
		// Each primitive will have "extras/formatted_buffer_views": [vertex_buffer_view_idx, index_buffer_view_idx]
		auto * converted_buffer_ptr = converted_buffer.data_as<byte>();
		Value converted_buffer_views(kArrayType);
		Value converted_accessors(kArrayType);

		auto const get_type = [](u32 dimension) -> const char *
		{
			if (dimension == 1) return "SCALAR";
			if (dimension == 2) return "VEC2";
			if (dimension == 3) return "VEC3";
			if (dimension == 4) return "VEC4";
			if (dimension == 4) return "MAT2"; // TODO(bekorn): :(
			if (dimension == 9) return "MAT3";
			/*if(dimension == 16)*/ return "MAT4";
		};
		auto const get_component_type = [](Geometry::Type::Value const & type) -> int
		{
			// https://javagl.github.io/GLConstantsTranslator/GLConstantsTranslator.html
			using enum Geometry::Type::Value;
			switch (type)
			{
				case F32: return 5126;
				case I8:
				case I8NORM: return 5120;
				case U8:
				case U8NORM: return 5121;
				case I16:
				case I16NORM: return 5122;
				case U16:
				case U16NORM: return 5123;
				case I32:
				case I32NORM: return 5124;
				case U32:
				case U32NORM: return 5125;
			}
			assert_enum_out_of_range();
		};
		auto const get_attrib_name = [](Geometry::Key const & key) -> std::string
		{
			auto & name = key.name;

			if (holds_alternative<std::string>(name))
				return get<std::string>(name).data();

			using enum Geometry::Key::Common;
			switch (get<Geometry::Key::Common>(name))
			{
				case POSITION: return "POSITION";
				case NORMAL: return "NORMAL";
				case TANGENT: return "TANGENT";
				case TEXCOORD: return fmt::format("{}_{}", "TEXCOORD", key.layer);
				case COLOR: return fmt::format("{}_{}", "COLOR", key.layer);
				case JOINTS: return fmt::format("{}_{}", "JOINTS", key.layer);
				case WEIGHTS: return fmt::format("{}_{}", "WEIGHTS", key.layer);
			}
			assert_enum_out_of_range();
		};

		for (auto mesh_idx = 0; auto & item: document["meshes"].GetArray())
		{
			auto const & mesh = item.GetObject();
			auto const & prims = mesh["primitives"].GetArray();
			auto const & loaded_prims = mesh_to_prim[mesh_idx++];

			for (auto i = 0; i < loaded_prims.size(); ++i)
			{
				auto const & prim = prims[i].GetObject();
				auto const & loaded_prim = loaded_prims[i];

				// Edit primitive
				auto accessor_idx = converted_accessors.Size();
				Value attribs(kObjectType);
				for (const auto & attrib: loaded_prim.layout->attributes)
				{
					if (not attrib.is_used()) continue;
					auto key = Value(get_attrib_name(attrib.key).c_str(), alloc);
					attribs.AddMember(key, accessor_idx, alloc);
					accessor_idx++;
				}
				prim["attributes"] = attribs;
				prim["indices"] = accessor_idx;

				// Add buffer views and copy to buffer
				u32 vertex_buffer_view_idx, index_buffer_view_idx;
				usize vertex_buffer_begin, index_buffer_begin;
				{
					// https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#_bufferview_target
					auto const TARGET_ARRAY_BUFFER = 34962;
					auto const TARGET_ELEMENT_ARRAY_BUFFER = 34963;

					vertex_buffer_view_idx = converted_buffer_views.Size();
					vertex_buffer_begin = converted_buffer_ptr - converted_buffer.data.get();

					Value vertex_buffer_view(kObjectType);
					vertex_buffer_view.AddMember("buffer", 0, alloc);
					vertex_buffer_view.AddMember("byteOffset", vertex_buffer_begin, alloc);
					vertex_buffer_view.AddMember("byteLength", loaded_prim.vertices.buffer.size, alloc);
					vertex_buffer_view.AddMember("target", TARGET_ARRAY_BUFFER, alloc);
					converted_buffer_views.PushBack(vertex_buffer_view, alloc);

					std::memcpy(converted_buffer_ptr, loaded_prim.vertices.buffer.data.get(), loaded_prim.vertices.buffer.size);
					converted_buffer_ptr += loaded_prim.vertices.buffer.size;


					index_buffer_view_idx = vertex_buffer_view_idx + 1;
					index_buffer_begin = converted_buffer_ptr - converted_buffer.data.get();
					auto index_buffer_size = loaded_prim.indices.buffer.size;

					Value index_buffer_view(kObjectType);
					index_buffer_view.AddMember("buffer", 0, alloc);
					index_buffer_view.AddMember("byteOffset", index_buffer_begin, alloc);
					index_buffer_view.AddMember("byteLength", index_buffer_size, alloc);
					index_buffer_view.AddMember("target", TARGET_ELEMENT_ARRAY_BUFFER, alloc);
					converted_buffer_views.PushBack(index_buffer_view, alloc);

					std::memcpy(converted_buffer_ptr, loaded_prim.indices.buffer.data.get(), index_buffer_size);
					converted_buffer_ptr += index_buffer_size;
				}
				// Add accessors
				auto vertex_buffer_size = 0;
				{
					for (auto ai = 0; ai < Geometry::ATTRIBUTE_COUNT; ++ai)
					{
						auto const & attrib = loaded_prim.layout->attributes[ai];
						if (not attrib.is_used()) continue;

						Value accessor(kObjectType);
						accessor.AddMember("bufferView", vertex_buffer_view_idx, alloc);
						accessor.AddMember("byteOffset", vertex_buffer_size, alloc);
						accessor.AddMember("componentType", get_component_type(attrib.vec.type), alloc);
						accessor.AddMember("type", Value(get_type(attrib.vec.dimension), alloc), alloc);
						accessor.AddMember("count", loaded_prim.vertices.count, alloc);
						accessor.AddMember("normalized", attrib.vec.type.is_normalized(), alloc);
						converted_accessors.PushBack(accessor, alloc);

						vertex_buffer_size += loaded_prim.vertices.count * attrib.vec.size();
					}
				}
				{
					Value accessor(kObjectType);
					accessor.AddMember("bufferView", index_buffer_view_idx, alloc);
					accessor.AddMember("byteOffset", 0, alloc);
					accessor.AddMember("componentType", get_component_type(Geometry::Type::U32), alloc);
					accessor.AddMember("type", Value(get_type(1), alloc), alloc);
					accessor.AddMember("count", loaded_prim.indices.count, alloc);
					converted_accessors.PushBack(accessor, alloc);
				}

				// Add extras for shortcut
				{
					Value formatted_buffers(kArrayType);
					formatted_buffers.PushBack(vertex_buffer_view_idx, alloc);
					formatted_buffers.PushBack(index_buffer_view_idx, alloc);

					Value extras(kObjectType);
					extras.AddMember("formatted_buffer_views", formatted_buffers, alloc);

					prim.AddMember("extras", extras, alloc);
				}
			}
		}

		document["bufferViews"] = converted_buffer_views;
		document["accessors"] = converted_accessors;


		/// Specify KHR_mesh_quantization extension for the extended data types
		// !!! this extension does not cover all the data types so the served gltf might not be compliant to the spec
		Value::StringRefType const EXTENSION = "KHR_mesh_quantization";

		if (auto iter = document.FindMember("extensionsUsed"); iter != document.MemberEnd())
			iter->value.GetArray().PushBack(EXTENSION, alloc);
		else
			document.AddMember("extensionsUsed", Value(kArrayType).PushBack(EXTENSION, alloc), alloc);

		if (auto iter = document.FindMember("extensionsRequired"); iter != document.MemberEnd())
			iter->value.GetArray().PushBack(EXTENSION, alloc);
		else
			document.AddMember("extensionsRequired", Value(kArrayType).PushBack(EXTENSION, alloc), alloc);
	}


	/// Write to file
	{
		auto const relative_path = std::filesystem::relative(desc.path, book.assets_dir);
		auto const asset_path = book.assets_dir / relative_path;
		auto const asset_dir = asset_path.parent_path();
		auto const served_path = book.served_dir / relative_path;
		auto const served_dir = served_path.parent_path();
		std::filesystem::create_directories(served_dir);

		// JSON
		// see: https://rapidjson.org/md_doc_stream.html#FileWriteStream
		StringBuffer buffer;
		PrettyWriter writer(buffer);
		document.Accept(writer);

		File::WriteString(served_path, {buffer.GetString(), buffer.GetSize()});

		// Binary
		File::WriteBytes(served_dir / relative_path.filename().replace_extension("bin"), converted_buffer);

		// Copy the rest
		for (auto const & dir_entry : std::filesystem::directory_iterator(asset_dir))
		{
			if (dir_entry.is_regular_file())
			{
				auto extension = dir_entry.path().extension();
				if (extension == ".gltf" or extension == ".bin")
					continue;
			}

			auto dst_path = served_dir / std::filesystem::relative(dir_entry.path(), asset_dir);
			using enum std::filesystem::copy_options;
			std::filesystem::copy(dir_entry.path(), dst_path, recursive | skip_existing);
		}
	}
}