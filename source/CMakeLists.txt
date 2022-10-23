cmake_minimum_required(VERSION 3.20)

add_executable(App main.cpp editor.cpp game.cpp)
target_precompile_headers(App PUBLIC .pchpp)
target_link_libraries(App PRIVATE Lib)
