#include "game.hpp"

#include "Lib/core/core.hpp"
#include "Lib/core/named.hpp"
#include "Lib/opengl/globals.hpp"

// !!! Temporary
i32 const line_count = 8*8;
i32 const line_length = 128;
GL::VertexArray lines_vao;

void Game::create_framebuffer()
{
	framebuffer.init(GL::FrameBuffer::Desc{
		.resolution = {720, 720},
		.attachments = {
			{
				.type = &GL::FrameBuffer::color0,
				.desc = GL::Texture2D::AttachmentDesc{
					.internal_format = GL::GL_RGBA16F, // formats without alpha does not work with compute shaders
					.mag_filter = GL::GL_NEAREST,
				},
			},
			{
				.type = &GL::FrameBuffer::depth,
				.desc = GL::Texture2D::AttachmentDesc{
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

	frame_info_uniform_buffer.init(
		MappedBuffer::UniformBlockDesc{
			.uniform_block = frame_info_uniform_block,
			.array_size = 1,
		}
	);

	glBindBufferBase(GL_UNIFORM_BUFFER, frame_info_uniform_block.binding, frame_info_uniform_buffer.id);


	// Setup Lights Uniform Buffer
	auto & lights_uniform_block = assets.uniform_blocks.get("Lights"_name);

	lights_uniform_buffer.init(
		Buffer::UniformBlockDesc{
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

	camera_uniform_buffer.init(
		MappedBuffer::UniformBlockDesc{
			.uniform_block = camera_uniform_block,
			.array_size = 1,
		}
	);

	glBindBufferBase(GL_UNIFORM_BUFFER, camera_uniform_block.binding, camera_uniform_buffer.id);


	// Setup GLTF Material Block and Buffer
	auto & material_block = Render::Material_gltf_pbrMetallicRoughness::block;
	material_block.init(
		{
			.layout = GetMapping(
				assets.programs.get(GLTF::pbrMetallicRoughness_program_name).storage_block_mappings, "Material"
			)
		}
	);

	gltf_material_buffer.init(
		Buffer::StorageBlockDesc{
			.usage = GL_DYNAMIC_DRAW,
			.storage_block = material_block,
			.array_size = glm::max(usize(16), assets.materials.resources.size()),
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

void Game::init()
{
	create_framebuffer();
	create_uniform_buffers();

	camera = Render::PerspectiveCamera{
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

	// fallback to default envmap
	if (not assets.texture_cubemaps.contains(settings.envmap_diffuse, settings.envmap_specular))
	{
		array<u8x3, 6> pixels;
		for (auto & pixel: pixels)
			pixel = u8x3(0.4 * 255);

		auto desc = GL::TextureCubemap::ImageDesc{
			.face_dimensions = {1, 1},
			.data = {reinterpret_cast<byte*>(pixels.data()), pixels.size() * 3*1},
		};

		assets.texture_cubemaps.generate(settings.envmap_diffuse).data.init(desc);
		assets.texture_cubemaps.generate(settings.envmap_specular).data.init(desc);
	}


	// init lines_vao
	Geometry::Attributes vertex_attributes;
	vertex_attributes.try_emplace(
		Geometry::Attribute::Key{Geometry::Attribute::Key::POSITION, 0},
		Geometry::Attribute::Data{Geometry::Attribute::Type::F32, 3}
	);
	auto const element_count = line_count * (line_length - 1) * 2;
	lines_vao.init(GL::VertexArray::EmptyDesc{
		.vertex_count = line_count * line_length,
		.vertex_attributes = vertex_attributes,
		.element_count = element_count,
		.attribute_mappings = assets.programs.get("lines"_name).attribute_mappings,
		.usage = GL::GL_DYNAMIC_COPY,
	});
	array<u32, element_count> lines_indices;
	for (auto l = 0; l < line_count; l++)
	{
		auto vert_base = l * line_length;
		auto elem_base = l * (line_length - 1) * 2;

		for (auto i = 0; i < line_length - 1; i++)
		{
			auto vert_idx = vert_base + i;
			auto elem_idx = elem_base + i * 2;

			lines_indices[elem_idx + 0] = vert_idx + 0;
			lines_indices[elem_idx + 1] = vert_idx + 1;
		}
	}

	GL::glNamedBufferSubData(lines_vao.element_buffer.id, 0, element_count * sizeof(u32), lines_indices.data());
}

void Game::update(GLFW::Window const & window, Render::FrameInfo const & frame_info)
{
	//	camera movement with WASD+QE+CTRL
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
			auto view = visit([](Render::Camera auto & c) { return f32x3x3(c.get_view()); }, camera);
			movement = inverse(view) * movement;
			if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
				visit([movement](Render::Camera auto & c) { c.target += movement; }, camera);
			else
				visit([movement](Render::Camera auto & c) { c.position += movement; }, camera);
		}
	}

	// Update scene tree
	assets.scene_tree.update_transforms();
}

void Game::render(GLFW::Window const & window, Render::FrameInfo const & frame_info)
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
		block.set(map, "SecondsSinceStart", frame_info.seconds_since_start);
		block.set(map, "SecondsSinceLastFrame", frame_info.seconds_since_last_frame);
		glFlushMappedNamedBufferRange(frame_info_uniform_buffer.id, 0, block.aligned_size);
	}

	auto camera_position = visit([](Render::Camera auto & c) { return c.position; }, camera);
	auto view = visit([](Render::Camera auto & c) { return c.get_view(); }, camera);
	auto projection = visit([](Render::Camera auto & c) { return c.get_projection(); }, camera);
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
		// clearing color is unnecessary, envmap fills the background
		glClearNamedFramebufferfv(framebuffer.id, GL_DEPTH, 0, &clear_depth);
	}

	// Shading
	{
		glEnable(GL_CULL_FACE), glCullFace(GL_BACK);
		glColorMask(true, true, true, true), glDepthMask(true), glDepthFunc(GL_LESS);

		auto const & gltf_pbr_program = assets.programs.get(GLTF::pbrMetallicRoughness_program_name);
		glUseProgram(gltf_pbr_program.id);

		glUniformHandleui64ARB(
			GetLocation(gltf_pbr_program.uniform_mappings, "envmap_diffuse"),
			assets.texture_cubemaps.get(settings.envmap_diffuse).handle
		);
		glUniformHandleui64ARB(
			GetLocation(gltf_pbr_program.uniform_mappings, "envmap_specular"),
			assets.texture_cubemaps.get(settings.envmap_specular).handle
		);
		glUniformHandleui64ARB(
			GetLocation(gltf_pbr_program.uniform_mappings, "envmap_brdf_lut"),
			assets.textures.get("envmap_brdf_lut"_name).handle
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

	// Lines
	{
		auto & lines_generate_program = assets.programs.get("lines_generate");
		glUseProgram(lines_generate_program.id);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, lines_vao.vertex_buffer.id);
		glUniformHandleui64ARB(
			GetLocation(lines_generate_program.uniform_mappings, "sdf"),
			assets.volumes.get("voxels_linear_view").handle
		);
		glDispatchCompute(1, 1, 1);

		auto & lines_program = assets.programs.get("lines");
		glUseProgram(lines_program.id);

		glBindVertexArray(lines_vao.id);
		glDrawElements(GL_LINES, lines_vao.element_count, GL_UNSIGNED_INT, nullptr);
	}

	// environment mapping
	{
		auto invVP = inverse(f32x3x3(view_projection));
		auto view_dirs = invVP * f32x4x3{
			{-1, -1, +1}, // uv 0,0
			{+1, -1, +1}, // uv 1,0
			{-1, +1, +1}, // uv 0,1
			{+1, +1, +1}, // uv 1,1
		};

		glDepthMask(false), glDepthFunc(GL_LEQUAL);
		auto & environment_map_program = assets.programs.get("environment_mapping");
		glUseProgram(environment_map_program.id);
		glUniformHandleui64ARB(
			GetLocation(environment_map_program.uniform_mappings, "environment_map"),
			assets.texture_cubemaps.get(settings.envmap_specular).handle
		);
		glUniformMatrix4x3fv(
			GetLocation(environment_map_program.uniform_mappings, "view_directions"),
			1, false, begin(view_dirs)
		);
		glBindVertexArray(GL::dummy_vao.id);
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}

	// tone mapping (hdr -> ldr)
	{
		glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);
		glDepthMask(false), glDepthFunc(GL_ALWAYS);
		auto & tone_mapping_program = assets.programs.get("tone_mapping"_name);
		glUseProgram(tone_mapping_program.id);
		glUniformHandleui64ARB(
			GetLocation(tone_mapping_program.uniform_mappings, "color_attachment"),
			framebuffer.color0.handle
		);
		glBindVertexArray(GL::dummy_vao.id);
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}

	// gamma correction
	{
		glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);

		glViewport(0, 0, framebuffer.resolution.x, framebuffer.resolution.y);
		glDepthMask(false), glDepthFunc(GL_ALWAYS);

		// TODO(bekorn): move this into the core project (a new project besides game and editor)
		auto & gamma_correction_program = assets.programs.get("gamma_correction"_name);
		glUseProgram(gamma_correction_program.id);
		glUniformHandleui64ARB(
			GetLocation(gamma_correction_program.uniform_mappings, "color_attachment"),
			framebuffer.color0.handle
		);
		glBindVertexArray(GL::dummy_vao.id);
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}
}
