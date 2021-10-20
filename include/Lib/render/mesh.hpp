#pragma once

#include "Lib/core/core_types.hpp"
#include "Lib/opengl/.hpp"
#include "Lib/file_management/gltf_loader.hpp"

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
struct Mesh
{
	std::vector<GL::Buffer> buffers;
	std::vector<GL::VAO_ArrayDraw> array_vaos;
	std::vector<GL::VAO_ElementDraw> element_vaos;
	std::vector<GL::Texture2D> textures;

	Mesh() noexcept = default;
	Mesh(Mesh const &) noexcept = delete;
	Mesh(Mesh &&) noexcept = default;

	Mesh(GLTF::GLTFData const & gltf_data, u32 mesh_index) noexcept
	{
		// TODO(bekorn): This method creates a vbo for each attribute,
		//  it would be better to merge them in a single vbo

		// Limitation: Only the first primitive
		auto const & primitive = gltf_data.meshes[mesh_index].primitives[0];
		auto const attribute_size = primitive.attributes.size();

		buffers.resize(attribute_size + primitive.has_indices());
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

		if (primitive.has_indices())
		{
			auto const & accessor_index = primitive.indices_accessor_index;
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

			element_vaos.emplace_back();
			auto & vao = element_vaos.back();
			vao.create(
				{
					.attributes = attributes,
					.element_array = gl_element_buffer,
				}
			);
		}
		else
		{
			array_vaos.emplace_back();
			auto & vao = array_vaos.back();
			vao.create(
				{
					.attributes = attributes
				}
			);
		}

		// Load Textures
		textures.resize(gltf_data.textures.size());
		for (auto i = 0; i < gltf_data.textures.size(); ++i)
		{
			auto const & texture = gltf_data.textures[i];

			// TODO(bekorn) have a default image
			auto const & image = texture.is_default_image()
								 ? throw std::runtime_error("not implemented")
								 : gltf_data.images[texture.image_index];

			// TODO(bekorn) have a default image
			auto const & sampler = texture.is_default_sampler()
								   ? throw std::runtime_error("not implemented")
								   : gltf_data.samplers[texture.sampler_index];

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
	}
};