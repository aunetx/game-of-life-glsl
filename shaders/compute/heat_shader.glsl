#version 460 core

layout (local_size_x = 8, local_size_y = 8) in;

layout (r32f, binding = 0) uniform image2D current_generation;
layout (r32f, binding = 1) uniform image2D next_generation;
layout (rgba32f, binding = 2) uniform image2D material;

uniform vec4 mouse;
uniform float radius_mouse = 5;
uniform float time;

uniform float dl; // dx and dy
uniform float dt;

ivec2 size = imageSize(current_generation);

// get the temperature at a coordinate, the red value * 1000
float T(int x, int y) {
    x %= size.x;
    y %= size.y;

    vec4 color = imageLoad(current_generation, ivec2(x, y));

    return color.r * 1000;
}

// compute the laplacian of T, d²T/dx² + d²T/dy²
float laplacian(int x, int y) {
    return (T(x + 1, y) + T(x - 1, y) + T(x, y + 1) + T(x, y - 1) - 4 * T(x, y)) / (dl * dl);
}

// compute the first derivative, dT/dx
float d1(int x, int y) {
    return T(x + 1, y) - T(x, y) / dl;
}

// compute the second derivative, dT/dy
float d2(int x, int y) {
    return T(x, y + 1) - T(x, y) / dl;
}

// the diffusion coefficient
float D(int x, int y) {
    x %= size.x;
    y %= size.y;

    vec4 color = imageLoad(material, ivec2(x, y));

    return color.r / 40;
}

// the speed of the wind for both axis
vec2 v(int x, int y) {
    x %= size.x;
    y %= size.y;

    vec4 color = imageLoad(material, ivec2(x, y));

    return (color.gb - vec2(0.5)) * 0.01;
}

// pause the game if the left button is clicked, or
bool is_paused() {
    return mouse.z == 1;
}

bool is_clicking_on_me(ivec2 pix_coord) {
    // the distance from the mouse to the pixel
    float dist_mouse = distance(mouse.xy, vec2(pix_coord) / size.x);

    return dist_mouse < radius_mouse / size.x && is_paused();
}

float diffusion(int x, int y) {
    return D(x, y) * laplacian(x, y);
}

float advection(int x, int y) {
    return -d1(x, y) * v(x, y).x - d2(x, y) * v(x, y).y;
}

void main() {
    ivec2 pixel_coord = ivec2(gl_GlobalInvocationID.xy);
    int x = pixel_coord.x;
    int y = pixel_coord.y;

    // get previous temperature
    float T_t = T(x, y);

    // increase temperature on click
    if (is_clicking_on_me(pixel_coord)) {
        T_t += 7;
    } else if (!is_paused()) {
        T_t += (advection(x, y) + diffusion(x, y)) * dt;
    }

    vec4 px = vec4(1.);
    px.r = T_t / 1000;

    imageStore(next_generation, pixel_coord, px);
}
