#include "gltf.hpp"

#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>

namespace GLTF
{
optional<std::string> Serve(LoadedData const & loaded_data, std::filesystem::path path)
{
	using namespace rapidjson;
	using namespace File;
	using namespace File::JSON;

	Document document;
	auto & alloc = document.GetAllocator();
	document.SetObject();


	/// Serialize GLTF

	// Buffers
	{
		// write buffers to disk as a binary
		usize total_size = 0;
		for (auto & buffer: loaded_data.buffers)
			total_size += buffer.size;

		::ByteBuffer all_buffers(total_size);
		usize current_size = 0;
		for (auto & buffer: loaded_data.buffers)
		{
			memcpy(all_buffers.data_as<byte>(current_size), buffer.data_as<byte>(), buffer.size);
			current_size += buffer.size;
		}

		auto buffer_path = std::filesystem::path(path).replace_extension("bin");
		File::WriteBytes(buffer_path, all_buffers);

		// add to json
		Value buffer(kObjectType);
		buffer.AddMember("byteLength", all_buffers.size, alloc);
		// TODO(bekorn): https://rapidjson.org/group___r_a_p_i_d_j_s_o_n___c_o_n_f_i_g.html#ga2f2eef0ee4477f3fe5874703a66e997f
		buffer.AddMember("uri", Value(buffer_path.filename().generic_string().c_str(), alloc), alloc);

		Value all(kArrayType);
		all.PushBack(buffer, alloc);

		document.AddMember("buffers", all, alloc);
	}

	// Buffer views
	{
		Value all(kArrayType);
		for (auto & bv: loaded_data.buffer_views)
		{
			Value buffer_view(kObjectType);
			buffer_view.AddMember("buffer", bv.buffer_index, alloc);
			buffer_view.AddMember("byteOffset", bv.offset, alloc);
			buffer_view.AddMember("byteLength", bv.length, alloc);
			if (bv.stride) buffer_view.AddMember("stride", bv.stride.value(), alloc);
			all.PushBack(buffer_view, alloc);
		}
		document.AddMember("buffer_views", all, alloc);
	}

	// Images
	{
		// TODO(bekorn): serve images
	}

	// Samplers
	{
		// TODO(bekorn): serve samplers
	}

	// Textures
	{
		// TODO(bekorn): serve textures
	}

	// Accessors
	{
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

		Value all(kArrayType);
		for (const auto & a: loaded_data.accessors)
		{
			Value accessor(kObjectType);
			accessor.AddMember("bufferView", a.buffer_view_index, alloc);
			accessor.AddMember("byteOffset", a.byte_offset, alloc);
			accessor.AddMember("componentType", a.vector_data_type, alloc);
			accessor.AddMember("type", Value(get_type(a.vector_dimension), alloc), alloc);
			accessor.AddMember("count", a.count, alloc);
			accessor.AddMember("normalized", a.normalized, alloc);
			all.PushBack(accessor, alloc);
		}
		document.AddMember("accessors", all, alloc);
	}

	// Meshes
	{
		Value all(kArrayType);
		for (const auto & m: loaded_data.meshes)
		{
			Value all_p(kArrayType);
			for (const auto & p: m.primitives)
			{
				Value all_a(kObjectType);
				for (const auto & a: p.attributes)
				{
					auto key = Value(a.name.c_str(), alloc);
					all_a.AddMember(key, a.accessor_index, alloc);
				}

				Value primitive(kObjectType);
				primitive.AddMember("name", Value(p.name.c_str(), alloc), alloc);
				primitive.AddMember("attributes", all_a, alloc);
				if (p.indices_accessor_index) primitive.AddMember("indices", p.indices_accessor_index.value(), alloc);
				if (p.material_index) primitive.AddMember("material", p.material_index.value(), alloc);
				all_p.PushBack(primitive, alloc);
			}

			Value mesh(kObjectType);
			mesh.AddMember("name", Value(m.name.c_str(), alloc), alloc);
			mesh.AddMember("primitives", all_p, alloc);
			all.PushBack(mesh, alloc);
		}
		document.AddMember("meshes", all, alloc);
	}


	/// Write to file
	// see: https://rapidjson.org/md_doc_stream.html#FileWriteStream
	StringBuffer buffer;
	PrettyWriter writer(buffer);
	document.Accept(writer);

	std::filesystem::create_directories(path.parent_path());
	File::WriteString(path, {buffer.GetString(), buffer.GetSize()});

	fmt::print("Served gltf:\n{}\n", buffer.GetString()); // !!! Temporary

	return {};
}
}