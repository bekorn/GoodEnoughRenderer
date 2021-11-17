#pragma once

#include "Lib/opengl/.hpp"

struct IMaterial
{
	// the functionality should be compatible with any shader,
	// but the data should be tightly coupled with individual shaders

	// TODO(bekorn): maybe generate the concrete Material structs with another program then compile this program.
	//  implication: no dynamic materials! for rendering purposes, ok. However, for an editor, this can be bad.
	//  maybe: the material class can be C-simple that it can be loaded dynamically on runtime (via a dll).
	//   so jit compile the generated Material into a dll and watch for changed dlls on this side.

	virtual void set_uniforms() = 0;
};

// for now, here is an example concrete Material
struct Material_gltf_pbrMetallicRoughness : IMaterial
{
	u32 base_color_texture;
	f32x4 base_color_factor;

	u32 metallic_roughness_texture;
	f32x2 metallic_roughness_factor;

	u32 emissive_texture;
	f32x3 emissive_factor;

	u32 occlusion_texture;
	u32 normal_texture;

	void set_uniforms() override
	{
		using namespace GL;

		glActiveTexture(GL_TEXTURE0 + 0);
		glBindTexture(GL_TEXTURE_2D, base_color_texture);
		glUniform4fv(0, 1, begin(base_color_factor));

		glActiveTexture(GL_TEXTURE0 + 1);
		glBindTexture(GL_TEXTURE_2D, metallic_roughness_texture);

		glActiveTexture(GL_TEXTURE0 + 2);
		glBindTexture(GL_TEXTURE_2D, emissive_texture);

		glActiveTexture(GL_TEXTURE0 + 3);
		glBindTexture(GL_TEXTURE_2D, occlusion_texture);

		glActiveTexture(GL_TEXTURE0 + 4);
		glBindTexture(GL_TEXTURE_2D, normal_texture);
	}
};