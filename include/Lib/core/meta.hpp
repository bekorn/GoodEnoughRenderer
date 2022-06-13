#pragma once

#if defined(_DEBUG_CORE)
constexpr bool DEBUG = true;
#else
constexpr bool DEBUG = false;
#endif

#if defined(_DEBUG_GL)
constexpr bool DEBUG_GL = true;
#else
constexpr bool DEBUG_GL = false;
#endif

// TODO(bekorn): not sure if this will be handy or useless
// https://blog.kowalczyk.info/article/j/guide-to-predefined-macros-in-c-compilers-gcc-clang-msvc-etc..html
namespace Meta
{
	enum class Platform
	{
		WIN64
	}
	constexpr Platform =
#if defined(_WIN64)
	#define Platform_WIN64
	Platform::WIN64
#endif
	;

	enum class Compiler
	{
		MSVC
	}
	constexpr Compiler =
#if defined(_MSC_VER)
		#define Compiler_MSVC
		Compiler::MSVC
#endif
	;
}
