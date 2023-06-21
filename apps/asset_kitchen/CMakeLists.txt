cmake_minimum_required(VERSION 3.20)

project(AssetKitchen)

string(APPEND CMAKE_RUNTIME_OUTPUT_DIRECTORY "/AssetKitchen")

set(APPS ${APPS} AssetKitchen PARENT_SCOPE)
add_executable(AssetKitchen main.cpp)
target_sources(AssetKitchen PRIVATE
    recipes/attrib_layout.cpp
    )
target_link_libraries(AssetKitchen PRIVATE
    AssetRecipes
    )
