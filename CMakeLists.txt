cmake_minimum_required(VERSION 3.21)

project(GoodEnoughRenderer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}-Binaries")

# common flags
if (MSVC)
    # checks if a switch statement without a default case handles every enum value
    # https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-4-c4062?view=msvc-170
    add_compile_options(/w14062)
    # better optimizations and make glm use SIMD (https://github.com/g-truc/glm/blob/master/manual.md#section2_11)
    add_compile_options(/arch:AVX2)
else()
#    add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

# Debug flags
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("Default\t CMAKE_CXX_FLAGS_DEBUG = ${CMAKE_CXX_FLAGS_DEBUG}")

    # Enable debugging for specific parts
    add_compile_definitions(
        _DEBUG_CORE
        _DEBUG_GL
    )

    if (MSVC)
        # see https://ledas.com/post/861-fast-debug-in-visual-c/
        set(CMAKE_CXX_FLAGS_DEBUG "/Zi /Ob1 /Od")
    else()
    endif()

    message("Custom\t CMAKE_CXX_FLAGS_DEBUG = ${CMAKE_CXX_FLAGS_DEBUG}")
endif()

# RelWithDebInfo flags
if(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    message("Default\t CMAKE_CXX_FLAGS_RELWITHDEBINFO = ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")

    if (MSVC)
        set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/Zi /O2 /Ob3 /DNDEBUG")
    else()
    endif()

    message("Custom\t CMAKE_CXX_FLAGS_RELWITHDEBINFO = ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
endif ()


add_subdirectory(libs)
add_subdirectory(apps/laboratory)
add_subdirectory(apps/asset_kitchen)

list(APPEND AllTargets ${APPS} ${LIBS})

# this only works like this, /MD flag prints wanings on builds and
# set_property(GLOBAL PROPERTY MSVC_RUNTIME_LIBRARY MultiThreadedDLL) has no effect
# TODO(bekorn): This is disabled because DirectxTex does not compile otherwise. Enable it
#if (MSVC AND CMAKE_BUILD_TYPE STREQUAL "Debug")
#    foreach (Target ${AllTargets})
#        set_property(TARGET ${Target} PROPERTY MSVC_RUNTIME_LIBRARY MultiThreadedDLL)
#    endforeach ()
#endif ()
