#pragma once

#include "Lib/core/.hpp"
#include "Lib/opengl/.hpp"
#include "Lib/render/.hpp"
#include "Lib/asset_kitchen/glsl/.hpp"
#include "Lib/asset_kitchen/gltf/.hpp"

#include "assets.hpp"
#include "renderer.hpp"
#include "camera.hpp"

struct Game final : IRenderer
{
	Assets & assets;

	explicit Game(Assets & assets) :
		assets(assets)
	{}

	MOVE(Game, delete)
	COPY(Game, delete)

	// Settings
	f32x4 clear_color{0.45f, 0.55f, 0.60f, 1.00f};
	f32 clear_depth = 1;
	i32x2 resolution{720, 720};
	GL::FrameBuffer framebuffer;
	vector<GL::Texture2D> framebuffer_attachments;

	variant<PerspectiveCamera, OrthographicCamera> camera;

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

	void create() final
	{
		create_framebuffer();

		camera = PerspectiveCamera{
			.position = {0, 0, -5},
			.up = {0, 1, 0},
			.target = {0, 0, 0},
			.fov = 45,
			.near = 0.1,
			.far = 10,
			.aspect_ratio = f32(resolution.x) / f32(resolution.y),
		};
	}

	void render(GLFW::Window const & window, FrameInfo const & frame_data) final
	{
		using namespace GL;

		glViewport(0, 0, resolution.x, resolution.y);

		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.id);
		glClearNamedFramebufferfv(framebuffer.id, GL_COLOR, 0, begin(clear_color));
		glClearNamedFramebufferfv(framebuffer.id, GL_DEPTH, 0, &clear_depth);

		glEnable(GL_DEPTH_TEST);

		auto const view = visit([](Camera auto const & c){ return c.get_view(); }, camera);
		auto const projection = visit([](Camera auto const & c){ return c.get_projection(); }, camera);
		auto const view_projection = projection * view;

		glUseProgram(assets.programs.get(GLTF::pbrMetallicRoughness_program_name).id);

		for (auto const & [key, mesh]: assets.meshes.resources)
		{
			auto const model = mesh.CalculateTransform();
			auto const transform = view_projection * model;
			// TODO(bekorn): find a proper location
			glUniformMatrix4fv(10, 1, false, begin(transform));

			for (auto const & drawable: mesh.drawables)
			{
				drawable.named_material.data->set_uniforms();

				glBindVertexArray(drawable.vertex_array.id);
				glDrawElements(GL_TRIANGLES, drawable.vertex_array.element_count, GL_UNSIGNED_INT, nullptr);
			}
		}
	}
};