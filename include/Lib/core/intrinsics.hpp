#pragma once

#include "meta.hpp"

[[noreturn]]
inline void unreachable() noexcept
{
#ifdef Compiler_MSVC
	// https://docs.microsoft.com/en-us/cpp/intrinsics/assume
	__assume(false);
#else
	// https://clang.llvm.org/docs/LanguageExtensions.html#builtin-unreachable
	__builtin_unreachable();
#endif
}


/// Custom assert and shortcuts
// available here https://godbolt.org/z/Gr95Eecb5
// on release: has a very small overhead (2-4 instructions) for the unreachable cases, rest is eliminated completely
// on debug: with addition to all the strings generated, overhead is ~15 instructions for MSVC, ~5 instructions for GCC

// some library includes c assert and I can't figured where, nonetheless I want to use my assert so it should be gone
#ifdef assert
#undef assert
#endif

// actual mechanism to assert
[[noreturn]] inline void assert_failure(
	const char * message, std::source_location const caller = std::source_location::current()
)
{
	fmt::print(
		stderr, "Assertion failed! {}\n{}:{}:{} ({})\n",
		message, caller.file_name(), caller.line(), caller.column(), caller.function_name()
	);
	// see https://en.cppreference.com/w/cpp/error/terminate for comparison
	std::abort();
}

inline void assert(
	bool condition, const char * message = "", std::source_location const caller = std::source_location::current()
)
{
	if (not condition)
		assert_failure(message, caller);
}

// macro overrides
#if DEBUG
	#define assert_failure(message) assert_failure(message)
	#define assert(...) assert(__VA_ARGS__)
#else
	#define assert_failure(message) unreachable()
	#define assert(...)
#endif

// shortcuts
#define assert_enum_out_of_range() assert_failure("Enum out of range")
#define assert_case_not_handled() assert_failure("Case not handled")
