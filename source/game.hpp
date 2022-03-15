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

	GL::Buffer lights_uniform_buffer;
	GL::Buffer camera_uniform_buffer;

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
			.position = {5, 2, 0},
			.up = {0, 1, 0},
			.target = {0, 0, 0},
			.fov = 60,
			.near = 0.1,
			.far = 30,
			.aspect_ratio = f32(resolution.x) / f32(resolution.y),
		};

		// load all the meshes to the gpu
		for (auto & [_, mesh] : assets.meshes)
			for (auto & drawable : mesh.drawables)
				drawable.load(assets.programs.get(GLTF::pbrMetallicRoughness_program_name));


		// Setup Lights Uniform Buffer
		auto & lights_uniform_block = assets.uniform_blocks.get("Lights"_name);

		lights_uniform_buffer.create(GL::Buffer::UniformBlockDescription{
			.usage = GL::GL_DYNAMIC_DRAW,
			.uniform_block = lights_uniform_block,
			.array_size = 1,
		});

		GL::glBindBufferBase(GL::GL_UNIFORM_BUFFER, lights_uniform_block.binding, lights_uniform_buffer.id);

		auto * map = (byte *) GL::glMapNamedBuffer(lights_uniform_buffer.id, GL::GL_WRITE_ONLY);
		lights_uniform_block.set(map, "Lights[0].position", f32x3{0, 1, 3});
		lights_uniform_block.set(map, "Lights[0].color", f32x3{1, 0, 0});
		lights_uniform_block.set(map, "Lights[0].intensity", f32{5});
		lights_uniform_block.set(map, "Lights[0].is_active", true);

		lights_uniform_block.set(map, "Lights[1].position", f32x3{0, 1, -3});
		lights_uniform_block.set(map, "Lights[1].color", f32x3{0, 1, 0});
		lights_uniform_block.set(map, "Lights[1].intensity", f32{3});
		lights_uniform_block.set(map, "Lights[1].is_active", true);

		lights_uniform_block.set(map, "Lights[3].position", f32x3{-10, 1, 0});
		lights_uniform_block.set(map, "Lights[3].color", f32x3{0, 0, 1});
		lights_uniform_block.set(map, "Lights[3].intensity", f32{8});
		lights_uniform_block.set(map, "Lights[3].is_active", true);
		GL::glUnmapNamedBuffer(lights_uniform_buffer.id);


		// Setup Camera Uniform Buffer
		auto & camera_uniform_block = assets.uniform_blocks.get("Camera"_name);

		camera_uniform_buffer.create(GL::Buffer::UniformBlockDescription{
			.usage = GL::GL_DYNAMIC_DRAW,
			.uniform_block = camera_uniform_block,
			.array_size = 1,
		});

		GL::glBindBufferBase(GL::GL_UNIFORM_BUFFER, camera_uniform_block.binding, camera_uniform_buffer.id);
	}

	void render(GLFW::Window const & window, FrameInfo const & frame_info) final
	{
		using namespace GL;

		glViewport(0, 0, resolution.x, resolution.y);

		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.id);
		glClearNamedFramebufferfv(framebuffer.id, GL_COLOR, 0, begin(clear_color));
		glClearNamedFramebufferfv(framebuffer.id, GL_DEPTH, 0, &clear_depth);

		glEnable(GL_DEPTH_TEST);

		auto camera_position = visit([](Camera auto & c){ return c.position; }, camera);
		auto view = visit([](Camera auto & c){ return c.get_view(); }, camera);
		auto projection = visit([](Camera auto & c){ return c.get_projection(); }, camera);
		auto view_projection = projection * view;

		// Update Camera Uniform Buffer
		{
			auto & camera_block = assets.uniform_blocks.get("Camera"_name);
			auto * map = (byte *) GL::glMapNamedBuffer(camera_uniform_buffer.id, GL::GL_WRITE_ONLY);
			camera_block.set(map, "CameraWorldPosition", camera_position);
			camera_block.set(map, "TransformV", view);
			camera_block.set(map, "TransformP", projection);
			camera_block.set(map, "TransformVP", view_projection);
			GL::glUnmapNamedBuffer(camera_uniform_buffer.id);
		}

		glUseProgram(assets.programs.get(GLTF::pbrMetallicRoughness_program_name).id);

		assets.scene_tree.update_transforms();

		for (auto & depth: assets.scene_tree.nodes)
			for (auto & node: depth)
			{
				if (node.mesh == nullptr)
					continue;

				// TODO(bekorn): find a proper locations for these uniforms
				glUniformMatrix4fv(10, 1, false, begin(node.matrix));
				auto transform_mvp = view_projection * node.matrix;
				glUniformMatrix4fv(11, 1, false, begin(transform_mvp));

				for (auto & drawable: node.mesh->drawables)
				{
					drawable.named_material.data.get()->set_uniforms();

					glBindVertexArray(drawable.vertex_array.id);
					glDrawElements(GL_TRIANGLES, drawable.vertex_array.element_count, GL_UNSIGNED_INT, nullptr);
				}
			}
	}
};