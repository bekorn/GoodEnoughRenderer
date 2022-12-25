#pragma once

#include "core.hpp"

namespace GL
{
// combines u8/f16 and linear/srgb (might be a problem later, might be not)
enum struct COLOR_SPACE
{
	LINEAR_BYTE, LINEAR_FLOAT, SRGB
};

}