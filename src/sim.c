/* sim.c -- https://github.com/takeiteasy/sim

 The MIT License (MIT)

 Copyright (c) 2024 George Watson

 Permission is hereby granted, free of charge, to any person
 obtaining a copy of this software and associated documentation
 files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge,
 publish, distribute, sublicense, and/or sell copies of the Software,
 and to permit persons to whom the Software is furnished to do so,
 subject to the following conditions:

 The above copyright notice and this permission notice shall be
 included in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. */

#define SOKOL_IMPL
#include "sim.h"
#define SOKOL_NO_ENTRY
#include "sokol_app.h"
#include "sokol_glue.h"
#include "sokol_args.h"
#include "sokol_time.h"

#if defined(SIM_WINDOWS)
#include <shlobj.h>
#if !defined(_DLL)
#include <shellapi.h>
#pragma comment(lib, "shell32")

extern int main(int argc, const char *argv[]);

#ifdef UNICODE
int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
#else
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#endif // UNICODE
{
    int n, argc;
    LPWSTR *wargv = CommandLineToArgvW(GetCommandLineW(), &argc);
    char **argv = calloc(argc + 1, sizeof(int));

    (void)hInstance;
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    for (n = 0; n < argc; n++) {
        int len = WideCharToMultiByte(CP_UTF8, 0, wargv[n], -1, 0, 0, NULL, NULL);
        argv[n] = malloc(len);
        WideCharToMultiByte(CP_UTF8, 0, wargv[n], -1, argv[n], len, NULL, NULL);
    }
    return main(argc, argv);
}
#endif // !_DLL
#endif // SIM_WINDOWS

#if !defined(DEFAULT_WINDOW_WIDTH)
#define DEFAULT_WINDOW_WIDTH 640
#endif

#if !defined(DEFAULT_WINDOW_HEIGHT)
#define DEFAULT_WINDOW_HEIGHT 480
#endif

#if !defined(DEFAULT_WINDOW_TITLE)
#define DEFAULT_WINDOW_TITLE "SIM"
#endif

#if !defined(DEFAULT_KEY_HOLD_DELAY)
#define DEFAULT_KEY_HOLD_DELAY 1
#endif

#if !defined(MAX_MATRIX_STACK)
#define MAX_MATRIX_STACK 32
#endif

typedef struct {
    float m0, m4, m8, m12;
    float m1, m5, m9, m13;
    float m2, m6, m10, m14;
    float m3, m7, m11, m15;
} mat4;

typedef struct {
    float x, y, z;
} vec3;

static vec3 vec3_new(float x, float y, float z) {
    return (vec3){x, y, z};
}

static vec3 vec3_normalize(vec3 v) {
    float length = sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
    if (!length)
        return v;
    else {
        float ilength = 1.0f/length;
        return (vec3) { v.x * ilength, v.y * ilength, v.z * ilength };
    }
}

static vec3 vec3_cross(vec3 a, vec3 b) {
    return (vec3) {
        a.y*b.z - a.z*b.y,
        a.z*b.x - a.x*b.z,
        a.x*b.y - a.y*b.x
    };
}

