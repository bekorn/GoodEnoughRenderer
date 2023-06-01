#include "editor_windows.hpp"

#include "game.hpp"

void GameSettingsWindow::update(Editor::Context & ctx)
{
	using namespace ImGui;

	auto & game = static_cast<Game&>(ctx.game);

	ColorEdit3("Clear color", begin(game.clear_color));

	static bool is_vsync_on = true;
	if (Checkbox("is vsync on", &is_vsync_on))
		glfwSwapInterval(is_vsync_on ? 1 : 0);
}

void MaterialWindow::update(Editor::Context & ctx)
{
	using namespace ImGui;

	auto & game = static_cast<Game&>(ctx.game);

	static Name material_name;
	if (BeginCombo("Material", material_name.string.data()))
	{
		for (auto const & [name, _]: game.assets.materials)
			if (Selectable(name.string.data()))
				material_name = name;

		EndCombo();
	}
	if (not game.assets.materials.contains(material_name))
	{
		Text("Pick a material");
		return;
	}
	auto & material = game.assets.materials.get(material_name);
	auto & block = material->get_block();

	auto buffer = ByteBuffer(block.data_size);
	material->write_to_buffer(buffer.begin());

	bool edited = false;
	for (auto & [name, variable]: block.variables)
		switch (variable.glsl_type)
		{
		case GL::GL_FLOAT:
		{
			edited |= DragFloat(name.data(), buffer.data_as<f32>(variable.offset));
			break;
		}
		case GL::GL_FLOAT_VEC2:
		{
			edited |= DragFloat2(name.data(), buffer.data_as<f32>(variable.offset));
			break;
		}
		case GL::GL_FLOAT_VEC3:
		{
			edited |= ColorEdit3(name.data(), buffer.data_as<f32>(variable.offset));
			break;
		}
		case GL::GL_FLOAT_VEC4:
		{
			edited |= ColorEdit4(name.data(), buffer.data_as<f32>(variable.offset));
			break;
		}
		case GL::GL_UNSIGNED_INT64_ARB:
		{
			// TODO(bekorn): display the texture
			// TODO(bekorn): should be editable
			LabelText(name.data(), "%llu", *buffer.data_as<u64>(variable.offset));
			break;
		}
		default:
		{ LabelText(name.data(), "%s is not supported", GL::glsl_uniform_type_to_string(variable.glsl_type)); }
		}

	if (edited)
	{
		material->read_from_buffer(buffer.begin());
		game.gltf_material_is_dirty.push(material_name); // !!! Temporary
	}
}

void Sdf3dWindow::init(Editor::Context const & ctx)
{
	auto & voxels = ctx.game.assets.volumes.generate(volume_name).data;

	voxels.init(GL::Texture3D::VolumeDesc{
		.dimensions = volume_res,
		.internal_format = GL::GL_RGBA8,

		.min_filter = GL::GL_NEAREST,
		.mag_filter = GL::GL_NEAREST,

		.wrap_s = GL::GL_CLAMP_TO_BORDER,
		.wrap_t = GL::GL_CLAMP_TO_BORDER,
		.wrap_r = GL::GL_CLAMP_TO_BORDER,
	});

	auto & voxels_linear_view = ctx.game.assets.volumes.generate(voxels_linear_view_name).data;

	voxels_linear_view.init(GL::Texture3D::ViewDesc{
		.source = voxels,

		.min_filter = GL::GL_LINEAR,
		.mag_filter = GL::GL_LINEAR,

		.wrap_s = GL::GL_CLAMP_TO_EDGE,
		.wrap_t = GL::GL_CLAMP_TO_EDGE,
		.wrap_r = GL::GL_CLAMP_TO_EDGE,
	});
}

