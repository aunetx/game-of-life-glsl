#version 460 core

layout(local_size_x = 8, local_size_y = 8) in;

layout(r8, binding = 0) uniform image2D current_generation;
layout(r8, binding = 1) uniform image2D next_generation;

uniform vec4 mouse;
uniform float radius_mouse = 5;

ivec2 size = imageSize(current_generation);

// get the corresponding pixel state on the game board
// by wrapping around the edges; a black pixel is false
bool get_pix_state(ivec2 pix_coord) {
    int x = pix_coord.x % size.x;
    int y = pix_coord.y % size.y;

    vec4 color = imageLoad(current_generation, ivec2(x, y));
    return color.r > 0.;
}

// count the neighbours of the given pixel
uint count_neighbours(ivec2 pix_coord) {
    uint count = 0;

    for(int i = -1; i < 2; i++) {
        for(int j = -1; j < 2; j++) {
            if(i != 0 && j != 0) {
                pix_coord.x += i;
                pix_coord.y += j;
                if(get_pix_state(pix_coord)) {
                    count += 1;
                }
            }
        }
    }

    return count;
}

// compute the next state of the given pixel
// we also give it the number of neighbours
bool should_live(ivec2 pix_coord, bool alive, uint neighbours_nb) {
    if(alive) {
        if(neighbours_nb < 2 || neighbours_nb > 3) {
            alive = false;
        }
    } else {
        if(neighbours_nb == 3) {
            alive = true;
        }
    }

    return alive;
}

// pause the game if the left button is clicked
bool is_paused() {
    return mouse.z == 1;
}

bool is_placing_pixels(ivec2 pix_coord) {
    // the distance from the mouse to the pixel
    float dist_mouse = distance(mouse.xy, vec2(pix_coord) / size.x);

    return dist_mouse < radius_mouse / size.x && is_paused();
}

void main() {
    ivec2 pixel_coord = ivec2(gl_GlobalInvocationID.xy);

    // get current state
    bool alive = get_pix_state(pixel_coord);

    // get number of neighbours
    uint neighbours_nb = count_neighbours(pixel_coord);

    // update next state if not paused
    if(!is_paused()) {
        alive = should_live(pixel_coord, alive, neighbours_nb);
    }

    // set pixel as alive (white) by default
    vec4 px = vec4(1);

    // if not alive, set red value to zero
    // if the mouse is placing the pixel, keep alive
    if(!alive && !(is_placing_pixels(pixel_coord))) {
        px.r = 0;
    }

    imageStore(next_generation, pixel_coord, px);
}
