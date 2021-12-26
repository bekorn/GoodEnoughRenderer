#pragma once

#include "Lib/core/index_ptr.hpp"
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

		// Pattern: Resize the vec and get resized portion
		template<typename T>
		span<T> Grow(vector<T> & vec, usize size)
		{
			vec.resize(vec.size() + size);
			return span(vec.end() - size, vec.end());
		}
	}

	void Convert(
		GLTFData const & gltf_data,
		vector<GL::Texture2D> & textures,
		vector<unique_ptr<Render::IMaterial>> & materials,
		vector<Geometry::Primitive> & primitives, vector<Render::Mesh> & meshes
	)
	{
		using namespace Helpers;

		// Load Textures
		auto new_textures = Grow(textures, gltf_data.textures.size());
		for (auto i = 0; i < gltf_data.textures.size(); ++i)
		{
			auto const & texture = gltf_data.textures[i];

			// TODO(bekorn) have a default image
			auto const & image = texture.image_index.has_value()
								 ? gltf_data.images[texture.image_index.value()]
								 : throw std::runtime_error("not implemented");

			auto const & sampler = texture.sampler_index.has_value()
								   ? gltf_data.samplers[texture.sampler_index.value()]
								   : GLTF::SamplerDefault;

			new_textures[i].create(
				GL::Texture2D::ImageDescription{
					.dimensions = image.dimensions,
					.has_alpha = image.channels == 4,

					.min_filter = GL::GLenum(sampler.min_filter),
					.mag_filter = GL::GLenum(sampler.mag_filter),
					.wrap_s = GL::GLenum(sampler.wrap_s),
					.wrap_t = GL::GLenum(sampler.wrap_t),

					.data = image.data.span_as<byte>(),
				}
			);
		}

		// Load materials
		auto new_materials = Grow(materials, gltf_data.materials.size());
		for (auto i = 0; i < gltf_data.materials.size(); ++i)
		{
			auto const & gltf_mat = gltf_data.materials[i];
			if (gltf_mat.pbr_metallic_roughness)
			{
				auto const & pbr_mat = gltf_mat.pbr_metallic_roughness.value();
				auto mat = make_unique<Render::Material_gltf_pbrMetallicRoughness>();

				// TODO: use texcoord indices as well
				if (pbr_mat.base_color_texture)
					mat->base_color_texture = new_textures[pbr_mat.base_color_texture->texture_index].id;
				else
					mat->base_color_factor = pbr_mat.base_color_factor;

				if (pbr_mat.metallic_roughness_texture)
					mat->metallic_roughness_texture = new_textures[pbr_mat.metallic_roughness_texture->texture_index].id;
				else
					mat->metallic_roughness_factor = {pbr_mat.metallic_factor, pbr_mat.roughness_factor};

				if (gltf_mat.emissive_texture)
					mat->emissive_texture = new_textures[gltf_mat.emissive_texture->texture_index].id;
				else
					mat->emissive_factor = gltf_mat.emissive_factor;

				if (gltf_mat.occlusion_texture)
					mat->occlusion_texture = new_textures[gltf_mat.occlusion_texture->texture_index].id;

				if (gltf_mat.normal_texture)
					mat->normal_texture = new_textures[gltf_mat.normal_texture->texture_index].id;

				new_materials[i] = move(mat);
			}
		}

		// Load meshes
		auto new_meshes = Grow(meshes, gltf_data.meshes.size());
		for (auto i = 0; i < gltf_data.meshes.size(); ++i)
		{
			auto const & gltf_mesh = gltf_data.meshes[i];
			auto & mesh = new_meshes[i];

			mesh.name = gltf_mesh.name;

			// Load Primitives
			auto new_primitives = Grow(primitives, gltf_mesh.primitives.size());
			for (auto primitive_index = 0; primitive_index < gltf_mesh.primitives.size(); ++primitive_index)
			{
				auto const & gltf_primitive = gltf_mesh.primitives[primitive_index];
				auto const attribute_size = gltf_primitive.attributes.size();

				auto & primitive = new_primitives[primitive_index];
				primitive.attributes.reserve(attribute_size);
				for (auto i = 0; i < attribute_size; ++i)
				{
					auto const & attribute = gltf_primitive.attributes[i];
					auto const & accessor = gltf_data.accessors[attribute.accessor_index];
					auto const & buffer_view = gltf_data.buffer_views[accessor.buffer_view_index];

					Geometry::Attribute::Data data{
						.type = IntoAttributeType(accessor.vector_data_type, accessor.normalized),
						.dimension = static_cast<u8>(accessor.vector_dimension),
					};

					u32 data_buffer_stride = data.type.size() * data.dimension;
					data.buffer = ByteBuffer(data_buffer_stride * accessor.count);

					if (buffer_view.stride.has_value())
					{
						// strided access
						auto const source_stride = buffer_view.stride.value();
						auto source = gltf_data.buffers[buffer_view.buffer_index]
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
						auto source = gltf_data.buffers[buffer_view.buffer_index]
							.span_as<byte>(buffer_view.offset + accessor.byte_offset, data.buffer.size);

						std::memcpy(data.buffer.begin(), source.data(), data.buffer.size);
					}

					primitive.attributes.emplace(std::piecewise_construct,
						std::make_tuple(IntoAttributeKey(attribute.name)),
						std::make_tuple(move(data))
					);
				}

				if (gltf_primitive.indices_accessor_index.has_value())
				{
					auto const & accessor = gltf_data.accessors[gltf_primitive.indices_accessor_index.value()];
					auto const & buffer_view = gltf_data.buffer_views[accessor.buffer_view_index];

					auto index_type = IntoAttributeType(accessor.vector_data_type, accessor.normalized);

					if (index_type == Geometry::Attribute::Type::U8)
					{
						auto const source = gltf_data.buffers[buffer_view.buffer_index]
							.span_as<u8>(accessor.byte_offset + buffer_view.offset, accessor.count * index_type.size());

						primitive.indices.resize(source.size());
						for (auto index: source)
							primitive.indices.emplace_back(index);
					}
					else if (index_type == Geometry::Attribute::Type::U16)
					{
						auto const source = gltf_data.buffers[buffer_view.buffer_index]
							.span_as<u16>(accessor.byte_offset + buffer_view.offset, accessor.count * index_type.size());

						primitive.indices.resize(source.size());
						for (auto index: source)
							primitive.indices.emplace_back(index);
					}
					else if (index_type == Geometry::Attribute::Type::U32)
					{
						auto const source = gltf_data.buffers[buffer_view.buffer_index]
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
					auto const & accessor = gltf_data.accessors[gltf_primitive.attributes[0].accessor_index];
					auto vertex_count = accessor.count;

					primitive.indices.resize(vertex_count);
					for (u32 i = 0; i < vertex_count; ++i)
						primitive.indices[i] = i;
				}

				//	TODO(bekorn): have a default material
				auto const material_index = gltf_primitive.material_index.has_value()
											? gltf_primitive.material_index.value()
											: throw std::runtime_error("not implemented");

				mesh.primitives.push_back(
					{
						.primitive_ptr = index_ptr(primitives, &primitive),
						.material_ptr = index_ptr(materials, &new_materials[material_index]),
					}
				);
			}
		}
	}
}