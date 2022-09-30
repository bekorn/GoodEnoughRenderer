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
add_library(core STATIC)
target_precompile_headers(core PUBLIC Lib/core/.pch.hpp)
target_sources(core PRIVATE Lib/core/core.cpp)
target_link_libraries(core PUBLIC
    glm::glm
    fmt::fmt-header-only)
target_include_directories(core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})

# file_amangement
add_library(file_management STATIC)
target_precompile_headers(file_management PUBLIC Lib/file_management/.pch.hpp)
target_sources(file_management PRIVATE Lib/file_management/core.cpp)
target_link_libraries(file_management PRIVATE
    core)
target_link_libraries(file_management PUBLIC
    rapidjson)
target_include_directories(file_management PUBLIC
    ${STB_INCLUDE_DIRS})


# debug optimizations
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set_target_properties(core file_management PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
endif()


# rest
add_library(Lib INTERFACE)
target_include_directories(Lib INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR})

target_precompile_headers(Lib INTERFACE
    Lib/glfw/.pch.hpp
    Lib/imgui/.pch.hpp
    Lib/opengl/.pch.hpp)

target_link_libraries(Lib INTERFACE
    core
    file_management
    glbinding::glbinding glbinding::glbinding-aux
    glfw
    imgui::imgui)
