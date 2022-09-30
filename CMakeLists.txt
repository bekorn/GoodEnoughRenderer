cmake_minimum_required(VERSION 3.21)

project(GoodEnoughRenderer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}-Binaries")


add_subdirectory(include)

add_subdirectory(source)
target_link_libraries(App
    PUBLIC Lib)


# Debug optimizations
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    # see section Version of CRT on https://ledas.com/post/861-fast-debug-in-visual-c/
    set_target_properties(App PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")

    # Not sure if this has any affects (can't see the generated assembly currently).
    # However, this flag is needed to generate same assembly from std::algorithms and bare for loops.
    list(APPEND CMAKE_CXX_FLAGS_DEBUG " /O1")

    # Enable debugging for specific parts
    target_compile_definitions(App PUBLIC _DEBUG_CORE)
    target_compile_definitions(App PUBLIC _DEBUG_GL)
endif ()
