#pragma once

#include "Lib/core/named.hpp"
#include "Lib/render/.hpp"

#include "core.hpp"

namespace GLTF
{
	namespace Helpers
	{
		// Pattern: String into Geometry::Attribute::Key
		Geometry::Attribute::Key IntoAttributeKey(std::string_view name)
		{
			using namespace Geometry::Attribute;

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

		Geometry::Attribute::Type IntoAttributeType(u32 type, bool is_normalized)
		{
			// see spec section 3.6.2.2. Accessor Data Types
			using enum Geometry::Attribute::Type::Value;

			if (is_normalized)
				switch (type)
				{
				case 5120: return I8NORM;
				case 5121: return U8NORM;
				case 5122: return I16NORM;
				case 5123: return U16NORM;
				case 5125: return U32NORM;
				}
			else
				switch (type)
				{
				case 5120: return I8;
				case 5121: return U8;
				case 5122: return I16;
				case 5123: return U16;
				case 5125: return U32;
				case 5126: return F32;
				}

			throw std::runtime_error("Unknown type or combination :(");
		}
	}

	void Convert(
		LoadedData const & loaded,
		Managed<GL::Texture2D> & textures,
		Managed<unique_ptr<Render::IMaterial>> & materials,
		Managed<Geometry::Primitive> & primitives,
		Managed<Render::Mesh> & meshes,
		Managed<Scene::Node> & nodes
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

			textures.generate(loaded_texture.name).data.create(
				GL::Texture2D::ImageDescription{
					.dimensions = loaded_image.dimensions,
					.has_alpha = loaded_image.channels == 4,

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
				auto mat = make_unique<Render::Material_gltf_pbrMetallicRoughness>();

				// TODO: use texcoord indices as well
				if (pbr_mat.base_color_texture)
					mat->base_color_texture = textures.get(loaded.textures[pbr_mat.base_color_texture->texture_index].name).id;
				else
					mat->base_color_factor = pbr_mat.base_color_factor;

				if (pbr_mat.metallic_roughness_texture)
					mat->metallic_roughness_texture = textures.get(loaded.textures[pbr_mat.metallic_roughness_texture->texture_index].name).id;
				else
					mat->metallic_roughness_factor = {pbr_mat.metallic_factor, pbr_mat.roughness_factor};

				if (loaded_mat.emissive_texture)
					mat->emissive_texture = textures.get(loaded.textures[loaded_mat.emissive_texture->texture_index].name).id;
				else
					mat->emissive_factor = loaded_mat.emissive_factor;

				if (loaded_mat.occlusion_texture)
					mat->occlusion_texture = textures.get(loaded.textures[loaded_mat.occlusion_texture->texture_index].name).id;

				if (loaded_mat.normal_texture)
					mat->normal_texture = textures.get(loaded.textures[loaded_mat.normal_texture->texture_index].name).id;

				materials.generate(loaded_mat.name).data = move(mat);
			}
		}

		// Convert primitives
		for (auto & loaded_mesh: loaded.meshes)
			for (auto & loaded_primitive: loaded_mesh.primitives)
			{
				auto & primitive = primitives.generate(Name(loaded_primitive.name)).data;

				primitive.attributes.reserve(loaded_primitive.attributes.size());
				for (auto & attribute: loaded_primitive.attributes)
				{
					auto & accessor = loaded.accessors[attribute.accessor_index];
					auto & buffer_view = loaded.buffer_views[accessor.buffer_view_index];

					auto data = Geometry::Attribute::Data{
						.type = IntoAttributeType(accessor.vector_data_type, accessor.normalized),
						.dimension = static_cast<u8>(accessor.vector_dimension),
					};

					u32 data_buffer_stride = data.type.size() * data.dimension;
					data.buffer = ByteBuffer(data_buffer_stride * accessor.count);

					if (buffer_view.stride.has_value())
					{
						// strided access
						auto source_stride = buffer_view.stride.value();
						auto source = loaded.buffers[buffer_view.buffer_index]
							.span_as<byte>(buffer_view.offset + accessor.byte_offset, source_stride * accessor.count);

						auto source_ptr = source.data();
						auto data_ptr = data.buffer.begin();
						auto data_end = data.buffer.end();
						while (data_ptr != data_end)
						{
							std::memcpy(data_ptr, source_ptr, data_buffer_stride);
							source_ptr += source_stride;
							data_ptr += data_buffer_stride;
						}
					}
					else
					{
						// contiguous access (tightly packed data)
						auto source = loaded.buffers[buffer_view.buffer_index]
							.span_as<byte>(buffer_view.offset + accessor.byte_offset, data.buffer.size);

						std::memcpy(data.buffer.begin(), source.data(), data.buffer.size);
					}

					primitive.attributes.emplace(IntoAttributeKey(attribute.name), move(data));
				}

				if (loaded_primitive.indices_accessor_index.has_value())
				{
					auto & accessor = loaded.accessors[loaded_primitive.indices_accessor_index.value()];
					auto & buffer_view = loaded.buffer_views[accessor.buffer_view_index];

					auto index_type = IntoAttributeType(accessor.vector_data_type, accessor.normalized);

					if (index_type == Geometry::Attribute::Type::U8)
					{
						auto source = loaded.buffers[buffer_view.buffer_index]
							.span_as<u8>(accessor.byte_offset + buffer_view.offset, accessor.count * index_type.size());

						primitive.indices.resize(source.size());
						for (auto index: source)
							primitive.indices.emplace_back(index);
					}
					else if (index_type == Geometry::Attribute::Type::U16)
					{
						auto source = loaded.buffers[buffer_view.buffer_index]
							.span_as<u16>(accessor.byte_offset + buffer_view.offset, accessor.count * index_type.size());

						primitive.indices.resize(source.size());
						for (auto index: source)
							primitive.indices.emplace_back(index);
					}
					else if (index_type == Geometry::Attribute::Type::U32)
					{
						auto source = loaded.buffers[buffer_view.buffer_index]
							.span_as<u32>(accessor.byte_offset + buffer_view.offset, accessor.count * index_type.size());

						// buffers other than vertex attributes are always tightly packed
						// see spec section 3.6.2.1. Overview, paragraph 2
						primitive.indices.resize(source.size());
						std::memcpy(primitive.indices.data(), source.data(), source.size());
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

		// TODO(bekorn): Convert nodes
		//  currently only using the transform and mesh, use parent as well
		for (auto & loaded_node: loaded.nodes)
		{
			nodes.generate(loaded_node.name, Scene::Node{
				.parent = nullptr,
				.transform = {
					.position = loaded_node.translation,
					.rotation = loaded_node.rotation,
					.scale = loaded_node.scale,
				},
				.mesh = loaded_node.mesh_index.has_value()
						? &meshes.get(loaded.meshes[loaded_node.mesh_index.value()].name)
						: nullptr,
			});
		}
	}
}