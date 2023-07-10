#include "compressonator_facade.hpp"
#include "compressonator.h"
#include "core/intrinsics.hpp"

ByteBuffer BC7(File::Image const & image)
{
	/// src image
	CMP_Texture src = {};
	src.dwSize = sizeof(CMP_Texture);
	src.dwWidth = image.dimensions.x;
	src.dwHeight = image.dimensions.y;
	src.dwPitch = image.dimensions.x;
	src.format = [&](){
		if (image.channels == 4) return CMP_FORMAT_RGBA_8888;
		if (image.channels == 3) return CMP_FORMAT_RGB_888;
		assert_failure("BC7 support RGB or RGBA images");
	}();
	src.dwDataSize = image.buffer.size;
    src.pData = image.buffer.data_as<CMP_BYTE>();

	/// dst image
	CMP_Texture dst = {};
	dst.dwSize = sizeof(CMP_Texture);
	dst.dwWidth = src.dwWidth;
	dst.dwHeight = src.dwHeight;
	dst.dwPitch = src.dwPitch;
	dst.format = CMP_FORMAT_BC7;

	ByteBuffer buffer(CMP_CalculateBufferSize(&dst));
	dst.dwDataSize = buffer.size;
	dst.pData = buffer.data_as<CMP_BYTE>();

	/// convert
	CMP_CompressOptions options = {};
	options.dwSize = sizeof(CMP_CompressOptions);
	options.nEncodeWith = CMP_GPU_DXC; // !!! has no effect :(
	options.fquality = 0.05;
	options.dwnumThreads = 0;  // Uses auto, else set number of threads from 1..127 max

	auto const callback = [](CMP_FLOAT progress, CMP_DWORD_PTR pUser1, CMP_DWORD_PTR pUser2)
	{
		fmt::print("\rCompressed {:.0f}%", progress);
		return false; // do not abort ongoing texture conversion
	};

	auto status = CMP_ConvertTexture(&src, &dst, &options, callback);
	fmt::print("{}", '\r');
	if (status != CMP_OK)
	{
		fmt::print("Compression returned an error {}\n", status);
		return {};
	}

	return move(buffer);
}
