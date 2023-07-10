cmake_minimum_required(VERSION 3.20)

project(AssetKitchen)

string(APPEND CMAKE_RUNTIME_OUTPUT_DIRECTORY "/AssetKitchen")

set(APPS ${APPS} AssetKitchen PARENT_SCOPE)
add_executable(AssetKitchen main.cpp)
target_precompile_headers(AssetKitchen PRIVATE .pchpp)
target_sources(AssetKitchen PRIVATE
    tools/compressonator_facade.cpp
    recipes/gltf.cpp
    chef.cpp
    )
target_link_libraries(AssetKitchen PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/Compressonator_MD.lib
    AssetRecipes
    )