void Sdf3dWindow::update(Editor::Context & ctx)
{
	using namespace ImGui;

	if (Button("Calculate"))
		should_voxelize = should_calculate_sdf = true;
	if (SameLine(), Checkbox("Every Frame", &should_calculate_sdf_every_frame); should_calculate_sdf_every_frame)
		should_voxelize = should_calculate_sdf = true;
	if (SameLine(), Button("Clear"))
		should_clear = true;

	InputInt("Fragment Multiplier", &fragment_multiplier);

	Checkbox("Visualize Volume", &should_visualize_volume);

	if (TreeNode("Debug"))
	{
		if (Button("Mesh > Voxel"))
			should_voxelize = true;

		if (Button("Voxel > SDF"))
			should_calculate_sdf = true;

		Checkbox("Visualize Voxels", &should_visualize_voxels);

		Checkbox("Visualize Isosurface", &should_visualize_isosurface);
		if (should_visualize_isosurface)
			SliderFloat("Value", &isosurface_value, 0, 1);

		TreePop();
	}
}

void Sdf3dWindow::render(Editor::Context const & ctx)
{
	if (should_clear)
	{
		should_clear = false;
		clear(ctx);
	}

	if (should_voxelize)
	{
		should_voxelize = false;
		voxelize(ctx);
	}

	if (should_calculate_sdf)
	{
		should_calculate_sdf = false;
		calculate_sdf(ctx);
	}

	if (should_visualize_volume)
		visualize_volume(ctx);

	if (should_visualize_voxels)
		visualize_voxels(ctx);

	if (should_visualize_isosurface)
		visualize_isosurface(ctx);
}

void Sdf3dWindow::clear(const Editor::Context & ctx)
{
	using namespace GL;

	auto clear_color = u8x4(0);
	glClearTexImage(
		ctx.game.assets.volumes.get(volume_name).id, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, begin(clear_color)
	);
}

void Sdf3dWindow::voxelize(const Editor::Context & ctx)
{
	using namespace GL;

	/// Get mesh
	const Render::Mesh * mesh = nullptr;
	f32x4x4 TransformM;

	auto & scene_tree = ctx.game.assets.scene_tree;
	auto & node_name = ctx.state.selected_node_name;
	if (auto it = scene_tree.named_indices.find(node_name); it != scene_tree.named_indices.end())
	{
		auto & node_idx = it->second;
		auto & node = scene_tree.get(node_idx);

		if (node.mesh != nullptr)
		{
			mesh = node.mesh;
			TransformM = node.matrix;
		}
	}

	if (mesh == nullptr)
	{
		fmt::print(stderr, "{}", "Please Select a Node with a mesh\n");
		return;
	}

	auto & voxels = ctx.game.assets.volumes.get(volume_name);
	{
		f32x4 clear_color(1, 1, 1, 1);
		glClearTexImage(voxels.id, 0, GL_RGBA, GL_FLOAT, begin(clear_color));
	}

	i32x2 render_res = fragment_multiplier * i32x2(volume_res);

	FrameBuffer fb;
	fb.init(FrameBuffer::EmptyDesc{
		.resolution = render_res,
	});
	glBindFramebuffer(GL_FRAMEBUFFER, fb.id);


	/// Mark surface voxels
	{
		glViewport({0, 0}, render_res);
		glDisable(GL_CULL_FACE);

		auto & voxelizer_program = ctx.game.assets.programs.get("voxelizer"_name);
		glUseProgram(voxelizer_program.id);
		glUniformHandleui64ARB(
			GetLocation(voxelizer_program.uniform_mappings, "voxels"),
			voxels.handle
		);
		glUniform3iv(
			GetLocation(voxelizer_program.uniform_mappings, "voxels_res"),
			1, begin(volume_res)
		);
		glUniform1iv(
			GetLocation(voxelizer_program.uniform_mappings, "fragment_multiplier"),
			1, &fragment_multiplier
		);
		glUniformMatrix4fv(
			GetLocation(voxelizer_program.uniform_mappings, "TransformM"),
			1, false, begin(TransformM)
		);
		for (auto & drawable : mesh->drawables)
		{
			glBindVertexArray(drawable.vertex_array.id);
			glDrawElements(GL_TRIANGLES, drawable.vertex_array.element_count, GL_UNSIGNED_INT, nullptr);
		}
	}

	/// Finalize
	{
		auto & voxelizer_finalize_program = ctx.game.assets.programs.get("voxelizer_finalize"_name);
		glUseProgram(voxelizer_finalize_program.id);
		glUniformHandleui64ARB(
			GetLocation(voxelizer_finalize_program.uniform_mappings, "voxels"),
			voxels.handle
		);
		glUniform3iv(
			GetLocation(voxelizer_finalize_program.uniform_mappings, "voxels_res"),
			1, begin(volume_res)
		);
		glDrawArrays(GL_POINTS, 0, compMul(volume_res));
	}
}

