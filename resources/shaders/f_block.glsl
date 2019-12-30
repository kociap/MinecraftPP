#version 460 core

uniform sampler2D sampler;

in vec2 tx_coords;
out vec3 color;

void main() {
    color = texture(sampler, tx_coords).rgb;
}