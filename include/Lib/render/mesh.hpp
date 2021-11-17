#pragma once

#include "Lib/core/core_types.hpp"
#include "Lib/opengl/.hpp"
#include "Lib/file_management/gltf_loader.hpp"

#include "material.hpp"
#include "drawable.hpp"

u32 GetGLConventionAttributeLocation(std::string const & name)
{
	if (name == "POSITION") return GL::ATTRIBUTE_LOCATION::POSITION;
	if (name == "NORMAL") return GL::ATTRIBUTE_LOCATION::NORMAL;
	if (name == "TANGENT") return GL::ATTRIBUTE_LOCATION::TANGENT;

	if (name == "TEXCOORD_0") return GL::ATTRIBUTE_LOCATION::TEXCOORD_0;
	if (name == "TEXCOORD_1") return GL::ATTRIBUTE_LOCATION::TEXCOORD_1;
	if (name == "TEXCOORD_2") return GL::ATTRIBUTE_LOCATION::TEXCOORD_2;
	if (name == "TEXCOORD_3") return GL::ATTRIBUTE_LOCATION::TEXCOORD_3;

	if (name == "COLOR_0") return GL::ATTRIBUTE_LOCATION::COLOR_0;
	if (name == "COLOR_1") return GL::ATTRIBUTE_LOCATION::COLOR_1;
	if (name == "COLOR_2") return GL::ATTRIBUTE_LOCATION::COLOR_2;
	if (name == "COLOR_3") return GL::ATTRIBUTE_LOCATION::COLOR_3;

	throw std::runtime_error("Attribute " + name + " is not conventional, don't know what to do :/");
}

// Limitation: Only supports OpenGL
// TODO(bekorn): This struct should not own the GLObjects, just reference them
// TODO(bekorn): Mesh should only contain data about geometry
struct Mesh
{
	// Resources
	std::vector<GL::Buffer> buffers;
	std::vector<GL::Texture2D> textures;
	std::vector<unique_ptr<IMaterial>> materials;

	// Vertex attributes + material index
	std::vector<ArrayDrawable> array_drawables;
	std::vector<ElementDrawable> element_drawables;

	Mesh() noexcept = default;
	Mesh(Mesh const &) noexcept = delete;
	Mesh(Mesh &&) noexcept = default;

