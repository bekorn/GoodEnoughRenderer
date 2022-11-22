#pragma once

#include "core.hpp"

namespace GL
{
	std::string const GLSL_VERSION_MACRO =
		"#version " + std::to_string(VERSION_MAJOR) + std::to_string(VERSION_MINOR) + "0\n";

	std::string const GLSL_COMMON_EXTENSIONS = R"___(
	#extension GL_ARB_bindless_texture : enable
	#extension GL_ARB_gpu_shader_int64 : enable
	layout(bindless_sampler) uniform;
	layout(bindless_image) uniform;
	)___";
}
