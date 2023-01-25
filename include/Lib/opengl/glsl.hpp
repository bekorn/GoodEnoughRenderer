#pragma once

#include "core.hpp"

namespace GL
{
std::string const GLSL_VERSION_MACRO = fmt::format("#version {}{}0\n", VERSION_MAJOR, VERSION_MINOR);

std::string const GLSL_COMMON_EXTENSIONS = R"___(
#extension GL_ARB_bindless_texture : enable
#extension GL_ARB_gpu_shader_int64 : enable
layout(bindless_sampler) uniform;
layout(bindless_image) uniform;
)___";
}
