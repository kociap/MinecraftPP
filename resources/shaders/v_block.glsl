#version 460 core
layout (location = 0) in vec3 ivtx;
layout (location = 1) in vec2 itx_coords;
layout (location = 2) in vec3 offsets;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

out vec2 tx_coords;

void main() {
    tx_coords = itx_coords;
    gl_Position = projection * view * model * vec4(ivtx.xyz + offsets.xyz, 1.0f);
}
