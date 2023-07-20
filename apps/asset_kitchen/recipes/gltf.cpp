#include "gltf.hpp"
#include "book.hpp"
#include "../tools/image_compression.hpp"

#include <core/intrinsics.hpp>
#include <file_io/core.hpp>
#include <file_io/json_utils.hpp>

namespace
{
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

// TODO(bekorn): maybe move this into lib/core/geometry.cpp
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

const char * to_string(GLTF::EncodedImage::MimeType type)
{
	using enum GLTF::EncodedImage::MimeType;
	switch (type)
	{
		case PNG: return "png";
		case JPEG: return "jpeg";
		case BC7: return "BC7";
		case BC6: return "BC6";
		default: assert_enum_out_of_range();
	}
}

GLTF::EncodedImage::MimeType ParseMimeType(std::string_view str)
{
	using enum GLTF::EncodedImage::MimeType;
	if (str == "image/png") return PNG;
	if (str == "image/jpeg") return JPEG;
	assert_failure("MimeType is not supported");
}

GLTF::EncodedImage::MimeType MimeTypeFromExtension(std::string_view str)
{
	using enum GLTF::EncodedImage::MimeType;
	if (str == ".png") return PNG;
	if (str == ".jpg") return JPEG;
	if (str == ".jpeg") return JPEG;
	assert_failure("Image type is not supported");
}
}

