cmake_minimum_required(VERSION 3.20)

# externals
find_package(glbinding CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(RapidJSON CONFIG REQUIRED)
find_path(STB_INCLUDE_DIRS "stb.h")
find_package(fmt CONFIG REQUIRED)


# asset_kitchen
list(APPEND LIBS asset_kitchen)
add_library(asset_kitchen STATIC)
target_sources(asset_kitchen PRIVATE
    Lib/asset_kitchen/glsl/vertex_layout/.cpp
    Lib/asset_kitchen/glsl/program/.cpp
    Lib/asset_kitchen/glsl/uniform_block/.cpp
    Lib/asset_kitchen/gltf/.cpp
    Lib/asset_kitchen/texture/.cpp
    Lib/asset_kitchen/cubemap/.cpp
    Lib/asset_kitchen/envmap/.cpp
    Lib/asset_kitchen/.cpp
)
target_include_directories(asset_kitchen PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(asset_kitchen PUBLIC
    Core
    FileIO
    OpenGL)
target_include_directories(asset_kitchen PUBLIC
    ${STB_INCLUDE_DIRS})

# editor
list(APPEND LIBS editor)
add_library(editor STATIC)
target_sources(editor PRIVATE
    Lib/editor/utils.cpp
    Lib/editor/core_windows.cpp
    Lib/editor/envmap_baker.cpp
)
target_include_directories(editor PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(editor PRIVATE
    Core
    OpenGL
    asset_kitchen)
target_precompile_headers(editor PUBLIC
    Lib/editor/.pchpp
    Lib/glfw/.pchpp
    Lib/imgui/.pchpp)
target_link_libraries(editor PUBLIC
    glfw
    imgui::imgui)

# export libs to parent scope
set(LIBS ${LIBS} PARENT_SCOPE)

add_library(Lib INTERFACE)
target_include_directories(Lib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Lib INTERFACE ${LIBS})
