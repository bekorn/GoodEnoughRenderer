#include "game.hpp"

#include "Lib/core/core.hpp"
#include "Lib/core/named.hpp"

void Game::create_framebuffer()
{
	framebuffer.create(GL::FrameBuffer::Description{
		.resolution = {720, 720},
		.attachments = {
			{
				.type = &GL::FrameBuffer::color0,
				.description = GL::Texture2D::AttachmentDescription{
					.internal_format = GL::GL_RGBA16F, // formats wihtout alpha does not work with compute shaders
					.mag_filter = GL::GL_NEAREST,
				},
			},
			{
				.type = &GL::FrameBuffer::depth,
				.description = GL::Texture2D::AttachmentDescription{
					.internal_format = GL::GL_DEPTH_COMPONENT32F,
				},
			},
		}
	});
}

void Game::create_uniform_buffers()
{
	using namespace GL;

	// Setup FrameInfo uniform buffer
	auto & frame_info_uniform_block = assets.uniform_blocks.get("FrameInfo"_name);

	frame_info_uniform_buffer.create(
		MappedBuffer::UniformBlockDescription{
			.uniform_block = frame_info_uniform_block,
			.array_size = 1,
		}
	);

	glBindBufferBase(GL_UNIFORM_BUFFER, frame_info_uniform_block.binding, frame_info_uniform_buffer.id);


	// Setup Lights Uniform Buffer
	auto & lights_uniform_block = assets.uniform_blocks.get("Lights"_name);

	lights_uniform_buffer.create(
		Buffer::UniformBlockDescription{
			.usage = GL::GL_DYNAMIC_DRAW,
			.uniform_block = lights_uniform_block,
			.array_size = 1,
		}
	);

	glBindBufferBase(GL_UNIFORM_BUFFER, lights_uniform_block.binding, lights_uniform_buffer.id);

	auto * map = (byte *) glMapNamedBuffer(lights_uniform_buffer.id, GL_WRITE_ONLY);
	lights_uniform_block.set(map, "Lights[0].position", f32x3{0, 1, 3});
	lights_uniform_block.set(map, "Lights[0].color", f32x3{1, 0, 0});
	lights_uniform_block.set(map, "Lights[0].range", f32{2 * 2});
	lights_uniform_block.set(map, "Lights[0].is_active", true);

	lights_uniform_block.set(map, "Lights[1].position", f32x3{0, 1, -3});
	lights_uniform_block.set(map, "Lights[1].color", f32x3{0, 1, 0});
	lights_uniform_block.set(map, "Lights[1].range", f32{1.6 * 2});
	lights_uniform_block.set(map, "Lights[1].is_active", true);

	lights_uniform_block.set(map, "Lights[2].position", f32x3{-2, 3, 0});
	lights_uniform_block.set(map, "Lights[2].color", f32x3{1, 1, 1});
	lights_uniform_block.set(map, "Lights[2].range", f32{3.5 * 2});
	lights_uniform_block.set(map, "Lights[2].is_active", true);

	lights_uniform_block.set(map, "Lights[3].position", f32x3{-10, 1, 0});
	lights_uniform_block.set(map, "Lights[3].color", f32x3{0, 0, 1});
	lights_uniform_block.set(map, "Lights[3].range", f32{2.5 * 2});
	lights_uniform_block.set(map, "Lights[3].is_active", true);
	glUnmapNamedBuffer(lights_uniform_buffer.id);


	// Setup Camera Uniform Buffer
	auto & camera_uniform_block = assets.uniform_blocks.get("Camera"_name);

	camera_uniform_buffer.create(
		MappedBuffer::UniformBlockDescription{
			.uniform_block = camera_uniform_block,
			.array_size = 1,
		}
	);

	glBindBufferBase(GL_UNIFORM_BUFFER, camera_uniform_block.binding, camera_uniform_buffer.id);


	// Setup GLTF Material Block and Buffer
	auto & material_block = Render::Material_gltf_pbrMetallicRoughness::block;
	material_block.create(
		{
			.layout = GetMapping(
				assets.programs.get(GLTF::pbrMetallicRoughness_program_name).storage_block_mappings, "Material"
			)
		}
	);

	gltf_material_buffer.create(
		Buffer::StorageBlockDescription{
			.usage = GL_DYNAMIC_DRAW,
			.storage_block = material_block,
			.array_size = assets.materials.resources.size(),
		}
	);

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

	environment_mapping_timer.create();
	tone_mapping_timer.create();
	gamma_correction_timer.create();

	camera = PerspectiveCamera{
		.position = {5, 2, 0},
		.up = {0, 1, 0},
		.target = {0, 0, 0},
		.fov = 60,
		.near = 0.1,
		.far = 30,
		.aspect_ratio = f32(framebuffer.resolution.x) / f32(framebuffer.resolution.y),
	};

	// load all the meshes to the gpu
	auto & attribute_mappings = assets.programs.get(GLTF::pbrMetallicRoughness_program_name).attribute_mappings;
	for (auto & [_, mesh]: assets.meshes)
		for (auto & drawable: mesh.drawables)
			drawable.load(attribute_mappings);
}

