#pragma once

#include "Lib/asset_kitchen/assets.hpp"

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
	GL::Buffer gltf_material_buffer; 									// !!! Temporary
	std::unordered_map<Name, u32, Name::Hasher> gltf_material2index;	// !!! Temporary
	std::queue<Name> gltf_material_is_dirty;							// !!! Temporary

	void create_framebuffer();
	void create_uniform_buffers();
	void create() final;
	void render(GLFW::Window const & window, FrameInfo const & frame_info) final;
};