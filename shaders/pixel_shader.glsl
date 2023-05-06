#version 460

layout (binding = 0) uniform sampler2D current_generation;

in vec2 outUV;

out vec4 fragColor;

void main() {
    // only the red color is important
    // we also set alpha to 1 to prevent problems
    fragColor = vec4(texture(current_generation, outUV).r, 0.1, 0.3, 1);
}