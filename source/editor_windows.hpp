#pragma once

#include "Lib/editor/core.hpp"
#include "Lib/editor/core_windows.hpp"

struct GameSettingsWindow : Editor::WindowBase
{
	const char * get_name() override
	{ return "Game Settings"; }

	void update(Editor::Context & ctx) override;
};

struct MetricsWindow : Editor::MetricsWindow
{
	void update(Editor::Context & ctx) override;
};

struct MaterialWindow : Editor::WindowBase
{
	const char * get_name() override
	{ return "Material Window"; }

	void update(Editor::Context & ctx) override;
};