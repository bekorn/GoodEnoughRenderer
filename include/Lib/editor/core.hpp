#pragma once

#include <core/core.hpp>
#include "Lib/opengl/framebuffer.hpp"
#include "Lib/opengl/texture_2d.hpp"
#include "Lib/render/frame_info.hpp"
#include "Lib/render/game_base.hpp"
#include "Lib/asset_kitchen/assets.hpp"
#include "Lib/glfw/core.hpp"
#include "Lib/imgui/core.hpp"

namespace Editor
{
struct Context;

struct WindowBase
{
	enum struct State : u8
	{ OKAY, WARNING, ERROR }
	state = State::OKAY;

	CTOR(WindowBase, default);
	COPY(WindowBase, delete);
	MOVE(WindowBase, default);
	virtual ~WindowBase() = default;

	virtual const char * get_name() = 0;

	virtual void init(Context const & ctx)
	{};

	virtual void update(Context & ctx)
	{};

	virtual void render(Context const & ctx)
	{};
};

struct State
{
	CTOR(State, default);
	COPY(State, delete);
	MOVE(State, default);

	Render::FrameInfo frame_info;
	f64 game_update_in_seconds;
	f64 game_render_in_seconds;

	bool should_game_render = true;
	bool has_program_errors;

	// selections
	Name selected_uniform_buffer_name;
	Name selected_program_name;
	Name selected_texture_name;
	Name selected_cubemap_name;
	Name selected_volume_name;
	Name selected_mesh_name;
	Name selected_node_name;
};

// TODO move to cpp
struct Context
{
	Render::GameBase & game;

	State state;
	Assets & editor_assets;

	vector<unique_one<WindowBase>> windows;
	GLFW::Window const & gltf_window;

	Context(Render::GameBase & game, Assets & editor_assets, GLFW::Window const & gltf_window) :
		game(game), editor_assets(editor_assets), gltf_window(gltf_window)
	{}

	void init()
	{
		// Enable docking
		auto & io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigDockingWithShift = true;

		// Table styling
		ImGui::GetStyle().CellPadding.x = 6;


		state.has_program_errors = not game.assets.program_errors.empty() or not editor_assets.program_errors.empty();
		// Game should not render if any program failed to compile
		if (state.has_program_errors)
			state.should_game_render = false;
	}

	void add_window(unique_one<WindowBase> && window)
	{
		window->init(*this);
		windows.emplace_back(move(window));
	}

	void remove_window()
	{
		//!!!????????
	}

	void workspaces()
	{
		using namespace ImGui;

		// see imgui_demo.cpp ShowExampleAppDockSpace function
		auto * viewport = GetMainViewport();
		SetNextWindowPos(viewport->WorkPos);
		SetNextWindowSize(viewport->WorkSize);
		SetNextWindowViewport(viewport->ID);
		PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
		PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2, 2));
		Begin(
			"MainWindow", nullptr,
			ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse
			| ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus
			| ImGuiWindowFlags_NoNavFocus
		);
		PopStyleVar(2);


		BeginTabBar("Workspaces");

		struct Workspace
		{
			const char * name;
			ImGuiID id;

			explicit Workspace(const char* name):
				name(name),
				id(GetID(name))
			{}
		};

		static array workspaces = {
			Workspace("Workspace 1"),
			Workspace("Workspace 2"),
			Workspace("Workspace 3"),
		};

		for (auto & workspace: workspaces)
		{
			if (BeginTabItem(workspace.name))
				DockSpace(workspace.id), EndTabItem();
			else
				DockSpace(workspace.id, {0, 0}, ImGuiDockNodeFlags_KeepAliveOnly);
		}

		EndTabBar();

		End();
	}

	void update_windows(Render::FrameInfo const & frame_info, f64 game_update_in_seconds, f64 game_render_in_seconds)
	{
		state.frame_info = frame_info;
		state.game_update_in_seconds = game_update_in_seconds;
		state.game_render_in_seconds = game_render_in_seconds;

		state.has_program_errors = not editor_assets.program_errors.empty() or not game.assets.program_errors.empty();

		workspaces();

		using namespace ImGui;

		for (auto & window: windows)
		{
			auto pushed_color_count = 0;
			if (window->state == WindowBase::State::ERROR)
			{
				PushStyleColor(ImGuiCol_Tab, {0.8, 0, 0, 1});
				PushStyleColor(ImGuiCol_TabActive, {1, 0, 0, 1});
				PushStyleColor(ImGuiCol_TabUnfocused, {0.8, 0, 0, 1});
				PushStyleColor(ImGuiCol_TabUnfocusedActive, {0.8, 0, 0, 1});
				PushStyleColor(ImGuiCol_TabHovered, {1, 0, 0, 1});
				pushed_color_count = 5;
			}

			auto is_visible = Begin(window->get_name(), nullptr, ImGuiWindowFlags_NoCollapse);

			PopStyleColor(pushed_color_count);

			if (is_visible)
				window->update(*this);

			End();
		}

//		ImGui::ShowDemoWindow();
	}

	void render_windows() const
	{
		for (auto & window: windows)
			window->render(*this);
	}
};
}