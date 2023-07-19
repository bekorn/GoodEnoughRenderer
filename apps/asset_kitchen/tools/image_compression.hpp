#pragma once

#include <core/core.hpp>
#include <file_io/core.hpp>

namespace ImgComp
{
bool Initialize();
ByteBuffer BC7(File::Image const & image);
}

