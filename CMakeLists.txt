cmake_minimum_required(VERSION 3.15)
project(MinecraftPP)

set(CMAKE_CXX_STANDARD 20)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(include/glfw)
add_subdirectory(include/fmt)
add_subdirectory(include/glm)

include_directories(include)
include_directories(include/imgui)
include_directories(include/glad)
include_directories(include/glm)
include_directories(include/stb)
include_directories(include/glfw/include/GLFW)
include_directories(include/imgui/imgui_impl)
include_directories(/usr/include/freetype2)

execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    RESULT_VARIABLE UPDATE_SUBMODULES_RESULT)

add_executable(MinecraftPP
    include/util.hpp
    include/glad/glad.h
    include/glad/glad.c
    include/imgui/imconfig.h
    include/imgui/imgui.cpp
    include/imgui/imgui.h
    include/imgui/imgui_demo.cpp
    include/imgui/imgui_draw.cpp
    include/imgui/imgui_internal.h
    include/imgui/imgui_widgets.cpp
    include/imgui/imstb_rectpack.h
    include/imgui/imstb_textedit.h
    include/imgui/imstb_truetype.h
    include/imgui/imgui_impl/imgui_impl_glfw.cpp
    include/imgui/imgui_impl/imgui_impl_glfw.h
    include/imgui/imgui_impl/imgui_impl_opengl3.cpp
    include/imgui/imgui_impl/imgui_impl_opengl3.h
    src/main.cpp)

set_target_properties(glfw fmt 
    PROPERTIES
    COMPILE_OPTIONS -Wno-everything
)

target_link_libraries(MinecraftPP glfw fmt)
if(NOT WIN32)
    target_link_libraries(MinecraftPP dl GL pthread)
endif()