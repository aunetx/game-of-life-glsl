#version 460 core

layout(local_size_x = 8, local_size_y = 8) in;

layout(r8, binding = 0) uniform readonly image2D source;
layout(r8, binding = 1) uniform writeonly image2D target;

void main() {
ivec2 pixel_coord = ivec2(gl_GlobalInvocationID.xy);

vec4 px = imageLoad(source, pixel_coord);

imageStore(target, pixel_coord, px);
}
