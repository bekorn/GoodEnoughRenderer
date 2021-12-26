#pragma once

#include "Lib/core/.hpp"
#include "Lib/opengl/.hpp"
#include "Lib/render/.hpp"
#include "Lib/asset_kitchen/glsl/.hpp"
#include "Lib/asset_kitchen/gltf/.hpp"

#include "globals.hpp"
#include "renderer.hpp"

struct Game : IRenderer
{
	// GL resources
	GL::ShaderProgram program; // will be a vector<shader> later
	vector<GL::Texture2D> textures;
	// Render resources
	vector<Geometry::Primitive> primitives;
	vector<unique_ptr<Render::IMaterial>> materials;
	vector<Render::Mesh> meshes;

	// Settings
	f32x4 clear_color{0.45f, 0.55f, 0.60f, 1.00f};
	f32 clear_depth = 1;
	i32x2 resolution{720, 720};
	GL::FrameBuffer framebuffer;
	vector<GL::Texture2D> framebuffer_attachments;

	void create_framebuffer()
	{
		framebuffer_attachments.resize(2);

		auto & color_attachment = framebuffer_attachments[0];
		color_attachment.create(
			GL::Texture2D::AttachmentDescription{
				.dimensions = resolution,
				.internal_format = GL::GL_RGB8,
			}
		);

		auto & depth_attachment = framebuffer_attachments[1];
		depth_attachment.create(
			GL::Texture2D::AttachmentDescription{
				.dimensions = resolution,
				.internal_format = GL::GL_DEPTH_COMPONENT32F,
			}
		);

		framebuffer.create(
			GL::FrameBuffer::Description{
				.attachments = {
					{
						.type = GL::GL_COLOR_ATTACHMENT0,
						.texture = color_attachment,
					},
					{
						.type = GL::GL_DEPTH_ATTACHMENT,
						.texture = depth_attachment,
					},
				}
			}
		);
	}

	void load_assets()
	{
		// TODO(bekorn): code duplication with the editor
		auto const glsl_data = GLSL::Load(
			{
				.stages = {
					{GL::GL_VERTEX_SHADER,   global_state.test_assets / "gltf_pbrMetallicRoughness.vert.glsl"},
					{GL::GL_FRAGMENT_SHADER, global_state.test_assets / "gltf_pbrMetallicRoughness.frag.glsl"},
				},
				.include_paths = {},
				.include_strings = {
					GL::GLSL_VERSION_MACRO,
				}
			}
		);

		if (auto expected = GLSL::Convert(glsl_data))
		{
			program = expected.into_result();
			program.update_interface_mapping();
			GL::glUseProgram(program.id);
		}
		else
		{
			std::cerr << expected.into_error();
		}


		auto const gltf_data = GLTF::Load(global_state.test_assets / "helmet/DamagedHelmet.gltf");
//		auto const gltf_data = GLTF::Load(global_state.test_assets / "avocado/Avocado.gltf");
//		auto const gltf_data = GLTF::Load(global_state.test_assets / "electric_guitar_fender_strat_plus/model.gltf");
//		auto const gltf_data = GLTF::Load(global_state.test_assets / "sponza/Sponza.gltf");
//		auto const gltf_data = GLTF::Load(global_state.test_assets / "flight_helmet/FlightHelmet.gltf");

		GLTF::Convert(gltf_data, textures, materials, primitives, meshes);
	}

	void create() override
	{
		create_framebuffer();
		load_assets();
	}

	void render(const GLFW::Window & w) override
	{
		using namespace GL;

		glViewport(0, 0, resolution.x, resolution.y);

		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.id);
		glClearNamedFramebufferfv(framebuffer.id, GL_COLOR, 0, begin(clear_color));
		glClearNamedFramebufferfv(framebuffer.id, GL_DEPTH, 0, &clear_depth);

		glEnable(GL_DEPTH_TEST);

		for (auto const & mesh: meshes)
		{
			auto transform = mesh.CalculateTransform();
			// TODO(bekorn): find a proper location
			glUniformMatrix4fv(10, 1, false, begin(transform));

			for (auto const & [_, material_ref, vertex_array]: mesh.primitives)
			{
				material_ref->get()->set_uniforms();

				glBindVertexArray(vertex_array.id);
				glDrawElements(GL_TRIANGLES, vertex_array.element_count, GL_UNSIGNED_INT, nullptr);
			}
		}
	}
};