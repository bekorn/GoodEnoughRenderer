cmake_minimum_required(VERSION 3.20)

project(Laboratory)

string(APPEND CMAKE_RUNTIME_OUTPUT_DIRECTORY "/Laboratory")

set(APPS ${APPS} Laboratory PARENT_SCOPE)
add_executable(Laboratory main.cpp game.cpp editor_windows.cpp)
target_precompile_headers(Laboratory PUBLIC .pchpp)
target_link_libraries(Laboratory PUBLIC ${LIBS})

# provide default imgui layout
add_custom_command(
    TARGET Laboratory POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui.ini
    $<TARGET_FILE_DIR:Laboratory>
)
