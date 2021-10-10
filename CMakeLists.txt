cmake_minimum_required(VERSION 3.20)

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