	Mesh(GLTF::GLTFData const & gltf_data, u32 mesh_index) noexcept
	{
		// TODO(bekorn): This method creates a vbo for each attribute,
		//  it would be better to merge them in a single vbo

		// Limitation: Only the first primitive
		auto const & primitive = gltf_data.meshes[mesh_index].primitives[0];
		{
			auto const attribute_size = primitive.attributes.size();

			buffers.resize(attribute_size + primitive.indices_accessor_index.has_value());
			for (auto i = 0; i < attribute_size; ++i)
			{
				auto const & attribute = primitive.attributes[i];
				auto const & accessor = gltf_data.accessors[attribute.accessor_index];
				auto const & buffer_view = gltf_data.buffer_views[accessor.buffer_view_index];
				auto const & buffer = gltf_data.buffers[buffer_view.buffer_index];

				buffers[i].create(
					{
						.type = GL::GL_ARRAY_BUFFER,
						.data = buffer.span_as<byte>(buffer_view.byte_offset, buffer_view.byte_length)
					}
				);
			}

			std::vector<GL::Attribute::Description> attributes;
			attributes.reserve(attribute_size);
			for (auto i = 0; i < attribute_size; ++i)
			{
				auto const & attribute = primitive.attributes[i];
				auto const & accessor = gltf_data.accessors[attribute.accessor_index];
				auto const & buffer_view = gltf_data.buffer_views[accessor.buffer_view_index];
				auto const & buffer = gltf_data.buffers[buffer_view.buffer_index];

				attributes.push_back(
					{
						.buffer = buffers[i], // !!! This is a reference: if buffers vector allocates, there will be chaos
						.byte_offset = 0, // because each attribute has its own VBO
						.location = GetGLConventionAttributeLocation(attribute.name),
						.vector_dimension = GL::GLint(accessor.vector_dimension),
						.vector_data_type = GL::GLenum(accessor.vector_data_type),
						.normalized = accessor.normalized,
					}
				);
			}

			//	TODO(bekorn): have a default material
			auto const material_index = primitive.material_index.has_value()
										? primitive.material_index.value()
										: throw std::runtime_error("not implemented");

			if (primitive.indices_accessor_index)
			{
				auto const & accessor_index = primitive.indices_accessor_index.value();
				auto const & buffer_view_index = gltf_data.accessors[accessor_index].buffer_view_index;
				auto const & buffer_view = gltf_data.buffer_views[buffer_view_index];
				auto const & buffer = gltf_data.buffers[buffer_view.buffer_index];

				auto & gl_element_buffer = buffers.back();
				gl_element_buffer.create(
					{
						.type = GL::GL_ELEMENT_ARRAY_BUFFER,
						.data = buffer.span_as<byte>(buffer_view.byte_offset, buffer_view.byte_length)
					}
				);

				GL::VAO_ElementDraw vao;
				vao.create(
					{
						.attributes = attributes,
						.element_array = gl_element_buffer,
					}
				);

				element_drawables.push_back(
					{
						.vao = std::move(vao),
						.material_index = material_index,
					}
				);
			}
			else
			{
				GL::VAO_ArrayDraw vao;
				vao.create(
					{
						.attributes = attributes,
					}
				);

				array_drawables.push_back(
					{
						.vao = std::move(vao),
						.material_index = material_index
					}
				);
			}
		}

		// Load Textures
		textures.resize(gltf_data.textures.size());
		for (auto i = 0; i < gltf_data.textures.size(); ++i)
		{
			auto const & texture = gltf_data.textures[i];

			// TODO(bekorn) have a default image
			auto const & image = texture.image_index.has_value()
								 ? gltf_data.images[texture.image_index.value()]
								 : throw std::runtime_error("not implemented");

			// TODO(bekorn) have a default image
			auto const & sampler = texture.sampler_index.has_value()
								   ? gltf_data.samplers[texture.sampler_index.value()]
								   : throw std::runtime_error("not implemented");

			textures[i].create(
				GL::Texture2D::ImageDescription{
					.dimensions = image.dimensions,
					.has_alpha = image.channels == 4,

					.min_filter = GL::GLenum(sampler.min_filter),
					.mag_filter = GL::GLenum(sampler.mag_filter),
					.wrap_s = GL::GLenum(sampler.wrap_s),
					.wrap_t = GL::GLenum(sampler.wrap_t),

					.data = image.data.data.get(),
				}
			);
		}

		// Load materials
		materials.resize(gltf_data.materials.size());
		for (auto i = 0; i < gltf_data.materials.size(); ++i)
		{
			auto const & gltf_mat = gltf_data.materials[i];
			if (gltf_mat.pbr_metallic_roughness)
			{
				auto const & pbr_mat = gltf_mat.pbr_metallic_roughness.value();
				auto mat = make_unique<Material_gltf_pbrMetallicRoughness>();

				// TODO: use texcoord indices as well
				if (pbr_mat.base_color_texture)
					mat->base_color_texture = textures[pbr_mat.base_color_texture->texture_index].id;
				else
					mat->base_color_factor = pbr_mat.base_color_factor;

				if (pbr_mat.metallic_roughness_texture)
					mat->metallic_roughness_texture = textures[pbr_mat.metallic_roughness_texture->texture_index].id;
				else
					mat->metallic_roughness_factor = {pbr_mat.metallic_factor, pbr_mat.roughness_factor};

				if (gltf_mat.emissive_texture)
					mat->emissive_texture = textures[gltf_mat.emissive_texture->texture_index].id;
				else
					mat->emissive_factor = gltf_mat.emissive_factor;

				if (gltf_mat.occlusion_texture)
					mat->occlusion_texture = textures[gltf_mat.occlusion_texture->texture_index].id;

				if (gltf_mat.normal_texture)
					mat->normal_texture = textures[gltf_mat.normal_texture->texture_index].id;

				materials[i] = std::move(mat);
			}
		}
	}
};