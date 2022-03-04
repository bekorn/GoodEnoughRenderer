cmake_minimum_required(VERSION 3.21)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/build/vcpkg/scripts/buildsystems/vcpkg.cmake CACHE STRING "Vcpkg toolchain file")
# set this ON to update packages
set(VCPKG_MANIFEST_INSTALL OFF)

project(GoodEnoughRenderer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}-Binaries")


add_subdirectory(include)

add_subdirectory(source)
target_link_libraries(App
    PUBLIC Lib)


# Debug optimizations
# see section Version of CRT on https://ledas.com/post/861-fast-debug-in-visual-c/
set_target_properties(App PROPERTIES
    MSVC_RUNTIME_LIBRARY "MultiThreadedDLL"
    )

# Not sure if this has any affects (can't see the generated assembly currently).
# However, this flag is needed to generate same assembly from std::algorithms and bare for loops.
list(APPEND CMAKE_CXX_FLAGS_DEBUG " /O1")