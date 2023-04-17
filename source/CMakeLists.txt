cmake_minimum_required(VERSION 3.20)

add_executable(App main.cpp game.cpp editor_windows.cpp)
target_precompile_headers(App PUBLIC .pchpp)
target_link_libraries(App PRIVATE Lib)

# provide default imgui layout
add_custom_command(
    TARGET App POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui.ini
    $<TARGET_FILE_DIR:App>
)
