#pragma once

#include <asset_recipes/assets.hpp>
#include <opengl/core.hpp>
#include <opengl/mapped_buffer.hpp>
#include <opengl/framebuffer.hpp>
#include <render/glfw.hpp>
#include <render/frame_info.hpp>
#include <render/game_base.hpp>

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
		Name envmap_diffuse = "envmap_diffuse";
		Name envmap_specular = "envmap_specular";

		bool is_gamma_correction_comp = false;

		bool is_lines_active = false;
		i32 lines_update_per_frame = 8;
	} settings;

	GL::MappedBuffer frame_info_uniform_buffer;
	GL::Buffer lights_uniform_buffer;
	GL::MappedBuffer camera_uniform_buffer;
	GL::Buffer gltf_material_buffer; 									// !!! Temporary
	std::unordered_map<Name, u32, Name::Hasher> gltf_material2index;	// !!! Temporary
	std::queue<Name> gltf_material_is_dirty;							// !!! Temporary

	void create_framebuffer();
	void create_uniform_buffers();
	void init() override;
	void update(GLFW::Window const & window, Render::FrameInfo const & frame_info) override;
	void render(GLFW::Window const & window, Render::FrameInfo const & frame_info) override;
};