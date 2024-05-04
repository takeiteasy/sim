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
#include "default.glsl.h"

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
    float x, y;
} vec2;

typedef struct {
    float x, y, z;
} vec3;

typedef struct {
    float x, y, z, w;
} vec4;

static vec2 vec2_new(float x, float y) {
    return (vec2){x, y};
}

static vec3 vec3_new(float x, float y, float z) {
    return (vec3){x, y, z};
}

static vec4 vec4_new(float x, float y, float z, float w) {
    return (vec4){x, y, z, w};
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

typedef struct {
    vec3 position;
    vec3 normal;
    vec2 texcoord;
    vec4 color;
} sim_vertex_t;

typedef struct {
    vec4 xxxx;
    vec4 yyyy;
    vec4 zzzz;
    vec4 wwww;
} sim_vertex_inst_t;

typedef struct {
    sim_vertex_t *vertices;
    sim_vertex_inst_t *instances;
    int vcount, icount;
} sim_vertex_buffer_t;

typedef struct {
    sg_pass_action pass_action;
    mat4 matrix_stack[MAX_MATRIX_STACK];
    int matrix_stack_count;
    mat4 modelview, projection, texture, *current;
    int matrix_mode;
    sim_vertex_buffer_t *vbuffer;
    sg_image texture0;
    vec2 texcoord;
    vec3 normal;
    vec4 color;
    sg_pipeline_desc pip_desc;
    sg_blend_state blend;
    int blend_mode;
} sim_state_t;

typedef struct {
    int down;
    uint64_t timestamp;
} sim_input_state_t;

typedef struct {
    sim_input_state_t keys[KEY_MENU+1];
    sim_input_state_t buttons[3];
    int modifier;
    struct {
        int x, y;
    } cursor;
    struct {
        float x, y;
    } scroll;
} sim_input_t;

typedef enum {
    INVALID_COMMAND = 0,
    VIEWPORT_COMMAND,
    SCISSOR_RECT_COMMAND,
    DRAW_CALL_COMMAND
} sim_command_type_t;

typedef struct sim_command_t {
    sim_command_type_t type;
    void *data;
    struct sim_command_t *next;
} sim_command_t;

typedef struct {
    sim_command_t *head, *tail;
} sim_command_stack_t;

typedef struct {
    int x, y, w, h;
} sim_rect_t;

typedef struct {
    int w, h;
    sg_image color, depth;
    sg_pass pass;
} sim_framebuffer_t;

typedef struct {
    sg_pass_action pass_action;
    sg_pipeline pip;
    sg_bindings bind;
    int vcount;
    int icount;
} sim_draw_call_t;

static struct sim_t {
    int running;
    int mouse_hidden;
    int mouse_locked;
    sapp_desc app_desc;
    void(*loop)(double);
    uint64_t last_time;
    void *userdata;
    sim_state_t state;
    sim_input_t current_input;
    sim_input_t last_input;
    sim_command_stack_t command_stack;
    sg_shader shader;
} sim = {
    .running = 0,
    .mouse_hidden = 0,
    .mouse_locked = 0,
    .app_desc = (sapp_desc) {
        .width = DEFAULT_WINDOW_WIDTH,
        .height = DEFAULT_WINDOW_WIDTH,
        .window_title = DEFAULT_WINDOW_TITLE
    },
    .userdata = NULL
};

static void init(void) {
    sg_desc desc = {
        .context = sapp_sgcontext(),
        .buffer_pool_size = 256
    };
    sg_setup(&desc);
    stm_setup();
    
    sim.shader = sg_make_shader(sim_shader_desc(sg_query_backend()));
    sim.state.pip_desc = (sg_pipeline_desc) {
        .shader = sim.shader,
        .layout = {
            .buffers[1].step_func = SG_VERTEXSTEP_PER_INSTANCE,
            .attrs = {
                [ATTR_vs_position] = {.format=SG_VERTEXFORMAT_FLOAT3, .buffer_index=0},
                [ATTR_vs_normal] = {.format=SG_VERTEXFORMAT_FLOAT3, .buffer_index=0},
                [ATTR_vs_texcoord0] = {.format=SG_VERTEXFORMAT_FLOAT2, .buffer_index=0},
                [ATTR_vs_color0] = {.format=SG_VERTEXFORMAT_FLOAT4, .buffer_index=0},
                [ATTR_vs_inst_mat_xxxx] = {.format=SG_VERTEXFORMAT_FLOAT4, .buffer_index=1},
                [ATTR_vs_inst_mat_yyyy] = {.format=SG_VERTEXFORMAT_FLOAT4, .buffer_index=1},
                [ATTR_vs_inst_mat_zzzz] = {.format=SG_VERTEXFORMAT_FLOAT4, .buffer_index=1},
                [ATTR_vs_inst_mat_wwww] = {.format=SG_VERTEXFORMAT_FLOAT4, .buffer_index=1}
            }
        },
        .depth = {
            .write_enabled = true
        },
    };
    sim.state.blend_mode = -1;
    sim.state.pip_desc.depth.compare = -1;
    sim.state.pip_desc.cull_mode = -1;
    sim_blend_mode(SIM_BLEND_NONE);
    sim_depth_func(SIM_CMP_DEFAULT);
    sim_cull_mode(SIM_CULL_DEFAULT);
    sim.last_time = stm_now();
}

static void frame(void) {
    uint64_t now = stm_now();
    double diff = stm_ms(stm_diff(now, sim.last_time));
    sim.loop(diff);
    sim.last_time = now;
    memcpy(&sim.last_input, &sim.current_input, sizeof(sim_input_t));
    memset(&sim.current_input, 0, sizeof(sim_input_t));
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

int sim_run(int argc, const char *argv[], void(*loop)(double)) {
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

int sim_is_key_down(int key) {
    return sim.current_input.keys[key].down == 1;
}

int sim_is_key_held(int key) {
    return sim_is_key_down(key) && stm_sec(stm_since(sim.current_input.keys[key].timestamp)) > DEFAULT_KEY_HOLD_DELAY;
}

int sim_was_key_pressed(int key) {
    return sim_is_key_down(key) && !sim.last_input.keys[key].down;
}

int sim_was_key_released(int key) {
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

int sim_is_button_down(int button) {
    return sim.current_input.buttons[button].down;
}

int sim_is_button_held(int button) {
    return sim_is_button_down(button) && stm_sec(stm_since(sim.current_input.buttons[button].timestamp)) > DEFAULT_KEY_HOLD_DELAY;
}

int sim_was_button_pressed(int button) {
    return sim_is_button_down(button) && !sim.last_input.buttons[button].down;
}

int sim_was_button_released(int button) {
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

void sim_matrix_mode(int mode) {
    switch (mode) {
        case SIM_MODELVIEW:
            sim.state.current = &sim.state.modelview;
            break;
        case SIM_PROJECTION:
            sim.state.current = &sim.state.projection;
            break;
        case SIM_TEXTURE:
            sim.state.current = &sim.state.texture;
            break;
        default:
            abort(); // unknown mode
    }
    sim.state.matrix_mode = mode;
}

void sim_push_matrix(void) {
    int n = sim.state.matrix_stack_count + 1;
    assert(n < MAX_MATRIX_STACK);
    memset(&sim.state.matrix_stack[n-1], 0, sizeof(mat4));
    sim.state.matrix_stack_count = n;
}

void sim_pop_matrix(void) {
    if (sim.state.matrix_stack_count == 1)
        memset(sim.state.matrix_stack, 0, MAX_MATRIX_STACK * sizeof(mat4));
    else
        memset(&sim.state.matrix_stack[sim.state.matrix_stack_count--], 0, sizeof(mat4));
}

void sim_load_identity(void) {
    *sim.state.current = mat4_id();
}

void sim_mul_matrix(const float *matf) {
    mat4 result, left = *sim.state.current, right;
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
    *sim.state.current = result;
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
    *sim.state.current = result;
}

void sim_clear_color(float r, float g, float b, float a) {
    sim.state.pass_action.colors[0].clear_value = (sg_color) {
        .r = r,
        .g = g,
        .b = b,
        .a = a
    };
}

static void push_command(sim_command_stack_t *stack, sim_command_type_t type, void *data) {
    sim_command_t *command = malloc(sizeof(sim_command_t));
    assert((command->type = type));
    command->data = data;
    if (!stack->head)
        stack->head = stack->tail = command;
    else {
        assert(stack->tail);
        stack->tail->next = command;
        stack->tail = command;
    }
}

static sim_command_t* pop_command(sim_command_stack_t *stack) {
    sim_command_t *head = stack->head;
    if (head)
        if (!(stack->head = stack->head->next))
            stack->tail = NULL;
    return head;
}

void sim_viewport(int x, int y, int width, int height) {
    sim_rect_t *rect = malloc(sizeof(sim_rect_t));
    rect->x = x;
    rect->y = y;
    rect->w = width;
    rect->h = height;
    push_command(&sim.command_stack, VIEWPORT_COMMAND, rect);
}

void sim_scissor_rect(int x, int y, int width, int height) {
    sim_rect_t *rect = malloc(sizeof(sim_rect_t));
    rect->x = x;
    rect->y = y;
    rect->w = width;
    rect->h = height;
    push_command(&sim.command_stack, SCISSOR_RECT_COMMAND, rect);
}

void sim_blend_mode(int mode) {
    if (mode == sim.state.blend_mode)
        return;
    sg_blend_state *blend = &sim.state.blend;
    switch (mode) {
        default:
        case SIM_BLEND_NONE:
            blend->enabled = false;
            blend->src_factor_rgb = SG_BLENDFACTOR_ONE;
            blend->dst_factor_rgb = SG_BLENDFACTOR_ZERO;
            blend->op_rgb = SG_BLENDOP_ADD;
            blend->src_factor_alpha = SG_BLENDFACTOR_ONE;
            blend->dst_factor_alpha = SG_BLENDFACTOR_ZERO;
            blend->op_alpha = SG_BLENDOP_ADD;
            break;
        case SIM_BLEND_BLEND:
            blend->enabled = true;
            blend->src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA;
            blend->dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
            blend->op_rgb = SG_BLENDOP_ADD;
            blend->src_factor_alpha = SG_BLENDFACTOR_ONE;
            blend->dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
            blend->op_alpha = SG_BLENDOP_ADD;
            break;
        case SIM_BLEND_ADD:
            blend->enabled = true;
            blend->src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA;
            blend->dst_factor_rgb = SG_BLENDFACTOR_ONE;
            blend->op_rgb = SG_BLENDOP_ADD;
            blend->src_factor_alpha = SG_BLENDFACTOR_ZERO;
            blend->dst_factor_alpha = SG_BLENDFACTOR_ONE;
            blend->op_alpha = SG_BLENDOP_ADD;
            break;
        case SIM_BLEND_MOD:
            blend->enabled = true;
            blend->src_factor_rgb = SG_BLENDFACTOR_DST_COLOR;
            blend->dst_factor_rgb = SG_BLENDFACTOR_ZERO;
            blend->op_rgb = SG_BLENDOP_ADD;
            blend->src_factor_alpha = SG_BLENDFACTOR_ZERO;
            blend->dst_factor_alpha = SG_BLENDFACTOR_ONE;
            blend->op_alpha = SG_BLENDOP_ADD;
            break;
        case SIM_BLEND_MUL:
            blend->enabled = true;
            blend->src_factor_rgb = SG_BLENDFACTOR_DST_COLOR;
            blend->dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
            blend->op_rgb = SG_BLENDOP_ADD;
            blend->src_factor_alpha = SG_BLENDFACTOR_DST_ALPHA;
            blend->dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
            blend->op_alpha = SG_BLENDOP_ADD;
            break;
    }
    sim.state.blend_mode = mode;
}

void sim_depth_func(int func) {
    if (func == sim.state.pip_desc.depth.compare)
        return;
    switch (func) {
        default:
        case SIM_CMP_DEFAULT:
            func = SIM_CMP_LESS_EQUAL;
        case SIM_CMP_NEVER:
        case SIM_CMP_LESS:
        case SIM_CMP_EQUAL:
        case SIM_CMP_LESS_EQUAL:
        case SIM_CMP_GREATER:
        case SIM_CMP_NOT_EQUAL:
        case SIM_CMP_GREATER_EQUAL:
        case SIM_CMP_ALWAYS:
        case SIM_CMP_NUM:
            sim.state.pip_desc.depth.compare = (sg_compare_func)func;
            break;
    }
}

void sim_cull_mode(int mode) {
    if (mode == sim.state.pip_desc.cull_mode)
        return;
    switch (mode) {
        default:
        case SIM_CULL_DEFAULT:
            mode = SIM_CULL_BACK;
        case SIM_CULL_NONE:
        case SIM_CULL_FRONT:
        case SIM_CULL_BACK:
            sim.state.pip_desc.cull_mode = mode;
            break;
    }
}

void sim_begin(int mode) {
    assert(!sim.state.vbuffer);
    sim_vertex_buffer_t *vb = sim.state.vbuffer = malloc(sizeof(sim_vertex_buffer_t));
    vb->vcount = 0;
    vb->vertices = malloc(0);
    vb->icount = 0;
    vb->instances = malloc(0);
    sg_primitive_type pt = SG_PRIMITIVETYPE_TRIANGLES;
    switch (mode) {
        default:
        case SIM_TRIANGLES:
            pt = SG_PRIMITIVETYPE_TRIANGLES;
            break;
        case SIM_POINTS:
            pt = SG_PRIMITIVETYPE_POINTS;
            break;
        case SIM_LINES:
            pt = SG_PRIMITIVETYPE_LINES;
            break;
        case SIM_LINE_STRIP:
            pt = SG_PRIMITIVETYPE_LINE_STRIP;
            break;
        case SIM_TRIANGLE_STRIP:
            pt = SG_PRIMITIVETYPE_TRIANGLE_STRIP;
            break;
    }
    sim.state.pip_desc.primitive_type = pt;
}

static void sim_push_vertex(vec3 position) {
    sim_vertex_buffer_t *vb = NULL;
    assert((vb = sim.state.vbuffer));
    vb->vertices = realloc(vb->vertices, ++vb->vcount * sizeof(sim_vertex_t));
    sim_vertex_t *v = &vb->vertices[vb->vcount-1];
    v->position = position;
    v->normal = sim.state.normal;
    v->texcoord = sim.state.texcoord;
    v->color = sim.state.color;
}

void sim_vertex2i(int x, int y) {
    sim_push_vertex(vec3_new((float)x, (float)y, 0.f));
}

void sim_vertex2f(float x, float y) {
    sim_push_vertex(vec3_new(x, y, 0.f));
}

void sim_vertex3f(float x, float y, float z) {
    sim_push_vertex(vec3_new(x, y, z));
}

void sim_texcoord2f(float x, float y) {
    sim.state.texcoord = vec2_new(x, y);
}

void sim_normal3f(float x, float y, float z) {
    sim.state.normal = vec3_new(x, y, z);
}

void sim_color4ub(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    sim.state.color = vec4_new((float)r / 255.f,
                               (float)g / 255.f,
                               (float)b / 255.f,
                               (float)a / 255.f);
}

void sim_color3f(float x, float y, float z) {
    sim.state.color = vec4_new(x, y, z, 1.f);
}

void sim_color4f(float x, float y, float z, float w) {
    sim.state.color = vec4_new(x, y, z, w);
}

void sim_flush(void) {
    sim_vertex_buffer_t *vb = NULL;
    assert((vb = sim.state.vbuffer));
    vb->instances = realloc(vb->instances, ++vb->icount *sizeof(sim_vertex_inst_t));
    sim_vertex_inst_t *inst = &vb->instances[vb->icount-1];
    mat4 m = sim.state.modelview;
    inst->xxxx = vec4_new(m.m0, m.m4, m.m8,  m.m12);
    inst->yyyy = vec4_new(m.m1, m.m5, m.m9,  m.m13);
    inst->zzzz = vec4_new(m.m2, m.m6, m.m10, m.m14);
    inst->wwww = vec4_new(m.m3, m.m7, m.m11, m.m15);
}

void sim_end(void) {
    sim_vertex_buffer_t *vb = NULL;
    assert((vb = sim.state.vbuffer));
    assert(vb->icount && vb->vcount);
    
    sim_draw_call_t *call = malloc(sizeof(sim_draw_call_t));
    memcpy(&call->pass_action, &sim.state.pass_action, sizeof(sg_pass_action));
    call->vcount = vb->vcount;
    call->icount = vb->icount;
    call->pip = sg_make_pipeline(&sim.state.pip_desc);
    sg_buffer_desc b0 = {
        .data = (sg_range) {
            .ptr = vb->vertices,
            .size = vb->vcount * sizeof(sim_vertex_t)
        }
    };
    sg_buffer_desc b1 = {
        .size = vb->icount * sizeof(sim_vertex_inst_t),
        .usage = SG_USAGE_STREAM
    };
    call->bind = (sg_bindings) {
        .vertex_buffers[0] = sg_make_buffer(&b0),
        .vertex_buffers[1] = sg_make_buffer(&b1),
        .fs_images[SLOT_texture0] = sim.state.texture0
    };
    sg_range range = {
        .ptr = vb->instances,
        .size = vb->icount * sizeof(sim_vertex_inst_t)
    };
    sg_update_buffer(call->bind.vertex_buffers[1], &range);
    push_command(&sim.command_stack, DRAW_CALL_COMMAND, call);
    
    free(vb->vertices);
    free(vb->instances);
    free(vb);
    sim.state.vbuffer = NULL;
}

void sim_commit(void) {
    sim_command_t *head = NULL;
    while ((head = pop_command(&sim.command_stack))) {
        switch (head->type) {
            case VIEWPORT_COMMAND:;
                sim_rect_t *vr = (sim_rect_t*)head->data;
                sg_apply_viewport(vr->x, vr->y, vr->w, vr->h, 1);
                break;
            case SCISSOR_RECT_COMMAND:;
                sim_rect_t *sr = (sim_rect_t*)head->data;
                sg_apply_scissor_rect(sr->x, sr->y, sr->w, sr->h, 1);
                break;
            case DRAW_CALL_COMMAND:;
                sim_draw_call_t *call = (sim_draw_call_t*)head->data;
                sg_begin_default_pass(&call->pass_action, sapp_width(), sapp_height());
                sg_apply_pipeline(call->pip);
                sg_apply_bindings(&call->bind);
                vs_params_t vs_params;
                memcpy(&vs_params.texture_matrix, &sim.state.texture, 16 * sizeof(float));
                memcpy(&vs_params.projection_matrix, &sim.state.projection, 16 * sizeof(float));
                sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &SG_RANGE(vs_params));
                sg_draw(0, call->vcount, call->icount);
                sg_end_pass();
                sg_destroy_pipeline(call->pip);
                sg_destroy_buffer(call->bind.vertex_buffers[0]);
                sg_destroy_buffer(call->bind.vertex_buffers[1]);
                break;
            default:
                abort(); // unknown command
        }
        free(head->data);
        free(head);
    }
    sg_commit();
}
