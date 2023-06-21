#pragma message("-- read ASSET/GLTF/.Cpp --")

#include "load.hpp"
#include "convert.hpp"

#include <execution>

namespace GLTF
{
LoadedData Load(Desc const & desc)
{
	using namespace rapidjson;
	using namespace File;
	using namespace File::JSON;

	LoadedData loaded;

	Document document;
	document.Parse(LoadAsString(desc.path).c_str());

	auto const file_dir = desc.path.parent_path();

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

	// Parse images
	if (auto const member = document.FindMember("images"); member != document.MemberEnd())
	{
		auto const & items = member->value.GetArray();
		loaded.images.resize(items.Size());
		std::transform(
			std::execution::par_unseq,
			items.Begin(), items.End(),
			loaded.images.data(),
			[&file_dir](Document::Array::ValueType const & item) -> GLTF::Image
			{
				auto const & image = item.GetObject();

				auto const member = image.FindMember("uri");
				if (member == image.MemberEnd())
					throw std::runtime_error("images without a uri file path are not supported yet");

				auto uri = member->value.GetString();
				if (uri[5] == ':') // check for "data:" (base64 encoded data as a json string)
					throw std::runtime_error("images without a uri file path are not supported yet");

				// gltf textures (first-pixel == uv(0,0)) do not require a vertical flip
				auto image_file = File::LoadImage(file_dir / uri, false);

				return {
					.data = move(image_file.buffer),
					.dimensions = image_file.dimensions,
					.channels = image_file.channels,
					.is_sRGB = false,
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
			loaded.samplers.push_back(
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
	NameGenerator texture_name_generator{.prefix = desc.name + ":texture:"};
	if (auto const member = document.FindMember("textures"); member != document.MemberEnd())
	{
		for (auto const & item: member->value.GetArray())
		{
			auto const & texture = item.GetObject();

			loaded.textures.push_back(
				{
					.name = texture_name_generator.get(texture, "name"),
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

	// Pass layout name
	loaded.layout_name = desc.layout_name;

	// Parse materials
	NameGenerator material_name_generator{.prefix = desc.name + ":material:"};
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

		auto const mark_sRGB = [&loaded](optional<Material::TexInfo> const & tex_info)
		{
			if (tex_info)
				if (auto & image_index = loaded.textures[tex_info.value().texture_index].image_index)
					loaded.images[image_index.value()].is_sRGB = true;
		};

		auto const & material = item.GetObject();
		Material mat{
			.name = material_name_generator.get(material, "name"),

			.normal_texture = get_tex_info(material, "normalTexture"),

			.occlusion_texture = get_tex_info(material, "occlusionTexture"),

			.emissive_texture = get_tex_info(material, "emissiveTexture"),
			.emissive_factor = GetF32x3(material, "emissiveFactor", {0, 0, 0}),

			.alpha_mode = GetString(material, "alphaMode", "OPAQUE"),
			.alpha_cutoff = GetF32(material, "alphaCutoff", 0.5),

			.double_sided = GetBool(material, "doubleSided", false),
		};
		// mark sRGB textures
		mark_sRGB(mat.emissive_texture);

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
			// mark sRGB textures
			mark_sRGB(mat.pbr_metallic_roughness.value().base_color_texture);
		}

		loaded.materials.push_back(mat);
	}

	// Parse nodes
	NameGenerator node_name_generator{.prefix = desc.name + ":node:"};
	if (auto const member = document.FindMember("nodes"); member != document.MemberEnd())
	{
		for (auto const & item: member->value.GetArray())
		{
			auto const & gltf_node = item.GetObject();

			Node node{
				.name = node_name_generator.get(gltf_node, "name"),
				.mesh_index = GetOptionalU32(gltf_node, "mesh"),
			};

			if (auto member = gltf_node.FindMember("matrix"); member != gltf_node.MemberEnd())
			{
				f32x4x4 matrix;
				auto const & arr = member->value.GetArray();
				for (auto i = 0; i < 4; ++i)
					for (auto j = 0; j < 4; ++j)
						matrix[i][j] = arr[i * 4 + j].GetFloat();

				f32x3 skew;
				f32x4 perspective;
				glm::decompose(matrix, node.scale, node.rotation, node.translation, skew, perspective);
			}
			else
			{
				node.translation = GetF32x3(gltf_node, "translation", {0, 0, 0});
				auto rotation = GetF32x4(gltf_node, "rotation", {0, 0, 0, 1});			 // gltf order (x, y, z, w)
				node.rotation = f32quat(rotation.w, rotation.x, rotation.y, rotation.z); // glm  order (w, x, y, z)
				node.scale = GetF32x3(gltf_node, "scale", f32x3{1, 1, 1});
			}

			if (auto member = gltf_node.FindMember("children"); member != gltf_node.MemberEnd())
				for (auto & child_index: member->value.GetArray())
					node.child_indices.push_back(child_index.GetUint());

			loaded.nodes.push_back(node);
		}
	}

	// Parse scene
	if (auto const member = document.FindMember("scenes"); member != document.MemberEnd())
	{
		u32 scene_index = 0;
		if (auto member = document.FindMember("scene"); member != document.MemberEnd())
			scene_index = member->value.GetUint();

		auto const & gltf_scene = member->value.GetArray()[scene_index].GetObject();

		if (auto member = gltf_scene.FindMember("nodes"); member != gltf_scene.MemberEnd())
			for (auto & node_index: member->value.GetArray())
				loaded.scene.node_indices.push_back(node_index.GetUint());
	}
	else
	{
		for (auto i = 0; i < loaded.nodes.size(); i++)
			loaded.scene.node_indices.push_back(i);
	}
	loaded.scene.name = desc.name + ":scene";

	return loaded;
}

namespace Helpers
{
	// Pattern: String into Geometry::Attribute::Key
	Geometry::Key IntoAttributeKey(std::string_view name)
	{
		using namespace Geometry;

		static auto const IntoCommon = [](std::string_view attribute_name) -> optional<Key::Common>
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
}

void Convert(
	LoadedData const & loaded,
	Managed<GL::Texture2D> & textures,
	Managed<unique_one<Render::IMaterial>> & materials,
	Managed<Geometry::Primitive> & primitives,
	Managed<Render::Mesh> & meshes,
	::Scene::Tree & scene_tree,
	Managed<Geometry::Layout> const & vertex_layouts
)
{
	using namespace Helpers;

	// Convert Textures
	for (auto & loaded_texture: loaded.textures)
	{
		// TODO(bekorn) have a default image
		auto & loaded_image = loaded_texture.image_index.has_value()
							  ? loaded.images[loaded_texture.image_index.value()]
							  : throw std::runtime_error("not implemented");

		auto & loaded_sampler = loaded_texture.sampler_index.has_value()
								? loaded.samplers[loaded_texture.sampler_index.value()]
								: GLTF::SamplerDefault;

		auto should_have_mipmaps = not (
			GL::GLenum(loaded_sampler.min_filter) == GL::GL_LINEAR or
			GL::GLenum(loaded_sampler.min_filter) == GL::GL_NEAREST
		);

		textures.generate(loaded_texture.name).data.init(
			GL::Texture2D::ImageDesc{
				.dimensions = loaded_image.dimensions,
				.has_alpha = loaded_image.channels == 4,
				.color_space = loaded_image.is_sRGB ? GL::COLOR_SPACE::SRGB_U8 : GL::COLOR_SPACE::LINEAR_U8,

				.levels = should_have_mipmaps ? 0 : 1,

				.min_filter = GL::GLenum(loaded_sampler.min_filter),
				.mag_filter = GL::GLenum(loaded_sampler.mag_filter),
				.wrap_s = GL::GLenum(loaded_sampler.wrap_s),
				.wrap_t = GL::GLenum(loaded_sampler.wrap_t),

				.data = loaded_image.data.span_as<byte>(),
			}
		);
	}

	// Convert materials
	for (auto & loaded_mat: loaded.materials)
	{
		if (loaded_mat.pbr_metallic_roughness.has_value())
		{
			auto & pbr_mat = loaded_mat.pbr_metallic_roughness.value();
			auto mat = make_unique_one<Render::Material_gltf_pbrMetallicRoughness>();

			// TODO: use texcoord indices as well
			if (pbr_mat.base_color_texture)
				mat->base_color_texture_handle = textures.get(loaded.textures[pbr_mat.base_color_texture->texture_index].name).handle;
			else
				mat->base_color_factor = pbr_mat.base_color_factor;

			if (pbr_mat.metallic_roughness_texture)
				mat->metallic_roughness_texture_handle = textures.get(loaded.textures[pbr_mat.metallic_roughness_texture->texture_index].name).handle;
			else
				mat->metallic_roughness_factor = {pbr_mat.metallic_factor, pbr_mat.roughness_factor};

			if (loaded_mat.emissive_texture)
				mat->emissive_texture_handle = textures.get(loaded.textures[loaded_mat.emissive_texture->texture_index].name).handle;
			else
				mat->emissive_factor = loaded_mat.emissive_factor;

			if (loaded_mat.occlusion_texture)
				mat->occlusion_texture_handle = textures.get(loaded.textures[loaded_mat.occlusion_texture->texture_index].name).handle;

			if (loaded_mat.normal_texture)
				mat->normal_texture_handle = textures.get(loaded.textures[loaded_mat.normal_texture->texture_index].name).handle;

			materials.generate(loaded_mat.name).data = move(mat);
		}
	}

	// Convert primitives
	// TODO(bekorn): the layout is same for the whole gltf file, it should be more granular, per material perhaps
	assert(loaded.layout_name.has_value(), "Default layout not supported yet");
	auto & layout = vertex_layouts.get(loaded.layout_name.value());
	vector<Geometry::Key> loaded_attrib_keys;
	loaded_attrib_keys.reserve(Geometry::ATTRIBUTE_COUNT);
	for (auto & loaded_mesh: loaded.meshes)
		for (auto & loaded_primitive: loaded_mesh.primitives)
		{
			auto & primitive = primitives.generate(Name(loaded_primitive.name)).data;

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
				assert(vec == layout_attrib.vec, "Primitive attribute is in a different format");

				auto attrib_location = layout_attrib.location; // also i
				auto & buffer = primitive.data.buffers[attrib_location];
				u32 buffer_stride = layout_attrib.vec.size();
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

	// Convert meshes
	for (auto & loaded_mesh: loaded.meshes)
	{
		auto & mesh = meshes.generate(loaded_mesh.name).data;

		// Create Drawables
		mesh.drawables.reserve(loaded_mesh.primitives.size());
		for (auto & loaded_primitive : loaded_mesh.primitives)
		{
			//	TODO(bekorn): have a default material
			auto material_index = loaded_primitive.material_index.has_value()
								  ? loaded_primitive.material_index.value()
								  : throw std::runtime_error("not implemented");

			auto & material_name = loaded.materials[material_index].name;

			mesh.drawables.push_back(
				{
					.primitive = primitives.get(loaded_primitive.name),
					.named_material = materials.get_named(material_name),
				}
			);
		}
	}

	// Convert scene
	{
		struct NodeToAdd
		{
			u32 loaded_index; // in loaded
			u32 parent_index; // in scene_tree
			u32 depth;
		};
		std::queue<NodeToAdd> queue;

		for (auto & node_index: loaded.scene.node_indices)
			queue.push({.loaded_index = node_index, .parent_index = 0, .depth = 0});

		while (not queue.empty())
		{
			auto [loaded_index, parent_index, depth] = queue.front();
			queue.pop();

			auto & loaded_node = loaded.nodes[loaded_index];

			auto [_, index] = scene_tree.add({
				.name = loaded_node.name,
				.depth = depth,
				.parent_index = parent_index,
				.transform = {
					.position = loaded_node.translation,
					.rotation = loaded_node.rotation,
					.scale = loaded_node.scale,
				},
				.mesh = loaded_node.mesh_index.has_value()
						? &meshes.get(loaded.meshes[loaded_node.mesh_index.value()].name)
						: nullptr,
			});

			for (auto & child_index: loaded_node.child_indices)
				queue.push({.loaded_index = child_index, .parent_index = index, .depth = depth + 1});
		}

		scene_tree.update_transforms();
	}
}

std::pair<Name, Desc> Parse(File::JSON::JSONObj o, std::filesystem::path const & root_dir)
{
	auto name = o.FindMember("name")->value.GetString();
	return {
		name,
		{
			.name = name,
			.path = root_dir / o["path"].GetString(),
			.layout_name = File::JSON::GetOptionalString(o, "layout"),
		}
	};
}
}