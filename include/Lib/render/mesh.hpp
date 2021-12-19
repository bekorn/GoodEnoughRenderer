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
	vector<GL::Buffer> buffers;
	vector<GL::Texture2D> textures;
	vector<unique_ptr<IMaterial>> materials;

	// Vertex attributes + material index
	vector<ArrayDrawable> array_drawables;
	vector<ElementDrawable> element_drawables;

	// Transform data
	f32x3 position{0, 0, 0};
	f32x3 rotation{0, 0, 0};
	f32 scale{1};

	Mesh() noexcept = default;
	Mesh(Mesh const &) noexcept = delete;
	Mesh(Mesh &&) noexcept = default;

	Mesh(GLTF::GLTFData const & gltf_data, u32 mesh_index) noexcept
	{
		// TODO(bekorn): This method creates a vbo for each attribute,
		//  it would be better to merge them in a single vbo

		// Load Buffers
		buffers.resize(gltf_data.buffer_views.size());
		for (auto i = 0; i < gltf_data.buffer_views.size(); ++i)
		{
			auto const & buffer_view = gltf_data.buffer_views[i];
			auto const & buffer = gltf_data.buffers[buffer_view.buffer_index];

			// element arrays will have the ARRAY_BUFFER type as well, but it doesn't matter apparently
			// no debug messages from Intel or Nvidia drivers, also using DSA API buffers can be typeless
			// see https://github.com/fendevel/Guide-to-Modern-OpenGL-Functions#glbuffer
			buffers[i].create(
				{
					.data = buffer.span_as<byte>(buffer_view.offset, buffer_view.length)
				}
			);
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

			auto const & sampler = texture.sampler_index.has_value()
								   ? gltf_data.samplers[texture.sampler_index.value()]
								   : GLTF::SamplerDefault;

			textures[i].create(
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

				materials[i] = move(mat);
			}
		}

		// Load Primitives
		for (auto const & primitive : gltf_data.meshes[mesh_index].primitives)
		{
			auto const attribute_size = primitive.attributes.size();

			vector<GL::Attribute::Description> attributes;
			attributes.reserve(attribute_size);
			for (auto i = 0; i < attribute_size; ++i)
			{
				auto const & attribute = primitive.attributes[i];
				auto const & accessor = gltf_data.accessors[attribute.accessor_index];
				auto const & buffer_view = gltf_data.buffer_views[accessor.buffer_view_index];

				attributes.push_back(
					{
						.buffer = buffers[accessor.buffer_view_index], // !!! This is a reference: if the vector allocates, there will be chaos
						.buffer_offset = 0, // because each attribute has its own VBO
						.buffer_stride = buffer_view.stride,

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

			if (primitive.indices_accessor_index.has_value())
			{
				auto const & accessor = gltf_data.accessors[primitive.indices_accessor_index.value()];

				GL::VAO_ElementDraw vao;
				vao.create(
					{
						.attributes = attributes,
						.element_array = buffers[accessor.buffer_view_index],
						.element_count = accessor.count,
					}
				);

				element_drawables.push_back(
					{
						.vao = move(vao),
						.material_index = material_index,
					}
				);
			}
			else
			{
				// assuming all the attributes have the same count
				auto const & accessor = gltf_data.accessors[primitive.attributes[0].accessor_index];
				auto vertex_count = accessor.count;

				GL::VAO_ArrayDraw vao;
				vao.create(
					{
						.attributes = attributes,
						.vertex_count = vertex_count,
					}
				);

				array_drawables.push_back(
					{
						.vao = move(vao),
						.material_index = material_index
					}
				);
			}
		}
	}

	glm::mat4x4 CalculateTransform() const
	{
		glm::mat4x4 transform(1);
		transform *= glm::translate(position);
		transform *= glm::orientate4(glm::radians(rotation));
		transform *= glm::scale(f32x3(scale));
		return transform;
	}
};