static float vec3_dot(vec3 a, vec3 b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

static vec3 vec3_sub(vec3 a, vec3 b) {
    return (vec3){ a.x - b.x, a.y - b.y, a.z - b.z };
}

static mat4 mat4_id(void) {
    return (mat4) {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
}

typedef struct sim_state_t {
    sg_pass_action pass_action;
    mat4 matrix_stack[MAX_MATRIX_STACK];
    int matrix_stack_count;
    mat4 modelview, projection, texture, transform, *current;
    int dirty;
    int matrix_mode;
    struct sim_state_t *next;
} sim_state_t;

static sim_state_t* create_default_state(void) {
    sim_state_t *result = malloc(sizeof(sim_state_t));
    memset(result, 0, sizeof(sim_state_t));
    result->pass_action = (sg_pass_action) {
        .colors[0] = (sg_color_attachment_action) {
            .load_action = SG_LOADACTION_CLEAR,
            .clear_value = (sg_color){0.39f,0.58f,0.92f,1.f}
        }
    };
    result->matrix_mode = SIM_MODELVIEW;
    result->matrix_stack_count = 1;
    result->next = NULL;
    return result;
}

static void release_state(sim_state_t *state) {
    if (state) {
        free(state);
    }
}

typedef struct {
    int down;
    uint64_t timestamp;
} sim_input_state_t;

typedef struct {
    sim_input_state_t keys[KEY_MENU+1];
    sim_input_state_t buttons[3];
    sim_modifier_t modifier;
    struct {
        int x, y;
    } cursor;
    struct {
        float x, y;
    } scroll;
} sim_input_t;

static struct sim_t {
    int running;
    int mouse_hidden;
    int mouse_locked;
    sapp_desc app_desc;
    void(*loop)(float);
#define X(NAME, ARGS) void(*NAME##_callback)ARGS;
    SIM_CALLBACKS
#undef X
    void *userdata;
    sim_state_t *default_state;
    struct {
        sim_state_t *head, *tail;
    } states;
    sim_input_t current_input;
    sim_input_t last_input;
} sim = {
    .running = 0,
    .mouse_hidden = 0,
    .mouse_locked = 0,
    .app_desc = (sapp_desc) {
        .width = DEFAULT_WINDOW_WIDTH,
        .height = DEFAULT_WINDOW_WIDTH,
        .window_title = DEFAULT_WINDOW_TITLE
    },
#define X(NAME, ARGS) .NAME##_callback = NULL,
    SIM_CALLBACKS
#undef X
    .userdata = NULL,
    .default_state = NULL,
    .states.head = NULL,
    .states.tail = NULL
};

#define sim_callback(CB, ...) \
    if (sim.CB##_callback)    \
        sim.CB##_callback(sim.userdata, __VA_ARGS__)

#define X(NAME, ARGS) void(*NAME##_callback)ARGS,
void sim_set_callbacks(SIM_CALLBACKS void* userdata) {
#undef X
#define X(NAME, ARGS) \
    sim.NAME##_callback = NAME##_callback;
    SIM_CALLBACKS
#undef X
    sim.userdata = userdata;
}

#define X(NAME, ARGS)                                            \
    void sim_set_##NAME##_callback(void(*NAME##_callback)ARGS) { \
        sim.NAME##_callback = NAME##_callback;                   \
    }
SIM_CALLBACKS
#undef X


static void init(void) {
    stm_setup();
}

static void frame(void) {
    memcpy(&sim.last_input, &sim.current_input, sizeof(sim_input_t));
    memset(&sim.current_input, 0, sizeof(sim_input_t));
    sim.loop(0.f);
}

static void event(const sapp_event *e) {
    switch (e->type) {
        case SAPP_EVENTTYPE_KEY_UP:
        case SAPP_EVENTTYPE_KEY_DOWN:
            sim.current_input.keys[e->key_code].down = SAPP_EVENTTYPE_KEY_DOWN;
            sim.current_input.keys[e->key_code].timestamp = stm_now();
            sim.current_input.modifier = e->modifiers;
            break;
        case SAPP_EVENTTYPE_MOUSE_UP:
        case SAPP_EVENTTYPE_MOUSE_DOWN:
            sim.current_input.buttons[e->mouse_button].down = SAPP_EVENTTYPE_MOUSE_DOWN;
            sim.current_input.buttons[e->mouse_button].timestamp = stm_now();
            sim.current_input.modifier = e->modifiers;
            break;
        case SAPP_EVENTTYPE_MOUSE_MOVE:
            sim.current_input.cursor.x = e->mouse_x;
            sim.current_input.cursor.y = e->mouse_y;
            break;
        case SAPP_EVENTTYPE_MOUSE_SCROLL:
            sim.current_input.scroll.x = e->scroll_x;
            sim.current_input.scroll.y = e->scroll_y;
            break;
        default:
            break;
    }
}

static void cleanup(void) {
    sg_shutdown();
}

int sim_run(int argc, const char *argv[], void(*loop)(float)) {
    assert(!sim.running);
    assert((sim.loop = loop));
    sim.app_desc.init_cb = init;
    sim.app_desc.frame_cb = frame;
    sim.app_desc.event_cb = event;
    sim.app_desc.cleanup_cb = cleanup;
    sapp_run(&sim.app_desc);
    return 0;
}

int sim_window_width(void) {
    return sim.running ? sapp_width() : -1;
}

int sim_window_height(void) {
    return sim.running ? sapp_height() : -1;
}

int sim_is_window_fullscreen(void) {
    return sim.running ? sapp_is_fullscreen() : sim.app_desc.fullscreen;
}

void sim_toggle_fullscreen(void) {
    if (!sim.running)
        sim.app_desc.fullscreen = !sim.app_desc.fullscreen;
    else
        sapp_toggle_fullscreen();
}

int sim_is_cursor_visible(void) {
    return sim.running ? sapp_mouse_shown() : sim.mouse_hidden;
}

void sim_toggle_cursor_visible(void) {
    if (sim.running)
        sapp_show_mouse(!sim.mouse_hidden);
    sim.mouse_hidden = !sim.mouse_hidden;
}

int sim_is_cursor_locked(void) {
    return sim.running? sapp_mouse_locked() : sim.mouse_locked;
}

void sim_toggle_cursor_lock(void) {
    if (sim.running)
        sapp_lock_mouse(!sim.mouse_locked);
    sim.mouse_locked = !sim.mouse_locked;
}

int sim_is_key_down(sim_key_t key) {
    return sim.current_input.keys[key].down == 1;
}

int sim_is_key_held(sim_key_t key) {
    return sim_is_key_down(key) && stm_sec(stm_since(sim.current_input.keys[key].timestamp)) > DEFAULT_KEY_HOLD_DELAY;
}

int sim_was_key_pressed(sim_key_t key) {
    return sim_is_key_down(key) && !sim.last_input.keys[key].down;
}

int sim_was_key_released(sim_key_t key) {
    return !sim_is_key_down(key) && sim.last_input.keys[key].down;
}

int sim_are_keys_down(int n, ...) {
    va_list args;
    va_start(args, n);
    int result = 1;
    for (int i = 0; i < n; i++)
        if (!sim.current_input.keys[va_arg(args, int)].down) {
            result = 0;
            goto BAIL;
        }
BAIL:
    va_end(args);
    return result;
}

int sim_any_keys_down(int n, ...) {
    va_list args;
    va_start(args, n);
    int result = 0;
    for (int i = 0; i < n; i++)
        if (sim.current_input.keys[va_arg(args, int)].down) {
            result = 1;
            goto BAIL;
        }
BAIL:
    va_end(args);
    return result;
}

int sim_is_button_down(sim_button_t button) {
    return sim.current_input.buttons[button].down;
}

int sim_is_button_held(sim_button_t button) {
    return sim_is_button_down(button) && stm_sec(stm_since(sim.current_input.buttons[button].timestamp)) > DEFAULT_KEY_HOLD_DELAY;
}

int sim_was_button_pressed(sim_button_t button) {
    return sim_is_button_down(button) && !sim.last_input.buttons[button].down;
}

int sim_was_button_released(sim_button_t button) {
    return !sim_is_button_down(button) && sim.last_input.buttons[button].down;
}

int sim_are_buttons_down(int n, ...) {
    va_list args;
    va_start(args, n);
    int result = 1;
    for (int i = 0; i < n; i++)
        if (!sim.current_input.buttons[va_arg(args, int)].down) {
            result = 0;
            goto BAIL;
        }
BAIL:
    va_end(args);
    return result;
}

int sim_any_buttons_down(int n, ...) {
    va_list args;
    va_start(args, n);
    int result = 0;
    for (int i = 0; i < n; i++)
        if (sim.current_input.buttons[va_arg(args, int)].down) {
            result = 1;
            goto BAIL;
        }
BAIL:
    va_end(args);
    return result;
}

int sim_has_mouse_move(void) {
    return sim.current_input.cursor.x != sim.last_input.cursor.x || sim.current_input.cursor.y != sim.last_input.cursor.y;
}

int sim_cursor_x(void) {
    return sim.current_input.cursor.x;
}

int sim_cursor_y(void) {
    return sim.current_input.cursor.y;
}

int sim_cursor_delta_x(void) {
    return sim.current_input.cursor.x - sim.last_input.cursor.x;
}

int sim_cursor_delta_y(void) {
    return sim.current_input.cursor.y - sim.last_input.cursor.y;
}

int sim_has_wheel_moved(void) {
    return sim.current_input.scroll.x != 0 || sim.current_input.scroll.y != 0;
}

float sim_scroll_x(void) {
    return sim.current_input.scroll.x;
}

float sim_scroll_y(void) {
    return sim.current_input.scroll.y;
}

void* sim_current_state(void) {
    return (void*)sim.states.tail;
}

void* sim_pop_state(void) {
    assert(sim.states.head && sim.states.tail);
    assert(sim.states.head != sim.states.tail);
    void *result = sim.states.tail;
    sim_state_t *cursor = sim.states.head;
    while (cursor) {
        if (cursor->next == result) {
            sim.states.tail = cursor;
            goto BAIL;
        }
        cursor = cursor->next;
    }
    abort(); // shouldn't be reached
BAIL:
    return result;
}

void sim_push_state(void *state) {
    if (!sim.states.head)
        sim.states.head = sim.states.tail = state;
    else {
        sim.states.tail->next = state;
        sim.states.tail = state;
    }
}

void sim_matrix_mode(int mode) {
    switch (mode) {
        case SIM_MODELVIEW:
            sim.states.tail->current = &sim.states.tail->modelview;
            break;
        case SIM_PROJECTION:
            sim.states.tail->current = &sim.states.tail->projection;
            break;
        case SIM_TEXTURE:
            sim.states.tail->current = &sim.states.tail->texture;
            break;
        default:
            abort(); // unknown mode
    }
    sim.states.tail->matrix_mode = mode;
}

void sim_push_matrix(void) {
    int n = sim.states.tail->matrix_stack_count + 1;
    assert(n < MAX_MATRIX_STACK);
    memset(&sim.states.tail->matrix_stack[n-1], 0, sizeof(mat4));
    sim.states.tail->matrix_stack_count = n;
}

void sim_pop_matrix(void) {
    if (sim.states.tail->matrix_stack_count == 1)
        memset(sim.states.tail->matrix_stack, 0, MAX_MATRIX_STACK * sizeof(mat4));
    else
        memset(&sim.states.tail->matrix_stack[sim.states.tail->matrix_stack_count--], 0, sizeof(mat4));
}

void sim_load_identity(void) {
    *sim.states.tail->current = mat4_id();
}

void sim_mul_matrix(const float *matf) {
    mat4 result, left = *sim.states.tail->current, right;
    memcpy(&right, matf, sizeof(float)*16);
    result.m0 = left.m0*right.m0 + left.m1*right.m4 + left.m2*right.m8 + left.m3*right.m12;
    result.m1 = left.m0*right.m1 + left.m1*right.m5 + left.m2*right.m9 + left.m3*right.m13;
    result.m2 = left.m0*right.m2 + left.m1*right.m6 + left.m2*right.m10 + left.m3*right.m14;
    result.m3 = left.m0*right.m3 + left.m1*right.m7 + left.m2*right.m11 + left.m3*right.m15;
    result.m4 = left.m4*right.m0 + left.m5*right.m4 + left.m6*right.m8 + left.m7*right.m12;
    result.m5 = left.m4*right.m1 + left.m5*right.m5 + left.m6*right.m9 + left.m7*right.m13;
    result.m6 = left.m4*right.m2 + left.m5*right.m6 + left.m6*right.m10 + left.m7*right.m14;
    result.m7 = left.m4*right.m3 + left.m5*right.m7 + left.m6*right.m11 + left.m7*right.m15;
    result.m8 = left.m8*right.m0 + left.m9*right.m4 + left.m10*right.m8 + left.m11*right.m12;
    result.m9 = left.m8*right.m1 + left.m9*right.m5 + left.m10*right.m9 + left.m11*right.m13;
    result.m10 = left.m8*right.m2 + left.m9*right.m6 + left.m10*right.m10 + left.m11*right.m14;
    result.m11 = left.m8*right.m3 + left.m9*right.m7 + left.m10*right.m11 + left.m11*right.m15;
    result.m12 = left.m12*right.m0 + left.m13*right.m4 + left.m14*right.m8 + left.m15*right.m12;
    result.m13 = left.m12*right.m1 + left.m13*right.m5 + left.m14*right.m9 + left.m15*right.m13;
    result.m14 = left.m12*right.m2 + left.m13*right.m6 + left.m14*right.m10 + left.m15*right.m14;
    result.m15 = left.m12*right.m3 + left.m13*right.m7 + left.m14*right.m11 + left.m15*right.m15;
    *sim.states.tail->current = result;
}

void sim_translate(float x, float y, float z) {
    mat4 translate = mat4_id();
    translate.m12 = x;
    translate.m13 = y;
    translate.m14 = z;
    float buffer[16];
    memcpy(&buffer, &translate, sizeof(float)*16);
    sim_mul_matrix(buffer);
}

void sim_rotate(float angle, float x, float y, float z) {
    float lengthSquared = x*x + y*y + z*z;
    if (lengthSquared != 1.f && lengthSquared != 0.f) {
        float inverseLength = 1.f/sqrtf(lengthSquared);
        x *= inverseLength;
        y *= inverseLength;
        z *= inverseLength;
    }
    
    float radian = 0.01745329252f*angle;
    float sinres = sinf(radian);
    float cosres = cosf(radian);
    float t = 1.f - cosres;
    
    float rotation[16];
    rotation[0] = x*x*t + cosres;
    rotation[1] = y*x*t + z*sinres;
    rotation[2] = z*x*t - y*sinres;
    rotation[3] = 0.f;
    rotation[4] = x*y*t - z*sinres;
    rotation[5] = y*y*t + cosres;
    rotation[6] = z*y*t + x*sinres;
    rotation[7] = 0.f;
    rotation[8] = x*z*t + y*sinres;
    rotation[9] = y*z*t - x*sinres;
    rotation[10] = z*z*t + cosres;
    rotation[11] = 0.f;
    rotation[12] = 0.f;
    rotation[13] = 0.f;
    rotation[14] = 0.f;
    rotation[15] = 1.;
    sim_mul_matrix(rotation);
}

void sim_scale(float x, float y, float z) {
    mat4 scale = mat4_id();
    scale.m0 = x;
    scale.m5 = y;
    scale.m10 = z;
    scale.m15 = 1.f;
    float buffer[16];
    memcpy(&buffer, &scale, sizeof(float)*16);
    sim_mul_matrix(buffer);
}

void sim_frustum(double left, double right, double bottom, double top, double znear, double zfar) {
    float rl = (float)(right - left);
    float tb = (float)(top - bottom);
    float fn = (float)(zfar - znear);
    float frustum[16];
    frustum[0] = ((float)znear*2.f)/rl;
    frustum[1] = 0.f;
    frustum[2] = 0.f;
    frustum[3] = 0.f;
    frustum[4] = 0.f;
    frustum[5] = ((float) znear*2.f)/tb;
    frustum[6] = 0.f;
    frustum[7] = 0.f;
    frustum[8] = ((float)right + (float)left)/rl;
    frustum[9] = ((float)top + (float)bottom)/tb;
    frustum[10] = -((float)zfar + (float)znear)/fn;
    frustum[11] = -1.f;
    frustum[12] = 0.f;
    frustum[13] = 0.f;
    frustum[14] = -((float)zfar*(float)znear*2.f)/fn;
    frustum[15] = 0.f;
    sim_mul_matrix(frustum);
}

void sim_ortho(double left, double right, double bottom, double top, double znear, double zfar) {
    float rl = (float)(right - left);
    float tb = (float)(top - bottom);
    float fn = (float)(zfar - znear);
    float ortho[16];
    ortho[0] = 2.f/rl;
    ortho[1] = 0.f;
    ortho[2] = 0.f;
    ortho[3] = 0.f;
    ortho[4] = 0.f;
    ortho[5] = 2.f/tb;
    ortho[6] = 0.f;
    ortho[7] = 0.f;
    ortho[8] = 0.f;
    ortho[9] = 0.f;
    ortho[10] = -2.f/fn;
    ortho[11] = 0.f;
    ortho[12] = -((float)left + (float)right)/rl;
    ortho[13] = -((float)top + (float)bottom)/tb;
    ortho[14] = -((float)zfar + (float)znear)/fn;
    ortho[15] = 1.f;
    sim_mul_matrix(ortho);
}

void sim_look_at(float eyeX, float eyeY, float eyeZ,
                 float targetX, float targetY, float targetZ,
                 float upX, float upY, float upZ) {
    vec3 eye = vec3_new(eyeX, eyeY, eyeZ);
    vec3 target = vec3_new(targetX, targetY, targetZ);
    vec3 up = vec3_new(upX, upY, upZ);
    vec3 vz = vec3_normalize(vec3_sub(eye, target));
    vec3 vx = vec3_cross(up, vz);
    vec3 vy = vec3_cross(vz, vx);
    
    mat4 result;
    result.m0 = vx.x;
    result.m1 = vy.x;
    result.m2 = vz.x;
    result.m3 = 0.0f;
    result.m4 = vx.y;
    result.m5 = vy.y;
    result.m6 = vz.y;
    result.m7 = 0.0f;
    result.m8 = vx.z;
    result.m9 = vy.z;
    result.m10 = vz.z;
    result.m11 = 0.0f;
    result.m12 = -vec3_dot(vx, eye);
    result.m13 = -vec3_dot(vy, eye);
    result.m14 = -vec3_dot(vz, eye);
    result.m15 = 1.0f;
    *sim.states.tail->current = result;
}
