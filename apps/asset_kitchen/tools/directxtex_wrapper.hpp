#pragma once

namespace Wrapper
{
struct Buffer
{
	void * ptr;
	size_t size;
	size_t width;
};

bool Initialize();

Buffer BC7(Buffer const & src_img);

}