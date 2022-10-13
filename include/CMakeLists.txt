cmake_minimum_required(VERSION 3.20)

# externals
find_package(glbinding CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(RapidJSON CONFIG REQUIRED)
find_path(STB_INCLUDE_DIRS "stb.h")
find_package(fmt CONFIG REQUIRED)


# core
list(APPEND LIBS core)
add_library(core STATIC)
target_precompile_headers(core PUBLIC Lib/core/.pch.hpp)
target_sources(core PRIVATE Lib/core/core.cpp)
target_link_libraries(core PUBLIC
    glm::glm
    fmt::fmt-header-only)
target_include_directories(core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})

# file_amangement
list(APPEND LIBS file_management)
add_library(file_management STATIC)
target_precompile_headers(file_management PUBLIC Lib/file_management/.pch.hpp)
target_sources(file_management PRIVATE
    Lib/file_management/core.cpp)
target_link_libraries(file_management PRIVATE
    core)
target_link_libraries(file_management PUBLIC
    rapidjson)
target_include_directories(file_management PRIVATE
    ${STB_INCLUDE_DIRS})

# opengl
list(APPEND LIBS opengl)
add_library(opengl STATIC)
target_precompile_headers(opengl PUBLIC Lib/opengl/.pch.hpp)
target_sources(opengl PRIVATE
    Lib/opengl/utils.cpp)
target_link_libraries(opengl PRIVATE
    core)
target_link_libraries(opengl PUBLIC
    glbinding::glbinding
    glbinding::glbinding-aux)

# asset_kitchen
list(APPEND LIBS asset_kitchen)
add_library(asset_kitchen STATIC)
target_sources(asset_kitchen PRIVATE
    Lib/asset_kitchen/glsl/program/.cpp
    Lib/asset_kitchen/glsl/uniform_block/.cpp
    Lib/asset_kitchen/gltf/.cpp
    Lib/asset_kitchen/.cpp
)
target_link_libraries(asset_kitchen PRIVATE
    core
    file_management
    opengl)
target_include_directories(asset_kitchen PUBLIC
    ${STB_INCLUDE_DIRS})


# debug optimizations
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set_target_properties(${LIBS} PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
endif()


# rest
add_library(Lib INTERFACE)
target_include_directories(Lib INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR})

target_precompile_headers(Lib INTERFACE
    Lib/glfw/.pch.hpp
    Lib/imgui/.pch.hpp)

target_link_libraries(Lib INTERFACE
    ${LIBS}
    glfw
    imgui::imgui)
