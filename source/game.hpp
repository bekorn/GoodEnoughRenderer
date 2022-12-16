#pragma once

#include "Lib/asset_kitchen/assets.hpp"
#include "Lib/opengl/core.hpp"
#include "Lib/opengl/mapped_buffer.hpp"
#include "Lib/opengl/framebuffer.hpp"
#include "Lib/opengl/timer.hpp"
#include "Lib/glfw/core.hpp"
#include "Lib/render/frame_info.hpp"
#include "Lib/render/game_base.hpp"

struct Game : Render::GameBase
{
	explicit Game(Assets & assets):
		Render::GameBase(assets)
	{}

	MOVE(Game, delete)
	COPY(Game, delete)

	// Settings
	struct Settings
	{
		bool is_zpass_on = false;

		bool is_environment_mapping_comp = false;
		Name environment_map_name = "environment_map";
		Name diffuse_irradiance_map_name = "diffuse_irradiance_map";

		bool is_gamma_correction_comp = false;
	} settings;

	GL::MappedBuffer frame_info_uniform_buffer;
	GL::Buffer lights_uniform_buffer;
	GL::MappedBuffer camera_uniform_buffer;
	GL::Buffer gltf_material_buffer; 									// !!! Temporary
	std::unordered_map<Name, u32, Name::Hasher> gltf_material2index;	// !!! Temporary
	std::queue<Name> gltf_material_is_dirty;							// !!! Temporary

	GL::Timer environment_mapping_timer;
	GL::Timer tone_mapping_timer;
	GL::Timer gamma_correction_timer;

	void create_framebuffer();
	void create_uniform_buffers();
	void create() override;
	void update(GLFW::Window const & window, FrameInfo const & frame_info) override;
	void render(GLFW::Window const & window, FrameInfo const & frame_info) override;
};