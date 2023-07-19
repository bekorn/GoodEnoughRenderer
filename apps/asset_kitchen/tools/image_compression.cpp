#include "image_compression.hpp"

#include "directxtex_wrapper.hpp"

#include <core/intrinsics.hpp>

namespace ImgComp
{
bool Initialize()
{ return Wrapper::Initialize(); }

ByteBuffer BC7(File::Image const & image)
{
	assert(image.channels == 4 and image.is_format_f32 == false, "Unsupported image format for BC7");

	auto buffer = Wrapper::BC7({
		.ptr = image.buffer.data_as<void>(),
		.size = image.buffer.size,
		.width = size_t(image.dimensions.y),
	});
	return ByteBuffer(move(buffer.ptr), buffer.size);
}
}


