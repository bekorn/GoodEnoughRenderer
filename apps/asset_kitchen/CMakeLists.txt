cmake_minimum_required(VERSION 3.20)

project(AssetKitchen)

string(APPEND CMAKE_RUNTIME_OUTPUT_DIRECTORY "/AssetKitchen")

set(APPS ${APPS} AssetKitchen PARENT_SCOPE)
add_executable(AssetKitchen main.cpp)
target_precompile_headers(AssetKitchen PRIVATE .pchpp)
target_sources(AssetKitchen PRIVATE
    tools/image_compression.cpp
    tools/directxtex_wrapper.cpp
    recipes/gltf.cpp
    chef.cpp
    )
target_link_libraries(AssetKitchen PRIVATE
    AssetRecipes
    )

find_package(directxtex CONFIG REQUIRED)
target_link_libraries(AssetKitchen PRIVATE Microsoft::DirectXTex "Ole32.lib")