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

	throw std::runtime_error("not implemented");
}

// Limitation: Only supports OpenGL
struct Mesh
{
	std::vector<std::shared_ptr<GL::Buffer>> buffers;
	std::vector<std::shared_ptr<GL::VAO_ArrayDraw>> array_vaos;
	std::vector<std::shared_ptr<GL::VAO_ElementDraw>> element_vaos;

	Mesh() noexcept = default;
	Mesh(Mesh const &) noexcept = delete;
	Mesh(Mesh &&) noexcept = default;

	Mesh(GLTF::GLTFData const & gltf_data, u32 mesh_index) noexcept
	{
		// TODO(bekorn): Probably it could be nice to reserve on all GLObject vectors beforehand
		//  (so no dangling reference pointers), and get rid of the shared_ptr

		// Limitation: Only the first primitive
		auto const & primitive = gltf_data.meshes[mesh_index].primitives[0];

		std::vector<GL::Attribute::Description> attributes;
		attributes.reserve(primitive.attributes.size());
		for (auto const & attribute: primitive.attributes)
		{
			auto const & accessor = gltf_data.accessors[attribute.accessor_index];
			auto const & buffer_view = gltf_data.buffer_views[accessor.buffer_view_index];
			auto const & buffer = gltf_data.buffers[buffer_view.buffer_index];

			buffers.emplace_back(std::make_unique<GL::Buffer>());
			auto & gl_buffer = *buffers.back();
			gl_buffer.create(
				{
					.type = GL::GL_ARRAY_BUFFER,
					.data = buffer.span_as<byte>(buffer_view.byte_offset, buffer_view.byte_length)
				}
			);

			attributes.push_back(
				{
					.buffer = gl_buffer,
//					.byte_offset = buffer_view.byte_offset,
					.byte_offset = 0, // because each attribute has its own VBO
					.location = GetGLConventionAttributeLocation(attribute.name),
					.vector_dimension = accessor.vector_dimension,
					.vector_data_type = GL::GLenum(accessor.vector_data_type),
					.normalized = accessor.normalized,
				}
			);
		}

		if (primitive.has_indices())
		{
			auto const & buffer_view_index = gltf_data.accessors[primitive.indices_accessor_index].buffer_view_index;
			auto const & buffer_view = gltf_data.buffer_views[buffer_view_index];
			auto const & buffer = gltf_data.buffers[buffer_view.buffer_index];

			buffers.emplace_back(std::make_unique<GL::Buffer>());
			auto & gl_element_buffer = *buffers.back();
			gl_element_buffer.create(
				{
					.type = GL::GL_ELEMENT_ARRAY_BUFFER,
					.data = buffer.span_as<byte>(buffer_view.byte_offset, buffer_view.byte_length)
				}
			);

			element_vaos.emplace_back(std::make_unique<GL::VAO_ElementDraw>());
			auto & vao = *element_vaos.back();
			vao.create(
				GL::VAO_ElementDraw::Description{
					.attributes = attributes,
					.element_array = gl_element_buffer,
				}
			);
		}
		else
		{
			array_vaos.emplace_back(std::make_unique<GL::VAO_ArrayDraw>());
			auto & vao = *array_vaos.back();
			vao.create(
				{
					.attributes = attributes
				}
			);
		}
	}


};