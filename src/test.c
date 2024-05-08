//
//  test.c
//  sim
//
//  Created by George Watson on 02/05/2024.
//

#include "sim.h"

static void init(void) {
    
}

static void loop(double delta) {

}

static void deinit(void) {
    
}

int main(int argc, const char *argv[]) {
    return sim_run(800, 600, "sim", init, loop, deinit);
}
