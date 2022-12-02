#include "editor_windows.hpp"

#include "game.hpp"

void GameSettingsWindow::update(Editor::Context & ctx)
{
	auto & game = static_cast<Game&>(ctx.game);

	using namespace ImGui;

	if (BeginCombo("Environment Map", game.settings.environment_map_name.string.data()))
	{
		for (auto & [name, _]: game.assets.texture_cubemaps)
			if (Selectable(name.string.data()))
				game.settings.environment_map_name = name;

		EndCombo();
	}

	Checkbox("Zpass", &game.settings.is_zpass_on);
	SameLine(), Checkbox("is env map comp", &game.settings.is_environment_mapping_comp);
	SameLine(), Checkbox("is gamma correction comp", &game.settings.is_gamma_correction_comp);

	static bool is_vsync_on = true;
	if (Checkbox("is vsync on", &is_vsync_on))
		glfwSwapInterval(is_vsync_on ? 1 : 0);
}

void MetricsWindow::update(Editor::Context & ctx)
{
	if constexpr (not GL::Timer::should_query)
		return;

	auto & game = static_cast<Game &>(ctx.game);

	{
		auto time = game.environment_mapping_timer.average_in_nanoseconds;
		Editor::TextFMT("{:30} {:6} us | {:6} ns", "Enrironment Mapping", time / 1'000, time);
	}
	{
		auto time = game.gamma_correction_timer.average_in_nanoseconds;
		Editor::TextFMT("{:30} {:6} us | {:6} ns", "Gamma Correction", time / 1'000, time);
	}
}


void MaterialWindow::update(Editor::Context & ctx)
{
	auto & game = static_cast<Game&>(ctx.game);

	using namespace ImGui;

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