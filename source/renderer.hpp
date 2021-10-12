#pragma once

#include "Lib/glfw/window.hpp"
#include "Lib/opengl/.hpp"

// TODO(bekorn): pass this without globals
struct GlobalState
{
	std::filesystem::path const test_assets = R"(E:\Users\Berk\Desktop\Projeler\Portfolio\GoodEnoughRenderer\test_assets)";
} global_state;

struct IRenderer
{
	virtual void create() = 0;

	virtual void render(GLFW::Window const & w) = 0;
};


struct MainRenderer : IRenderer
{
	// Settings
	bool show_demo_window = false;

	f32x4 clear_color = f32x4(0.45f, 0.55f, 0.60f, 1.00f);
	i32 active_vao = 1;
	bool enable_depth_test = true;

	//	std::string shader_name = "basic";
	//
	//	ShaderHandle shader_handles[ShaderHandle::STAGE_COUNT] = {
	//		ShaderHandle(shader_name.data(), (ShaderHandle::Stage) 0),
	//		ShaderHandle(shader_name.data(), (ShaderHandle::Stage) 1),
	//		ShaderHandle(shader_name.data(), (ShaderHandle::Stage) 2)
	//	};
	//
	//	void update_handles(std::string const & name)
	//	{
	//		for (auto & handle : shader_handles)
	//			handle = ShaderHandle(name, handle.stage);
	//	}
	//
	//	// Persistent resources
	//	ShaderProgram program;
	//
	//	Texture2D render_target;

	std::vector<std::unique_ptr<GL::Buffer>> gl_buffers;
	std::vector<std::unique_ptr<GL::VAO_ElementDraw>> element_vaos;
	std::vector<std::unique_ptr<GL::VAO_ArrayDraw>> array_vaos;

	std::string status;

	GLTF::GLTFData gltf_data;

	void create() final
	{
		gltf_data = GLTF::Load(global_state.test_assets / "vertex colored cube.gltf");

		// Only meshes[0].primitives[0] for now
		auto const & primitive = gltf_data.meshes[0].primitives[0];

		std::vector<GL::Attribute::Description> attributes;
		attributes.reserve(primitive.attributes.size());
		for (auto const & attribute: primitive.attributes)
		{
			auto const & accessor = gltf_data.accessors[attribute.accessor_index];
			auto const & buffer_view = gltf_data.buffer_views[accessor.buffer_view_index];
			auto const & buffer = gltf_data.buffers[buffer_view.buffer_index];

			gl_buffers.emplace_back(std::make_unique<GL::Buffer>());
			auto & gl_buffer = *gl_buffers.back();
			gl_buffer.create(
				{
					.type = GL::GL_ARRAY_BUFFER,
					.data = buffer.span_as<byte>(buffer_view.byte_offset, buffer_view.byte_length)
				}
			);

			attributes.push_back(
				GL::Attribute::Description{
					.buffer = gl_buffer,
					.location = 0,
					.vector_dimension = accessor.vector_dimension,
					.vector_data_type = GL::GLenum(accessor.vector_data_type),
				}
			);
		}

		if (primitive.has_indices())
		{
			auto const & buffer_view_index = gltf_data.accessors[primitive.indices_accessor_index].buffer_view_index;
			auto const & buffer_view = gltf_data.buffer_views[buffer_view_index];
			auto const & buffer = gltf_data.buffers[buffer_view.buffer_index];

			gl_buffers.emplace_back(std::make_unique<GL::Buffer>());
			auto & gl_element_buffer = *gl_buffers.back();
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

	void metrics_window()
	{
		using namespace ImGui;

		SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Once);
		SetNextWindowSize(ImVec2(600, 160), ImGuiCond_Once);
		Begin("Metrics", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

		Text(
			"Application average %.3f ms/frame (%.1f FPS)",
			1000.f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate
		);

		Separator();

		Checkbox("Demo Window", &show_demo_window); // Edit bools storing our window open/close state

		Separator();

		Text("|> %s", status.c_str());

		End();
	}

	/*
	void program_window()
	{
		using namespace ImGui;

		Begin("Program Controls"); // Create a window called "Hello, world!" and append into it.

		if (Text("Name"), SameLine(), InputText("", state.shader_name.data(), state.shader_name.size()))
			state.update_handles(state.shader_name.data());


		{
			auto & vert = state.shader_handles[ShaderHandle::VERT];
			Text("vert = %s", vert.path.string().c_str());
			bool value = vert.is_file();
			SameLine(), ImGui::Checkbox("Exists", &value);
		}
		{
			auto & frag = state.shader_handles[ShaderHandle::FRAG];
			Text("frag = %s", frag.path.string().c_str());
			bool value = frag.is_file();
			SameLine(), ImGui::Checkbox("Exists", &value);
		}
		{
			auto & geom = state.shader_handles[ShaderHandle::GEOM];
			Text("geom = %s", geom.path.string().c_str());
			bool value = geom.is_file();
			SameLine(), ImGui::Checkbox("Exists", &value);
		}

		if (Button("Compile", {100, GetTextLineHeight() * 1.5f}))
		{
			if (state.program.id != 0)
			{
				glDeleteProgram(state.program.id);
				state.program.id = 0;
			}

			state.program.create(
				{
					.name = state.shader_name.data()
				}
			);

			if (state.program.id != 0)
			{
				std::clog << (state.status = "Compiled successfully") << '\n';
			}
			else
			{
				GLint maxLength = 0;
				glGetShaderiv(state.program.id, GL_INFO_LOG_LENGTH, &maxLength);

				// The maxLength includes the NULL character
				std::vector<GLchar> errorLog(maxLength);
				glGetShaderInfoLog(state.program.id, maxLength, &maxLength, errorLog.data());

				std::cerr << (state.status = "Compilation failed :(\n"s + errorLog.data()) << '\n';
			}
		}

		NewLine();
		Text("Program ID: %i", state.program.id);

		End();
	}
	*/

	void realtime_settings_window()
	{
		using namespace ImGui;

		Begin("Realtime Settings");

		ColorEdit3("clear color", glm::value_ptr(clear_color));

		Checkbox("Enable Depth Test", &enable_depth_test);

		SliderInt("Active VAO", &active_vao, 0, 1);

		End();
	}

	void render(GLFW::Window const & window) final
	{
		using namespace GL;

		glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// UI windows
		metrics_window();
		//		program_window();
		realtime_settings_window();

		for (auto const & vao : element_vaos)
		{
			glBindVertexArray(vao->id);
			glDrawElements(GL_TRIANGLES, vao->element_count, GL_UNSIGNED_SHORT, nullptr);
		}
	}
};