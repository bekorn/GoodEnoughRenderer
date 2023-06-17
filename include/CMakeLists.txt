cmake_minimum_required(VERSION 3.20)

# externals
find_package(glbinding CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(RapidJSON CONFIG REQUIRED)
find_path(STB_INCLUDE_DIRS "stb.h")
find_package(fmt CONFIG REQUIRED)


# export libs to parent scope
set(LIBS ${LIBS} PARENT_SCOPE)

add_library(Lib INTERFACE)
target_include_directories(Lib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Lib INTERFACE ${LIBS})
