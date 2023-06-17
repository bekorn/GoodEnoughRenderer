cmake_minimum_required(VERSION 3.20)

# Third party libs
find_package(glbinding CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(RapidJSON CONFIG REQUIRED)
find_path(STB_INCLUDE_DIRS "stb.h")
find_package(fmt CONFIG REQUIRED)


# Core
list(APPEND LIBS Core)
add_library(Core STATIC)
target_include_directories(Core PUBLIC core/)
target_precompile_headers(Core PUBLIC core/core/.pchpp)
target_sources(Core PRIVATE core/core/core.cpp)

target_link_libraries(Core
    PUBLIC
    glm::glm
    fmt::fmt-header-only)


# FileIO
list(APPEND LIBS FileIO)
add_library(FileIO STATIC)
target_include_directories(FileIO PUBLIC file_io/)
target_precompile_headers(FileIO PUBLIC file_io/file_io/.pchpp)
target_sources(FileIO PRIVATE
    file_io/file_io/core.cpp)
target_link_libraries(FileIO PUBLIC
    Core)

target_link_libraries(FileIO PUBLIC
    rapidjson)
target_include_directories(FileIO PRIVATE
    ${STB_INCLUDE_DIRS})


# OpenGL
list(APPEND LIBS OpenGL)
add_library(OpenGL STATIC)
target_include_directories(OpenGL PUBLIC opengl/)
target_precompile_headers(OpenGL PUBLIC opengl/opengl/.pchpp)
target_sources(OpenGL PRIVATE
    opengl/opengl/utils.cpp)
target_include_directories(OpenGL PUBLIC ../include/Lib) # !!! this is here just for the geometry.hpp
target_link_libraries(OpenGL PUBLIC
    Core)

target_link_libraries(OpenGL PUBLIC
    glbinding::glbinding
    glbinding::glbinding-aux)



# export libs to parent scope
set(LIBS ${LIBS} PARENT_SCOPE)


