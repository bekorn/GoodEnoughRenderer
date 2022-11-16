#pragma once

#include "Lib/asset_kitchen/assets.hpp"
#include "Lib/opengl/core.hpp"
#include "Lib/opengl/mapped_buffer.hpp"
#include "Lib/opengl/framebuffer.hpp"
#include "Lib/opengl/timer.hpp"
#include "Lib/glfw/core.hpp"

#include "frame_info.hpp"
#include "camera.hpp"

struct Game
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
	GL::Texture2D framebuffer_depth_attachment;
	GL::Texture2D framebuffer_color_attachment;

	struct Settings
	{
		bool is_zpass_on = false;

		bool is_environment_mapping_comp = false;
		Name environment_map_name = "environment_map";

		bool is_gamma_correction_comp = false;
	} settings;

	variant<PerspectiveCamera, OrthographicCamera> camera;

	GL::MappedBuffer frame_info_uniform_buffer;
	GL::Buffer lights_uniform_buffer;
	GL::MappedBuffer camera_uniform_buffer;
	GL::Buffer gltf_material_buffer; 									// !!! Temporary
	std::unordered_map<Name, u32, Name::Hasher> gltf_material2index;	// !!! Temporary
	std::queue<Name> gltf_material_is_dirty;							// !!! Temporary

	GL::Timer environment_map_timer;
	GL::Timer gamma_correction_timer;

	void create_framebuffer();
	void create_uniform_buffers();
	void create();
	void update(GLFW::Window const & window, FrameInfo const & frame_info);
	void render(GLFW::Window const & window, FrameInfo const & frame_info);
};