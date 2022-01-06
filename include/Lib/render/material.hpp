#pragma once

#include "Lib/opengl/core.hpp"

namespace Render
{
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
	struct Material_gltf_pbrMetallicRoughness final : IMaterial
	{
		u32 base_color_texture;
		f32x4 base_color_factor;

		u32 metallic_roughness_texture;
		f32x2 metallic_roughness_factor;

		u32 emissive_texture;
		f32x3 emissive_factor;

		u32 occlusion_texture;
		u32 normal_texture;

		void set_uniforms() final
		{
			using namespace GL;

			glBindTextureUnit(0, base_color_texture);
			glUniform4fv(0, 1, begin(base_color_factor));

			glBindTextureUnit(1, metallic_roughness_texture);

			glBindTextureUnit(2, emissive_texture);

			glBindTextureUnit(3, occlusion_texture);

			glBindTextureUnit(4, normal_texture);
		}
	};
}