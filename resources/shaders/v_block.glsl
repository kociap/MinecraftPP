#version 460 core
layout (location = 0) in vec3 vtx;

void main() {
    gl_Position = vec4(vtx, 1.0f);
}
