#pragma once

#include "Lib/asset_kitchen/assets.hpp"
#include "Lib/opengl/core.hpp"
#include "Lib/opengl/mapped_buffer.hpp"
#include "Lib/opengl/framebuffer.hpp"
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

	variant<PerspectiveCamera, OrthographicCamera> camera;

	GL::MappedBuffer frame_info_uniform_buffer;
	GL::Buffer lights_uniform_buffer;
	GL::MappedBuffer camera_uniform_buffer;
	GL::Buffer gltf_material_buffer; 									// !!! Temporary
	std::unordered_map<Name, u32, Name::Hasher> gltf_material2index;	// !!! Temporary
	std::queue<Name> gltf_material_is_dirty;							// !!! Temporary

	void create_framebuffer();
	void create_uniform_buffers();
	void create();
	void update(GLFW::Window const & window, FrameInfo const & frame_info);
	void render(GLFW::Window const & window, FrameInfo const & frame_info);
};