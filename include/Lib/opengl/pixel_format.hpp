#pragma once

#include "core.hpp"

namespace GL
{
// combines color space (linear/srgb) and channel format (u8/f32). (might be a problem later, might be not)
// TODO(bekorn): LINEAR_F32 takes too much space, LINEAR_F16 might be sufficient for many cases
// TODO(bekorn): SRGB color space can be eliminated completely by transforming the texture before uploading to GPU.
//  currently SRGB affects the internal format, if transformed beforehand, only glsl side would care for SRGBness
enum struct COLOR_SPACE
{
	LINEAR_U8, LINEAR_F32, SRGB_U8
};

}