void Game::update(GLFW::Window const & window, FrameInfo const & frame_info)
{
	//	cmaera movement with WASD+QE+CTRL
	{
		auto dir = f32x3(0);
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) dir.z -= 1;
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) dir.z += 1;
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) dir.x += 1;
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) dir.x -= 1;
		if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) dir.y += 1;
		if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) dir.y -= 1;

		const auto speed = 4.f;
		auto movement = speed * frame_info.seconds_since_last_frame * dir;
		if (any(notEqual(movement, f32x3(0))))
		{
			auto view = visit([](Camera auto & c) { return f32x3x3(c.get_view()); }, camera);
			movement = inverse(view) * movement;
			if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
				visit([movement](Camera auto & c) { c.target += movement; }, camera);
			else
				visit([movement](Camera auto & c) { c.position += movement; }, camera);
		}
	}

	// Update scene tree
	assets.scene_tree.update_transforms();
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

	// Update FrameInfo uniform buffer
	{
		auto & map = frame_info_uniform_buffer.map;
		auto & block = assets.uniform_blocks.get("FrameInfo"_name);
		block.set(map, "DepthAttachmentHandle", framebuffer.depth.handle);
		block.set(map, "ColorAttachmentHandle", framebuffer.color0.handle);
		block.set(map, "FrameIdx", frame_info.idx);
		block.set(map, "SecondsSinceStart", f32(frame_info.seconds_since_start));
		block.set(map, "SecondsSinceLastFrame", frame_info.seconds_since_last_frame);
		glFlushMappedNamedBufferRange(frame_info_uniform_buffer.id, 0, block.aligned_size);
	}

	auto camera_position = visit([](Camera auto & c) { return c.position; }, camera);
	auto view = visit([](Camera auto & c) { return c.get_view(); }, camera);
	auto projection = visit([](Camera auto & c) { return c.get_projection(); }, camera);
	auto view_projection = projection * view;

	// Update Camera Uniform Buffer
	{
		auto & map = camera_uniform_buffer.map;
		auto & block = assets.uniform_blocks.get("Camera"_name);
		block.set(map, "TransformV", view);
		block.set(map, "TransformP", projection);
		block.set(map, "TransformVP", view_projection);
		block.set(map, "TransformV_inv", glm::inverse(view));
		block.set(map, "TransformP_inv", glm::inverse(projection));
		block.set(map, "TransformVP_inv", glm::inverse(view_projection));
		block.set(map, "CameraWorldPosition", camera_position);
		glFlushMappedNamedBufferRange(camera_uniform_buffer.id, 0, block.aligned_size);
	}

	// Clear framebuffer
	{
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.id);
		glViewport(i32x2(0), framebuffer.resolution);
		glColorMask(true, true, true, true), glDepthMask(true);
		glClearNamedFramebufferfv(framebuffer.id, GL_COLOR, 0, begin(clear_color));
		glClearNamedFramebufferfv(framebuffer.id, GL_DEPTH, 0, &clear_depth);
	}

	glEnable(GL_CULL_FACE), glCullFace(GL_BACK);

	// zpass
	if (settings.is_zpass_on)
	{
		glColorMask(false, false, false, false), glDepthMask(true), glDepthFunc(GL_LESS);

		auto const & zpas_program = assets.programs.get("zpass");
		glUseProgram(zpas_program.id);

		auto location_TransformMVP = GetLocation(zpas_program.uniform_mappings, "TransformMVP");

		for (auto & depth: assets.scene_tree.nodes)
			for (auto & node: depth)
			{
				if (node.mesh == nullptr)
					continue;

				auto transform_mvp = view_projection * node.matrix;
				glUniformMatrix4fv(location_TransformMVP, 1, false, begin(transform_mvp));

				for (auto & drawable: node.mesh->drawables)
				{
					// TODO(bekorn): position being in location 0 for both pbr and zpass program is just coincidence,
					//  make it a guarantee!
					glBindVertexArray(drawable.vertex_array.id);
					glDrawElements(GL_TRIANGLES, drawable.vertex_array.element_count, GL_UNSIGNED_INT, nullptr);
				}
			}

		glColorMask(true, true, true, true), glDepthMask(false), glDepthFunc(GL_EQUAL);
	}

	// shading pass
	{
		if (not settings.is_zpass_on)
			glColorMask(true, true, true, true), glDepthMask(true), glDepthFunc(GL_LESS);

		auto const & gltf_pbr_program = assets.programs.get(GLTF::pbrMetallicRoughness_program_name);
		glUseProgram(gltf_pbr_program.id);

		glUniformHandleui64ARB(
			GetLocation(gltf_pbr_program.uniform_mappings, "diffuse_irradiance_map"),
			assets.texture_cubemaps.get(settings.diffuse_irradiance_map_name).handle
		);

		auto location_TransformM = GetLocation(gltf_pbr_program.uniform_mappings, "TransformM");
		auto location_TransformMVP = GetLocation(gltf_pbr_program.uniform_mappings, "TransformMVP");

		for (auto & depth: assets.scene_tree.nodes)
			for (auto & node: depth)
			{
				if (node.mesh == nullptr)
					continue;

				glUniformMatrix4fv(location_TransformM, 1, false, begin(node.matrix));
				auto transform_mvp = view_projection * node.matrix;
				glUniformMatrix4fv(location_TransformMVP, 1, false, begin(transform_mvp));

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

	// environment mapping
	{
		environment_mapping_timer.begin(frame_info.idx, frame_info.seconds_since_start);
		glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);

		auto invVP = inverse(f32x3x3(view_projection));
		auto view_dirs = invVP * f32x4x3{
			{-1, -1, +1}, // uv 0,0
			{+1, -1, +1}, // uv 1,0
			{-1, +1, +1}, // uv 0,1
			{+1, +1, +1}, // uv 1,1
		};

		if (settings.is_environment_mapping_comp)
		{
			auto & environment_map_program = assets.programs.get("environment_mapping_comp");
			glUseProgram(environment_map_program.id);
			glUniformHandleui64ARB(
				GetLocation(environment_map_program.uniform_mappings, "depth_attachment"),
				framebuffer.depth.handle
			);
			glBindImageTexture(
				GetLocation(environment_map_program.uniform_mappings, "color_attachment"),
				framebuffer.color0.id, 0,
				false, 0,
				GL_READ_WRITE,
				GL_RGBA16F
			);
			auto framebuffer_size = f32x2(framebuffer.resolution);
			glUniform2fv(
				GetLocation(environment_map_program.uniform_mappings, "framebuffer_size"),
				1, begin(framebuffer_size)
			);
			glUniformHandleui64ARB(
				GetLocation(environment_map_program.uniform_mappings, "environment_map"),
				assets.texture_cubemaps.get(settings.environment_map_name).handle
			);
			glUniformMatrix4x3fv(
				GetLocation(environment_map_program.uniform_mappings, "view_directions"),
				1, false, begin(view_dirs)
			);
			glDispatchCompute(framebuffer.resolution.x / 8, framebuffer.resolution.y / 8, 1);
		}
		else
		{
			glDepthMask(false), glDepthFunc(GL_LEQUAL);
			auto & environment_map_program = assets.programs.get("environment_mapping");
			glUseProgram(environment_map_program.id);
			glUniformHandleui64ARB(
				GetLocation(environment_map_program.uniform_mappings, "environment_map"),
				assets.texture_cubemaps.get(settings.environment_map_name).handle
			);
			glUniformMatrix4x3fv(
				GetLocation(environment_map_program.uniform_mappings, "view_directions"),
				1, false, begin(view_dirs)
			);
			glDrawArrays(GL_TRIANGLES, 0, 3);
		}
		environment_mapping_timer.end();
	}

	// tone mapping (hdr -> ldr)
	{
		tone_mapping_timer.begin(frame_info.idx, frame_info.seconds_since_start);

		glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);
		glDepthMask(false), glDepthFunc(GL_ALWAYS);
		auto & tone_mapping_program = assets.programs.get("tone_mapping"_name);
		glUseProgram(tone_mapping_program.id);
		glUniformHandleui64ARB(
			GetLocation(tone_mapping_program.uniform_mappings, "color_attachment"),
			framebuffer.color0.handle
		);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		tone_mapping_timer.end();
	}

	// gamma correction
	{
		gamma_correction_timer.begin(frame_info.idx, frame_info.seconds_since_start);
		glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);
		if (settings.is_gamma_correction_comp)
		{
			auto & gamma_correct_program = assets.programs.get("gamma_correction_comp"_name);
			glUseProgram(gamma_correct_program.id);
			glBindImageTexture(
				GetLocation(gamma_correct_program.uniform_mappings, "img"),
				framebuffer.color0.id, 0,
				false, 0,
				GL_READ_WRITE,
				GL_RGBA16F
			);
			glDispatchCompute(framebuffer.resolution.x / 8, framebuffer.resolution.y / 8, 1);
		}
		else
		{
			glDepthMask(false), glDepthFunc(GL_ALWAYS);
			auto & gamma_correction_program = assets.programs.get("gamma_correction"_name);
			glUseProgram(gamma_correction_program.id);
			glUniformHandleui64ARB(
				GetLocation(gamma_correction_program.uniform_mappings, "color_attachment"),
				framebuffer.color0.handle
			);
			glDrawArrays(GL_TRIANGLES, 0, 3);
		}
		gamma_correction_timer.end();
	}
}