void Sdf3dWindow::calculate_sdf(const Editor::Context & ctx)
{
	using namespace GL;

	auto & volume = ctx.game.assets.volumes.get(volume_name);

	Texture3D temp_volume;
	temp_volume.init(Texture3D::VolumeDesc{
		.dimensions = volume_res,
		.internal_format = GL::GL_RGBA8,

		.min_filter = GL::GL_NEAREST,
		.mag_filter = GL::GL_NEAREST,

		.wrap_s = GL::GL_CLAMP_TO_BORDER,
		.wrap_t = GL::GL_CLAMP_TO_BORDER,
		.wrap_r = GL::GL_CLAMP_TO_BORDER,
	});

	// Even though the following draw calls do not render to a framebuffer, it fails to calculate sdf when the
	// fragment_multiplier is large enough. Setting viewport to something small prevents that
	glViewport(0, 0, 1, 1);


	/// Run jump flood
	{
		auto & jump_flood_program = ctx.game.assets.programs.get("jump_flood_3d");
		glUseProgram(jump_flood_program.id);

		i32 read_volume_unit;
		glGetUniformiv(
			jump_flood_program.id, GetLocation(jump_flood_program.uniform_mappings, "read_volume"),
			&read_volume_unit
		);
		i32 write_volume_unit;
		glGetUniformiv(
			jump_flood_program.id, GetLocation(jump_flood_program.uniform_mappings, "write_volume"),
			&write_volume_unit
		);

		glUniform3iv(
			GetLocation(jump_flood_program.uniform_mappings, "volume_res"),
			1, begin(volume_res)
		);

		i32 step = compMax(volume_res) / 2;
		bool write_to_temp = true; // initial data is stored in volume, so we write to temp_volume first

		while (step != 0)
		{
			glBindImageTexture(
				write_volume_unit,
				(write_to_temp ? temp_volume : volume).id, 0, 0, 0,
				GL_WRITE_ONLY, GL_RGBA8
			);
			glBindImageTexture(
				read_volume_unit,
				(not write_to_temp ? temp_volume : volume).id, 0, 0, 0,
				GL_READ_ONLY, GL_RGBA8
			);
			glUniform1i(
				GetLocation(jump_flood_program.uniform_mappings, "step"),
				step
			);
			glDrawArrays(GL_POINTS, 0, compMul(volume_res));

			step /= 2;
			write_to_temp = not write_to_temp;
		}

		if (not write_to_temp) // most recent write had made to temp_volume
			glCopyImageSubData( // volume = move(temp_volume) does not play well with editor ui currently
				temp_volume.id, GL_TEXTURE_3D, 0,
				0, 0, 0,
				volume.id, GL_TEXTURE_3D, 0,
				0, 0, 0,
				volume_res.x, volume_res.y, volume_res.z
			);
	}


	/// Finalize jump flood
	{
		auto & jump_flood_finalize = ctx.game.assets.programs.get("jump_flood_3d_finalize");
		glUseProgram(jump_flood_finalize.id);
		i32 volume_unit;
		glGetUniformiv(
			jump_flood_finalize.id, GetLocation(jump_flood_finalize.uniform_mappings, "volume"),
			&volume_unit
		);
		glBindImageTexture(
			volume_unit,
			volume.id, 0, 0, 0,
			GL_READ_WRITE, GL_RGBA8
		);
		glUniform3iv(
			GetLocation(jump_flood_finalize.uniform_mappings, "volume_res"),
			1, begin(volume_res)
		);
		glDrawArrays(GL_POINTS, 0, compMul(volume_res));
	}
}

