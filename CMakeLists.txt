cmake_minimum_required(VERSION 3.21)

project(GoodEnoughRenderer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}-Binaries")

# common flags
set(CMAKE_C_STANDARD_LIBRARIES "")
set(CMAKE_CXX_STANDARD_LIBRARIES "")
if (MSVC)
    # checks if a switch statement without a default case handles every enum value
    # https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-4-c4062?view=msvc-170
    add_compile_options(/w14062)
    # better optimizations and make glm use SIMD (https://github.com/g-truc/glm/blob/master/manual.md#section2_11)
    add_compile_options(/arch:AVX2)
endif()

# RelWithDebInfo flags
if (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    message("Default\t CMAKE_CXX_FLAGS_RELWITHDEBINFO = ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")

    # Enable debugging for specific parts
    add_compile_definitions(
        _DEBUG_CORE
        _DEBUG_GL
    )

    if (MSVC)
        # see https://ledas.com/post/861-fast-debug-in-visual-c/
        set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/Zi /Ob1 /Od /DNDEBUG")
    else()
    endif()

    message("Custom\t CMAKE_CXX_FLAGS_RELWITHDEBINFO = ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
endif()

# Release flags
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    message("Default\t CMAKE_CXX_FLAGS_RELEASE = ${CMAKE_CXX_FLAGS_RELEASE}")

    if (MSVC)
        set(CMAKE_CXX_FLAGS_RELEASE "/Zi /O2 /Ob3 /DNDEBUG")
    else()
    endif()

    message("Custom\t CMAKE_CXX_FLAGS_RELEASE = ${CMAKE_CXX_FLAGS_RELEASE}")
endif ()


add_subdirectory(libs)
add_subdirectory(apps/laboratory)
add_subdirectory(apps/asset_kitchen)

list(APPEND AllTargets ${APPS} ${LIBS})
