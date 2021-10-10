#pragma once

#if defined(_DEBUG)
constexpr bool DEBUG = true;
#else
constexpr bool DEBUG = false;
#endif

// TODO(bekorn): not sure if this will be handy or useless
// https://blog.kowalczyk.info/article/j/guide-to-predefined-macros-in-c-compilers-gcc-clang-msvc-etc..html
namespace Meta::Platform
{
#if defined(_WIN64)
	constexpr bool WIN64 = true;
#endif
}

namespace Meta::Compiler
{
#if defined(_MSC_VER)
	constexpr bool MSVC = true;
#endif
}