void GLTF::Serve(Book const & book, Desc const & desc)
{
	using namespace GLTF;
	using namespace rapidjson;
	using namespace File;
	using namespace File::JSON;

	Document document;
	document.Parse(LoadAsString(desc.path).c_str());
	auto & alloc = document.GetAllocator();

	auto const rel_file_path = std::filesystem::relative(desc.path, book.assets_dir);
	auto const asset_abs_file_path = book.assets_dir / rel_file_path;
	auto const asset_abs_dir_path = asset_abs_file_path.parent_path();
	auto const served_abs_file_path = book.served_dir / rel_file_path;
	auto const served_abs_dir_path = served_abs_file_path.parent_path();
	std::filesystem::create_directories(served_abs_dir_path);

	/// Parse
	vector<ByteBuffer> loaded_buffers;
	vector<BufferView> loaded_buffer_views;
	vector<Accessor> loaded_accessors;
	vector<RawMesh> loaded_meshes;
	vector<RawImage> loaded_images;
	{
		// Parse buffers
		for (auto const & item: document["buffers"].GetArray())
		{
			auto const & buffer = item.GetObject();

			auto file_size = buffer["byteLength"].GetUint64();
			auto file_name = buffer["uri"].GetString();
			// Limitation: only loads separate file binaries
			loaded_buffers.emplace_back(LoadAsBytes(asset_abs_dir_path / file_name, file_size));
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

			vector<RawPrimitive> primitives;
			primitives.reserve(mesh["primitives"].Size());
			for (auto const & item: mesh["primitives"].GetArray())
			{
				auto const & primitive = item.GetObject();

				vector<Attribute> attributes;
				attributes.reserve(primitive["attributes"].MemberCount());
				assert(attributes.size() < Geometry::ATTRIBUTE_COUNT, "Primitive has too many attributes");

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

		// Parse images
		if (auto iter = document.FindMember("images"); iter != document.MemberEnd())
		{
			for (auto const & item: iter->value.GetArray())
			{
				auto const & image = item.GetObject();

				auto uri = GetString(image, "uri", {});
				if (uri.size() > 5 and uri[5] == ':') // check for "data:" (base64 encoded data as a json string)
					assert_failure("images that embed data into uri are not supported yet");
				else if (not uri.empty())
					loaded_images.push_back({
						.uri = uri,
						.buffer_view_index = {},
						.mime_type = {},
					});
				else
					loaded_images.push_back({
						.uri = {},
						.buffer_view_index = image["bufferView"].GetUint(),
						.mime_type = image["mimeType"].GetString(),
					});
			}

			// tag sRGB images
			auto const & textures = document["textures"].GetArray();

			auto const mark_sRGB = [&textures, &loaded_images](ConstObj material, const char * name)
			{
				if (auto iter = material.FindMember(name); iter != material.MemberEnd())
				{
					auto const & tex_info = iter->value.GetObject();
					auto texture_idx = tex_info["index"].GetUint();
					auto image_idx = textures[texture_idx]["source"].GetUint();
					loaded_images[image_idx].is_sRGB = true;
				}
			};

			for (auto const & item : document["materials"].GetArray())
			{
				auto const & material = item.GetObject();
				mark_sRGB(material, "emissiveTexture");

				if (auto iter = material.FindMember("pbrMetallicRoughness"); iter != material.MemberEnd())
					mark_sRGB(iter->value.GetObject(), "baseColorTexture");
			}
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
					for (auto i = 0; auto & index: primitive.indices.as_span())
						index = i++;
				}
			}
		}
	}


	/// Convert Geometry
	auto const geometry_buffer_rel_path = "geometry.bin";
	ByteBuffer geometry_buffer;
	{
		document["buffers"] = Value(kArrayType);

		/// Buffer {buffers[0]}
		{
			auto total_size = 0;
			for (const auto & primitives: mesh_to_prim)
				for (const auto & prim: primitives)
					total_size += prim.vertices.buffer.size + prim.indices.buffer.size;
			geometry_buffer = ByteBuffer(total_size);

			Value buffer(kObjectType);
			buffer.AddMember("byteLength", geometry_buffer.size, alloc);
			buffer.AddMember("uri", Value(geometry_buffer_rel_path, alloc), alloc);

			document["buffers"].PushBack(buffer, alloc);
		}

		/// JSON
		// Each primitive will have:
		// - vertex_{offset, size}, index_{offset, size} that point to geometry buffer
		// - material index
		auto * converted_buffer_ptr = geometry_buffer.data_as<byte>();

		for (auto mesh_idx = 0; auto & item: document["meshes"].GetArray())
		{
			auto const & mesh = item.GetObject();
			auto const & prims = mesh["primitives"].GetArray();
			auto const & loaded_prims = mesh_to_prim[mesh_idx++];

			for (auto i = 0; i < loaded_prims.size(); ++i)
			{
				auto const & prim_js = prims[i].GetObject();
				auto const & prim = loaded_prims[i];

				auto const material_idx = GetOptionalU32(prim_js, "material");

				prim_js.EraseMember(prim_js.MemberBegin(), prim_js.MemberEnd());

				if (material_idx.has_value()) prim_js.AddMember("material", material_idx.value(), alloc);

				prim_js.AddMember("vertex_offset", converted_buffer_ptr - geometry_buffer.data.get(), alloc);
				prim_js.AddMember("vertex_count", prim.vertices.count, alloc);
				std::memcpy(converted_buffer_ptr, prim.vertices.buffer.data.get(), prim.vertices.buffer.size);
				converted_buffer_ptr += prim.vertices.buffer.size;

				prim_js.AddMember("index_offset", converted_buffer_ptr - geometry_buffer.data.get(), alloc);
				prim_js.AddMember("index_count", prim.indices.count, alloc);
				std::memcpy(converted_buffer_ptr, prim.indices.buffer.data.get(), prim.indices.buffer.size);
				converted_buffer_ptr += prim.indices.buffer.size;
			}
		}
	}


	/// Process Images
	if (not loaded_images.empty())
	{
		auto const image_buffer_rel_path = "image.bin";

		vector<Image> images;
		images.reserve(loaded_images.size());

		ImgComp::Initialize();

		Timer<std::chrono::milliseconds> timer;
		fmt::print("{}", "Processing images:\n");


		/// Load into RAM and encode each image
		for (const auto & loaded_image: loaded_images)
		{
			File::Image decoded_image;
			if (loaded_image.uri.empty())
			{
				assert_failure("Textures with BufferView are not supported yet");
			}
			else
			{
				auto encoded_buffer = LoadAsBytes(asset_abs_dir_path / loaded_image.uri);
				auto outcome = File::DecodeImage(encoded_buffer, false);
				assert(outcome, "Failed to decode image");

				decoded_image = outcome.into_result();
			}
			assert(not decoded_image.is_format_f32, "HDR in GLTF is not supported yet");

			// BC7 takes 4 channeled, so expand
			if (decoded_image.channels != 4)
			{
				auto * src_pixels = decoded_image.buffer.data_as<u8>();
				auto src_pixel_size = decoded_image.channels;

				ByteBuffer expanded(compMul(decoded_image.dimensions) * sizeof(u8[4]));
				auto * dst_pixels = expanded.data_as<u8[4]>();

				for (auto & dst_pixel : expanded.span_as<u8[4]>())
					memcpy(dst_pixel, src_pixels, src_pixel_size),
					src_pixels += src_pixel_size;

				decoded_image.buffer = move(expanded);
				decoded_image.channels = 4;
			}

			decoded_image.buffer = ImgComp::BC7(decoded_image);
			timer.timeit(stdout, loaded_image.uri);

			images.push_back({
				.data = move(decoded_image.buffer),
				.dimensions = decoded_image.dimensions,
				.is_sRGB = loaded_image.is_sRGB,
				.compression = Image::BC7,
			});
		}


		/// Merge images into image_buffer {buffers[1]} and populate GLTF:Images
		ByteBuffer image_buffer;
		{
			auto total_size = 0;
			for (const auto & image: images)
				total_size += image.data.size;
			image_buffer = ByteBuffer(total_size);
		}

		Value images_js(kArrayType);
		for (usize offset = 0; auto const & image: images)
		{
			Value image_js(kObjectType);
			image_js.AddMember("width", image.dimensions.x, alloc);
			image_js.AddMember("height", image.dimensions.y, alloc);
			image_js.AddMember("offset", offset, alloc);
			image_js.AddMember("size", image.data.size, alloc);
			images_js.PushBack(image_js, alloc);

			memcpy(image_buffer.data_as<byte>(offset), image.data.data.get(), image.data.size);

			offset += image.data.size;
		}
		document["images"] = images_js;

		images.clear();


		/// Compress and populate GLTF:Buffers
		auto compressed_image_buffer = Compress(image_buffer);
		fmt::print(
			"Image buffer {} KiB compressed into {} KiB ({}%)",
			image_buffer.size >> 10, compressed_image_buffer.size >> 10,
			f32(100) * compressed_image_buffer.size / image_buffer.size
		);

		Value buffer_js(kObjectType);
		buffer_js.AddMember("byteLengthUncompressed", image_buffer.size, alloc);
		buffer_js.AddMember("byteLength", compressed_image_buffer.size, alloc);
		buffer_js.AddMember("uri", Value(image_buffer_rel_path, alloc), alloc);
		document["buffers"].PushBack(buffer_js, alloc);


		/// Write to disk
		WriteBytes(served_abs_dir_path / image_buffer_rel_path, compressed_image_buffer);
	}


	/// Edit JSON
	document.RemoveMember("bufferViews");


	/// Write to file
	{
		// JSON
		// see: https://rapidjson.org/md_doc_stream.html#FileWriteStream
		StringBuffer buffer;
		PrettyWriter writer(buffer);
		document.Accept(writer);
		WriteString(served_abs_file_path, {buffer.GetString(), buffer.GetSize()});

		// Binary
		WriteBytes(served_abs_dir_path / geometry_buffer_rel_path, geometry_buffer);
	}
}
