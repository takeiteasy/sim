//
//  test.c
//  sim
//
//  Created by George Watson on 02/05/2024.
//

#include "sim.h"

static float vertices[] = {
    -1.0, -1.0, -1.0, 1.0,  1.0, 0.0, 0.0, 1.0,
     1.0, -1.0, -1.0, 1.0,  1.0, 0.0, 0.0, 1.0,
     1.0,  1.0, -1.0, 1.0,  1.0, 0.0, 0.0, 1.0,
    -1.0,  1.0, -1.0, 1.0,  1.0, 0.0, 0.0, 1.0,
    
    -1.0, -1.0,  1.0, 1.0,  0.0, 1.0, 0.0, 1.0,
     1.0, -1.0,  1.0, 1.0,  0.0, 1.0, 0.0, 1.0,
     1.0,  1.0,  1.0, 1.0,  0.0, 1.0, 0.0, 1.0,
    -1.0,  1.0,  1.0, 1.0,  0.0, 1.0, 0.0, 1.0,
    
    -1.0, -1.0, -1.0, 1.0,  0.0, 0.0, 1.0, 1.0,
    -1.0,  1.0, -1.0, 1.0,  0.0, 0.0, 1.0, 1.0,
    -1.0,  1.0,  1.0, 1.0,  0.0, 0.0, 1.0, 1.0,
    -1.0, -1.0,  1.0, 1.0,  0.0, 0.0, 1.0, 1.0,
    
     1.0, -1.0, -1.0, 1.0,   1.0, 0.5, 0.0, 1.0,
     1.0,  1.0, -1.0, 1.0,   1.0, 0.5, 0.0, 1.0,
     1.0,  1.0,  1.0, 1.0,   1.0, 0.5, 0.0, 1.0,
     1.0, -1.0,  1.0, 1.0,   1.0, 0.5, 0.0, 1.0,
    
    -1.0, -1.0, -1.0, 1.0,  0.0, 0.5, 1.0, 1.0,
    -1.0, -1.0,  1.0, 1.0,  0.0, 0.5, 1.0, 1.0,
     1.0, -1.0,  1.0, 1.0,  0.0, 0.5, 1.0, 1.0,
     1.0, -1.0, -1.0, 1.0,  0.0, 0.5, 1.0, 1.0,
    
    -1.0,  1.0, -1.0, 1.0,  1.0, 0.0, 0.5, 1.0,
    -1.0,  1.0,  1.0, 1.0,  1.0, 0.0, 0.5, 1.0,
     1.0,  1.0,  1.0, 1.0,  1.0, 0.0, 0.5, 1.0,
     1.0,  1.0, -1.0, 1.0,  1.0, 0.0, 0.5, 1.0
};

static int indices[] = {
    0,  1,  2,   0,  2,  3,
    6,  5,  4,   7,  6,  4,
    8,  9,  10,  8,  10, 11,
    14, 13, 12,  15, 14, 12,
    16, 17, 18,  16, 18, 19,
    22, 21, 20,  23, 22, 20
};

static int texture = 0;

static void init(void) {
    texture = sim_load_texture_path("/Users/george/Downloads/pear.jpg");
}

static void loop(double t) {
    static float rx = 0.f;
    static float ry = 0.f;
    rx += 1.f * t;
    ry += 2.f * t;
    sim_matrix_mode(SIM_MATRIXMODE_PROJECTION);
    sim_load_identity();
    sim_perspective(60.f, (float)sim_window_width() / (float)sim_window_height(), .01f, 10.f);
    sim_matrix_mode(SIM_MATRIXMODE_MODELVIEW);
    sim_load_identity();
    sim_look_at(0.0f, 1.5f, 6.0f,
                0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f);
    sim_rotate(rx, 1.f, 0.f, 0.f);
    sim_rotate(ry, 0.f, 1.f, 0.f);

    sim_begin(SIM_DRAW_TRIANGLES);
    for (int i = 0; i < 36; i++) {
        int j = indices[i];
        float *data = vertices + j * 8;
        sim_color4f(data[4], data[5], data[6], data[7]);
        sim_vertex3f(data[0], data[1], data[2]);
    }
    
    sim_push_texture(texture);
    sim_draw();
    sim_end();
    sim_pop_texture();
}

static void deinit(void) {
    
}

int main(int argc, const char *argv[]) {
    return sim_run(800, 600, "sim", init, loop, deinit);
}