void Sdf3dWindow::visualize_volume(const Editor::Context & ctx)
{
	using namespace GL;

	glBindFramebuffer(GL_FRAMEBUFFER, ctx.game.framebuffer.id);
	glViewport(i32x2(0), ctx.game.framebuffer.resolution);

	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST), glDepthFunc(GL_LESS), glDepthMask(false);

	auto & program = ctx.game.assets.programs.get("is_in_volume");
	glUseProgram(program.id);

	glUniformHandleui64ARB(
		GetLocation(program.uniform_mappings, "depth_attachment"),
		ctx.game.framebuffer.depth.handle
	);

	glDrawArrays(GL_POINTS, 0, 1);
}

void Sdf3dWindow::visualize_voxels(const Editor::Context & ctx)
{
	using namespace GL;

	if (not ctx.state.should_game_render)
		return;

	glBindFramebuffer(GL_FRAMEBUFFER, ctx.game.framebuffer.id);
	glViewport(i32x2(0), ctx.game.framebuffer.resolution);
	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST), glDepthFunc(GL_LESS), glDepthMask(true);

	auto & voxel_to_cube_program = ctx.game.assets.programs.get("voxel_to_cube");
	glUseProgram(voxel_to_cube_program.id);
	glUniformHandleui64ARB(
		GetLocation(voxel_to_cube_program.uniform_mappings, "volume"),
		ctx.game.assets.volumes.get(volume_name).handle
	);

	auto compute_res = volume_res + 1; // include boundaries, then ceil to multiple of 4  (see voxel_to_cube.vert.glsl)
	auto reminder = compute_res % 4;
	auto mask = i32x3(notEqual(reminder, i32x3(0)));
	compute_res += mask * (4 - reminder);
	glUniform3iv(
		GetLocation(voxel_to_cube_program.uniform_mappings, "compute_res"),
		1, begin(compute_res)
	);

	auto TransformVP = visit([](Render::Camera auto & c) { return c.get_projection() * c.get_view(); }, ctx.game.camera);
	auto transform = TransformVP * translate(f32x3(-1)) * scale(2.f / f32x3(volume_res)); // maps [0,volume_res] to [-1,+1]
	glUniformMatrix4fv(
		GetLocation(voxel_to_cube_program.uniform_mappings, "transform"),
		1, false, begin(transform)
	);

	glDrawArrays(GL_POINTS, 0, compMul(compute_res));
}

void Sdf3dWindow::visualize_isosurface(const Editor::Context & ctx)
{
	using namespace GL;

	if (not ctx.state.should_game_render)
		return;

	glBindFramebuffer(GL_FRAMEBUFFER, ctx.game.framebuffer.id);
	glViewport(i32x2(0), ctx.game.framebuffer.resolution);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST), glDepthFunc(GL_LESS), glDepthMask(true);

	auto & isosurface_program = ctx.game.assets.programs.get("isosurface");
	glUseProgram(isosurface_program.id);
	glUniformHandleui64ARB(
		GetLocation(isosurface_program.uniform_mappings, "sdf"),
		ctx.game.assets.volumes.get(voxels_linear_view_name).handle
	);
	glUniform1f(
		GetLocation(isosurface_program.uniform_mappings, "isosurface_value"),
		isosurface_value
	);

	glDrawArrays(GL_POINTS, 0, 1);
}
