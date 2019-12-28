cmake_minimum_required(VERSION 3.15)
project(MinecraftPP)

set(CMAKE_CXX_STANDARD 20)

include_directories(include)
include_directories(include/imgui)
include_directories(include/imgui/imgui_impl)
include_directories(/usr/include/freetype2)

add_executable(MinecraftPP
        include/util.hpp
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

target_link_libraries(MinecraftPP glfw fmt assimp dl GL pthread freetype glad)