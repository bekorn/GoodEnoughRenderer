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
		{ LabelText(name.data(), "%s is not supported", GL::GLSLTypeToString(variable.glsl_type).data()); }
		}

	if (edited)
	{
		material->read_from_buffer(buffer.begin());
		game.gltf_material_is_dirty.push(material_name); // !!! Temporary
	}
}

void Voxelizer::init(Editor::Context const & ctx)
{
	auto & voxels = ctx.game.assets.volumes.generate(voxels_name).data;

	voxels.init(GL::Texture3D::VolumeDesc{
		.dimensions = voxels_res,
		.internal_format = GL::GL_RGBA8,

		.min_filter = GL::GL_NEAREST,
		.mag_filter = GL::GL_NEAREST,

		.wrap_s = GL::GL_CLAMP_TO_BORDER,
		.wrap_t = GL::GL_CLAMP_TO_BORDER,
		.wrap_r = GL::GL_CLAMP_TO_BORDER,
	});
}

void Voxelizer::update(Editor::Context & ctx)
{
	using namespace ImGui;

	if (Button("Compute"))
		should_compute = true;

	if (SameLine(), Button("Clear"))
		should_clear = true;

	Checkbox("Visualize Voxels", &should_visualize_voxels);

	InputInt("fragment_multiplier", &fragment_multiplier);
}

void Voxelizer::render(Editor::Context const & ctx)
{
	if (should_clear)
	{
		should_clear = false;
		clear(ctx);
	}

	if (should_compute)
	{
		should_compute = false;
		voxelize(ctx);
	}

	if (should_visualize_voxels)
		visualize_voxels(ctx);
}

void Voxelizer::clear(const Editor::Context & ctx)
{
	using namespace GL;

	auto clear_color = u8x4(0);
	glClearTexSubImage(
		ctx.game.assets.volumes.get(voxels_name).id, 0,
		0, 0, 0,
		voxels_res.x, voxels_res.y, voxels_res.z,
		GL_RGBA, GL_UNSIGNED_BYTE, begin(clear_color)
	);
}

void Voxelizer::voxelize(const Editor::Context & ctx)
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

	auto & voxels = ctx.game.assets.volumes.get(voxels_name);

	i32x2 render_res = fragment_multiplier * i32x2(voxels_res);

	FrameBuffer fb;
	fb.init(FrameBuffer::EmptyDesc{
		.resolution = render_res,
	});
	glBindFramebuffer(GL_FRAMEBUFFER, fb.id);

	/// Mark surface voxels
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
		1, begin(voxels_res)
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

	fmt::print("Voxelized {}\n", node_name);
}

void Voxelizer::visualize_voxels(const Editor::Context & ctx)
{
	using namespace GL;

	if (not ctx.state.should_game_render)
		return;

	glBindFramebuffer(GL_FRAMEBUFFER, ctx.game.framebuffer.id);
	glViewport(i32x2(0), ctx.game.framebuffer.resolution);
	glEnable(GL_DEPTH_TEST), glDepthFunc(GL_LESS), glDepthMask(true);

	auto & voxel_to_cube_program = ctx.game.assets.programs.get("voxel_to_cube");
	glUseProgram(voxel_to_cube_program.id);
	glUniformHandleui64ARB(
		GetLocation(voxel_to_cube_program.uniform_mappings, "volume"),
		ctx.game.assets.volumes.get(voxels_name).handle
	);
	glUniform3iv(
		GetLocation(voxel_to_cube_program.uniform_mappings, "volume_res"),
		1, begin(voxels_res)
	);
	auto compute_res = voxels_res + 1; // include boundaries, then ceil to multiple of 4  (see voxel_to_cube.vert.glsl)
	auto reminder = compute_res % 4;
	auto mask = i32x3(notEqual(reminder, i32x3(0)));
	compute_res += mask * (4 - reminder);
	glUniform3iv(
		GetLocation(voxel_to_cube_program.uniform_mappings, "compute_res"),
		1, begin(compute_res)
	);
	glDrawArrays(GL_POINTS, 0, compMul(compute_res));
}