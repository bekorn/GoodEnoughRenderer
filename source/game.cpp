#include "game.hpp"

#include "Lib/core/core.hpp"
#include "Lib/core/named.hpp"

void Game::create_framebuffer()
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

void Game::create_uniform_buffers()
{
	using namespace GL;

	// Setup Lights Uniform Buffer
	auto & lights_uniform_block = assets.uniform_blocks.get("Lights"_name);

	lights_uniform_buffer.create(Buffer::UniformBlockDescription{
		.usage = GL::GL_DYNAMIC_DRAW,
		.uniform_block = lights_uniform_block,
		.array_size = 1,
	});

	glBindBufferBase(GL_UNIFORM_BUFFER, lights_uniform_block.binding, lights_uniform_buffer.id);

	auto * map = (byte *) glMapNamedBuffer(lights_uniform_buffer.id, GL_WRITE_ONLY);
	lights_uniform_block.set(map, "Lights[0].position", f32x3{0, 1, 3});
	lights_uniform_block.set(map, "Lights[0].color", f32x3{1, 0, 0});
	lights_uniform_block.set(map, "Lights[0].intensity", f32{5});
	lights_uniform_block.set(map, "Lights[0].is_active", true);

	lights_uniform_block.set(map, "Lights[1].position", f32x3{0, 1, -3});
	lights_uniform_block.set(map, "Lights[1].color", f32x3{0, 1, 0});
	lights_uniform_block.set(map, "Lights[1].intensity", f32{3});
	lights_uniform_block.set(map, "Lights[1].is_active", true);

	lights_uniform_block.set(map, "Lights[2].position", f32x3{-2, 3, 0});
	lights_uniform_block.set(map, "Lights[2].color", f32x3{1, 1, 1});
	lights_uniform_block.set(map, "Lights[2].intensity", f32{16});
	lights_uniform_block.set(map, "Lights[2].is_active", true);

	lights_uniform_block.set(map, "Lights[3].position", f32x3{-10, 1, 0});
	lights_uniform_block.set(map, "Lights[3].color", f32x3{0, 0, 1});
	lights_uniform_block.set(map, "Lights[3].intensity", f32{8});
	lights_uniform_block.set(map, "Lights[3].is_active", true);
	glUnmapNamedBuffer(lights_uniform_buffer.id);


	// Setup Camera Uniform Buffer
	auto & camera_uniform_block = assets.uniform_blocks.get("Camera"_name);

	camera_uniform_buffer.create(Buffer::UniformBlockDescription{
		.usage = GL_DYNAMIC_DRAW,
		.uniform_block = camera_uniform_block,
		.array_size = 1,
	});

	glBindBufferBase(GL_UNIFORM_BUFFER, camera_uniform_block.binding, camera_uniform_buffer.id);


	// Setup GLTF Material Block and Buffer
	auto & material_block = Render::Material_gltf_pbrMetallicRoughness::block;
	material_block.create({
		.layout = *std::ranges::find(
			assets.programs.get(GLTF::pbrMetallicRoughness_program_name).storage_block_mappings,
			"Material", &StorageBlockMapping::key
		)
	});

	gltf_material_buffer.create(Buffer::StorageBlockDescription{
		.usage = GL_DYNAMIC_DRAW,
		.storage_block = material_block,
		.array_size = assets.materials.resources.size(),
	});

	auto * buffer = (byte *) glMapNamedBuffer(gltf_material_buffer.id, GL_WRITE_ONLY);
	for (auto i = 0; auto & [name, material]: assets.materials)
	{
		gltf_material2index.emplace(name, i);
		material->write_to_buffer(buffer + i * material_block.aligned_size);
		i++;
	}
	glUnmapNamedBuffer(gltf_material_buffer.id);
}

void Game::create()
{
	create_framebuffer();
	create_uniform_buffers();

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
	auto & attribute_mappings = assets.programs.get(GLTF::pbrMetallicRoughness_program_name).attribute_mappings;
	for (auto & [_, mesh] : assets.meshes)
		for (auto & drawable : mesh.drawables)
			drawable.load(attribute_mappings);
}

void Game::render(GLFW::Window const & window, FrameInfo const & frame_info)
{
	using namespace GL;

	// Update gltf materials !!! Temporary
	{
		auto & block = Render::Material_gltf_pbrMetallicRoughness::block;

		while (not gltf_material_is_dirty.empty())
		{
			auto & name = gltf_material_is_dirty.front();
			auto & material = assets.materials.get(name);

			auto * map = (byte *) glMapNamedBufferRange(
				gltf_material_buffer.id,
				gltf_material2index.at(name) * block.aligned_size,
				block.data_size,
				BufferAccessMask::GL_MAP_WRITE_BIT
			);
			material->write_to_buffer(map);
			glUnmapNamedBuffer(gltf_material_buffer.id);

			gltf_material_is_dirty.pop();
		}
	}

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
		auto * map = (byte *) glMapNamedBuffer(camera_uniform_buffer.id, GL_WRITE_ONLY);
		camera_block.set(map, "CameraWorldPosition", camera_position);
		camera_block.set(map, "TransformV", view);
		camera_block.set(map, "TransformP", projection);
		camera_block.set(map, "TransformVP", view_projection);
		glUnmapNamedBuffer(camera_uniform_buffer.id);
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
				// Bind Material Buffer
				auto & material_block = Render::Material_gltf_pbrMetallicRoughness::block;
				glBindBufferRange(
					GL_SHADER_STORAGE_BUFFER, material_block.binding, gltf_material_buffer.id,
					gltf_material2index.at(drawable.named_material.name) * material_block.aligned_size,
					material_block.data_size
				);

				glBindVertexArray(drawable.vertex_array.id);
				glDrawElements(GL_TRIANGLES, drawable.vertex_array.element_count, GL_UNSIGNED_INT, nullptr);
			}
		}
}
