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
		u64 base_color_texture_handle{0};
		f32x4 base_color_factor;

		u64 metallic_roughness_texture_handle{0};
		f32x2 metallic_roughness_factor;

		u64 emissive_texture_handle{0};
		f32x3 emissive_factor;

		u64 occlusion_texture_handle{0};
		u64 normal_texture_handle{0};

		void set_uniforms() final
		{
			using namespace GL;

			glUniformHandleui64ARB(0, base_color_texture_handle);
			glUniform4fv(1, 1, begin(base_color_factor));

			glUniformHandleui64ARB(2, metallic_roughness_texture_handle);
			glUniformHandleui64ARB(3, emissive_texture_handle);
			glUniformHandleui64ARB(4, occlusion_texture_handle);
			glUniformHandleui64ARB(5, normal_texture_handle);
		}
	};
}