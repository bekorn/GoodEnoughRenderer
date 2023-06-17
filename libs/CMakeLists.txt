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


# export libs to parent scope
set(LIBS ${LIBS} PARENT_SCOPE)


