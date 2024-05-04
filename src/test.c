//
//  test.c
//  sim
//
//  Created by George Watson on 02/05/2024.
//

#include "sim.h"

static void loop(double delta) {
    sim_begin(SIM_TRIANGLES);
    sim_color4f(1.0f, 0.0f, 0.0f, 1.0f);
    sim_vertex3f(0.0f,  0.5f, 0.5f);
    sim_color4f(0.0f, 1.0f, 0.0f, 1.0f);
    sim_vertex3f(0.5f, -0.5f, 0.5f);
    sim_color4f(0.0f, 0.0f, 1.0f, 1.0f);
    sim_vertex3f(-0.5f, -0.5f, 0.5f);
    sim_flush();
    sim_end();
    sim_commit();
}

int main(int argc, const char *argv[]) {
    return sim_run(argc, argv, loop);
}
