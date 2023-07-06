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
	{
		auto const & buffers = document["buffers"].GetArray();

		auto const & geometry_buffer = buffers[0].GetObject();
		loaded.geometry_buffer = LoadAsBytes(
			file_dir / geometry_buffer["uri"].GetString(),
			geometry_buffer["byteLength"].GetUint64()
		);

		if (buffers.Size() > 1)
		{
			auto const & image_buffer = buffers[1].GetObject();
			loaded.image_buffer = LoadAsBytes(
				file_dir / image_buffer["uri"].GetString(),
				image_buffer["byteLength"].GetUint64()
			);
		}
	}

	// Parse images
	if (auto const member = document.FindMember("images"); member != document.MemberEnd())
	{
		auto const & images = member->value.GetArray();
		loaded.images.resize(images.Size());
		auto const & image_buffer = loaded.image_buffer;

		std::transform(
			std::execution::par_unseq,
			images.Begin(), images.End(),
			loaded.images.data(),
			[&image_buffer](Value const & item) -> GLTF::Image
			{
				auto const & image = item.GetObject();

				auto const mime_type = image["mimeType"].GetString();
				auto const offset = image["offset"].GetUint();
				auto const size = image["size"].GetUint();
				auto const buffer = image_buffer.span_as<byte>(offset, size);

				// gltf textures (first-pixel == uv(0,0)) do not require a vertical flip
				auto outcome = File::DecodeImage(buffer, false);
				assert(outcome, "Failed to decode image in gltf");

				auto decoded_image = outcome.into_result();
				return {
					.data = move(decoded_image.buffer),
					.dimensions = decoded_image.dimensions,
					.channels = decoded_image.channels,
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

			auto const & extras = primitive["extras"].GetObject();
			auto const & buffers = extras["buffers"].GetObject();
			auto const & vertex_buffer = buffers["vertex"].GetObject();
			auto const & index_buffer = buffers["index"].GetObject();

			primitives.push_back({
				.name = primitive_name_generator.get(primitive, "name"),
				.material_index = GetOptionalU32(primitive, "material"),
				.vertex_buffer_offset = vertex_buffer["offset"].GetUint(),
				.vertex_count = vertex_buffer["count"].GetUint(),
				.index_buffer_offset = index_buffer["offset"].GetUint(),
				.index_count = index_buffer["count"].GetUint(),
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

void Convert(
	LoadedData const & loaded,
	Managed<GL::Texture2D> & textures,
	Managed<unique_one<Render::IMaterial>> & materials,
	Managed<Render::Mesh> & meshes,
	::Scene::Tree & scene_tree,
	Managed<Geometry::Layout> const & attrib_layouts
)
{
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

	// Convert meshes
	// TODO(bekorn): the layout is same for the whole gltf file, it should be more granular, per material perhaps
	assert(loaded.layout_name.has_value(), "Default layout not supported yet");
	auto & layout = attrib_layouts.get(loaded.layout_name.value());
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
			auto named_material = materials.get_named(material_name);

			Geometry::Primitive primitive;
			primitive.layout = &layout;

			auto * buffer_ptr = loaded.geometry_buffer.data.get();

			primitive.vertices.init(*primitive.layout, loaded_primitive.vertex_count);
			memcpy(primitive.vertices.buffer.begin(), buffer_ptr + loaded_primitive.vertex_buffer_offset, primitive.vertices.buffer.size);

			primitive.indices.init(loaded_primitive.index_count);
			memcpy(primitive.indices.buffer.begin(), buffer_ptr + loaded_primitive.index_buffer_offset, primitive.indices.buffer.size);

			mesh.drawables.emplace_back(named_material, primitive);
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
	auto const & name = o["name"].GetString();
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