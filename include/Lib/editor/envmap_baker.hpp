#pragma once

#include "core.hpp"

namespace Editor
{
// TODO(bekorn): assets needed for envmap generation should be loaded when needed,
//  otherwise this will constantly increase the startup time while being rarely used
struct EnvmapBakerWindow : Editor::WindowBase
{
	const char * get_name() override
	{ return "Envmap Baker"; }

	Name selected_name;
	bool is_texture_changed = false;
	f32x2 texture_size;
	bool should_generate_envmap = false;
	bool should_generate_brdf_lut = false;

	Name cubemap_name;
	void update(Editor::Context & ctx) override;
	void render(Editor::Context const & ctx) override;
	void generate_brdf_lut(const Editor::Context & ctx) const;
	void generate_envmap(Editor::Context const & ctx) const;
};
}