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

#include "sim.h"
#define SOKOL_IMPL
#define SOKOL_NO_ENTRY
#include "sokol/sokol_gfx.h"
#include "sokol/sokol_app.h"
#include "sokol/sokol_glue.h"
#include "sokol/sokol_time.h"
#include "sokol/sokol_log.h"
#define HANDMADE_MATH_IMPLEMENTATION
#define HANDMADE_MATH_NO_SSE
#include "HandmadeMath.h"
#include "sim.glsl.h"
#define QOI_IMPLEMENTATION
#include "qoi.h"
#define STB_IMAGE_IMPLEMENTATION
#define STB_NO_GIF
#include "stb_image.h"

#if defined(SIM_WINDOWS)
#include <windows.h>
#include <io.h>
#include <dirent.h>
#define F_OK 0
#define access _access
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
#else
#include <unistd.h>
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
    int down;
    uint64_t timestamp;
} sim_input_state_t;

typedef struct {
    sim_input_state_t keys[SIM_KEY_MENU+1];
    sim_input_state_t buttons[3];
    int modifier;
    struct {
        int x, y;
    } cursor;
    struct {
        float x, y;
    } scroll;
} sim_input_t;

typedef struct {
    hmm_mat4 stack[MAX_MATRIX_STACK];
    int count;
} sim_matrix_stack_t;

typedef struct {
    hmm_vec4 x, y, z, w;
} sim_vs_inst_t;

typedef struct {
    hmm_vec4 position;
    hmm_vec3 normal;
    hmm_vec2 texcoord;
    hmm_vec4 color;
} sim_vertex_t;

enum {
    SIM_CMD_VIEWPORT,
    SIM_CMD_SCISSOR_RECT,
    SIM_CMD_DRAW_CALL
};

typedef struct {
    float x, y, w, h;
} sim_rect_t;

typedef struct {
    sim_vertex_t *vertices;
    int vcount;
    sim_vs_inst_t *instances;
    int icount;
    hmm_mat4 projection, texture_matrix;
    sg_pipeline pip;
    sg_bindings bind;
} sim_draw_call_t;

typedef struct {
    sim_matrix_stack_t matrix_stack[SIM_MATRIXMODE_COUNT];
    int matrix_mode;
    sg_color clear_color;
    sim_draw_call_t draw_call;
    sim_vertex_t current_vertex;
    sg_pipeline_desc pip_desc;
    sg_blend_state blend;
    int blend_mode;
    sg_image current_texture;
} sim_state_t;

typedef struct sim_command_t {
    void *data;
    int type;
    struct sim_command_t *next;
} sim_command_t;

typedef struct {
    sim_command_t *head, *tail;
} sim_command_queue_t;

static struct sim_t {
    int running;
    int mouse_hidden;
    int mouse_locked;
    sapp_desc app_desc;
    void(*init)(void);
    void(*loop)(double);
    void(*deinit)(void);
    void *userdata;
    sim_input_t current_input;
    sim_input_t last_input;
    sim_state_t state;
    sim_command_queue_t commands;
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

static hmm_mat4* sim_matrix_stack_head(int mode) {
    assert(mode >= 0 && mode < SIM_MATRIXMODE_COUNT);
    sim_matrix_stack_t *stack = &sim.state.matrix_stack[mode];
    return stack->count ? &stack->stack[stack->count-1] : NULL;
}

static hmm_mat4* sim_current_matrix(void) {
    return sim_matrix_stack_head(sim.state.matrix_mode);
}

static void sim_set_current_matrix(hmm_mat4 mat) {
    hmm_mat4 *head = sim_current_matrix();
    *head = mat;
}

static void sim_push_vertex(void) {
    sim.state.draw_call.vertices = realloc(sim.state.draw_call.vertices, ++sim.state.draw_call.vcount * sizeof(sim_vertex_t));
    memcpy(&sim.state.draw_call.vertices[sim.state.draw_call.vcount-1], &sim.state.current_vertex, sizeof(sim_vertex_t));
}

static void sim_push_command(int type, void *data) {
    sim_command_t *command = malloc(sizeof(sim_command_t));
    command->type = type;
    command->data = data;
    command->next = NULL;
    
    if (!sim.commands.head && !sim.commands.tail)
        sim.commands.head = sim.commands.tail = command;
    else {
        sim.commands.tail->next = command;
        sim.commands.tail = command;
    }
}

static void init(void) {
    sg_desc desc = {
        .environment = sglue_environment(),
        .logger.func = slog_func,
        .buffer_pool_size = 256
    };
    sg_setup(&desc);
    stm_setup();
    if (sim.init)
        sim.init();
    
    sim.shader = sg_make_shader(sim_shader_desc(sg_query_backend()));
    sim.state.pip_desc = (sg_pipeline_desc) {
        .layout = {
            .buffers[1].step_func = SG_VERTEXSTEP_PER_INSTANCE,
            .attrs = {
                [ATTR_vs_position] = { .format=SG_VERTEXFORMAT_FLOAT4, .buffer_index=0 },
                [ATTR_vs_normal] = { .format=SG_VERTEXFORMAT_FLOAT3, .buffer_index=0 },
                [ATTR_vs_texcoord] = { .format=SG_VERTEXFORMAT_FLOAT2, .buffer_index=0 },
                [ATTR_vs_color_v] = { .format=SG_VERTEXFORMAT_FLOAT4, .buffer_index=0 },
                [ATTR_vs_inst_mat_x] = { .format=SG_VERTEXFORMAT_FLOAT4, .buffer_index=1 },
                [ATTR_vs_inst_mat_y] = { .format=SG_VERTEXFORMAT_FLOAT4, .buffer_index=1 },
                [ATTR_vs_inst_mat_z] = { .format=SG_VERTEXFORMAT_FLOAT4, .buffer_index=1 },
                [ATTR_vs_inst_mat_w] = { .format=SG_VERTEXFORMAT_FLOAT4, .buffer_index=1 }
            }
        },
        .shader = sim.shader,
        .cull_mode = SG_CULLMODE_BACK,
        .depth = {
            .compare = SG_COMPAREFUNC_LESS_EQUAL,
            .write_enabled = true
        }
    };
    
    for (int i = 0; i < SIM_MATRIXMODE_COUNT; i++) {
        sim_matrix_stack_t *stack = &sim.state.matrix_stack[i];
        stack->count = 1;
        stack->stack[0] = HMM_Mat4();
    }
}

static void frame(void) {
    const float t = (float)(sapp_frame_duration() * 60.);
    sim.loop(t);

    sg_begin_pass(&(sg_pass) {
        .action = {
            .colors[0] = {
                .load_action = SG_LOADACTION_CLEAR,
                .clear_value = sim.state.clear_color
            }
        },
        .swapchain = sglue_swapchain()
    });
    sim_command_t *cursor = sim.commands.head;
    while (cursor) {
        sim_command_t *tmp = cursor->next;
        switch (cursor->type) {
            case SIM_CMD_VIEWPORT:
            case SIM_CMD_SCISSOR_RECT:;
                sim_rect_t *rect = (sim_rect_t*)cursor->data;
                if (cursor->type == SIM_CMD_VIEWPORT)
                    sim_viewport(rect->x, rect->y, rect->w, rect->h);
                else
                    sim_scissor_rect(rect->x, rect->y, rect->w, rect->h);
                break;
            case SIM_CMD_DRAW_CALL:;
                sim_draw_call_t *call = (sim_draw_call_t*)cursor->data;
                sg_apply_pipeline(call->pip);
                sg_apply_bindings(&call->bind);
                vs_params_t vs_params;
                vs_params.texture_matrix = call->texture_matrix;
                vs_params.projection = call->projection;
                sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &SG_RANGE(vs_params));
                sg_draw(0, call->vcount, call->icount);
                sg_destroy_buffer(call->bind.vertex_buffers[0]);
                sg_destroy_buffer(call->bind.vertex_buffers[1]);
                sg_destroy_pipeline(call->pip);
                break;
            default:
                abort();
        }
        free(cursor->data);
        free(cursor);
        cursor = tmp;
    }
    sim.commands.head = sim.commands.tail = NULL;
    sg_end_pass();
    sg_commit();
    
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
    if (sim.deinit)
        sim.deinit();
    sg_shutdown();
}

int sim_run(int window_width,
            int window_height,
            const char *title,
            void(*_init)(void),
            void(*loop)(double),
            void(*deinit)(void)) {
    assert(!sim.running);
    sim.init = _init;
    sim.deinit = deinit;
    assert((sim.loop = loop));
    sim.app_desc.width = window_width > 0 ? window_width : DEFAULT_WINDOW_WIDTH;
    sim.app_desc.height = window_height > 0 ? window_height : DEFAULT_WINDOW_HEIGHT;
    sim.app_desc.window_title = title ? title : DEFAULT_WINDOW_TITLE;
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
        case SIM_MATRIXMODE_PROJECTION:
        case SIM_MATRIXMODE_TEXTURE:
        case SIM_MATRIXMODE_MODELVIEW:
            sim.state.matrix_mode = mode;
            break;
        default:
            abort(); // unknown mode
    }
}

void sim_push_matrix(void) {
    sim_matrix_stack_t *cs = &sim.state.matrix_stack[sim.state.matrix_mode];
    assert(cs->count + 1 < MAX_MATRIX_STACK);
    cs->stack[cs->count] = cs->stack[cs->count-1];
    cs->count++;
}

void sim_pop_matrix(void) {
    sim_matrix_stack_t *cs = &sim.state.matrix_stack[sim.state.matrix_mode];
    if (cs->count == 1)
        memset(&cs->stack, 0, MAX_MATRIX_STACK * sizeof(hmm_mat4));
    else
        cs->stack[--cs->count] = HMM_Mat4();
}

void sim_load_identity(void) {
    sim_set_current_matrix(HMM_Mat4d(1.f));
}

void sim_mul_matrix(const float *matf) {
    hmm_mat4 right;
    memcpy(&right, matf, sizeof(float) * 16);
    hmm_m4 result = HMM_MultiplyMat4(*sim_current_matrix(), right);
    sim_set_current_matrix(result);
}

#define SIM_MUL_MATRIX(M) sim_mul_matrix((float*)&(M))

void sim_translate(float x, float y, float z) {
    hmm_mat4 mat = HMM_Translate(HMM_Vec3(x, y, z));
    SIM_MUL_MATRIX(mat);
}

void sim_rotate(float angle, float x, float y, float z) {
    hmm_mat4 mat = HMM_Rotate(angle, HMM_Vec3(x, y, z));
    SIM_MUL_MATRIX(mat);
}

void sim_scale(float x, float y, float z) {
    hmm_mat4 mat = HMM_Scale(HMM_Vec3(x, y, z));
    SIM_MUL_MATRIX(mat);
}

void sim_ortho(double left, double right, double bottom, double top, double znear, double zfar) {
    hmm_mat4 mat = HMM_Orthographic(left, right, bottom, top, znear, zfar);
    SIM_MUL_MATRIX(mat);
}

void sim_perspective(float fovy, float aspect_ratio, float znear, float zfar) {
    hmm_mat4 mat = HMM_Perspective(fovy, aspect_ratio, znear, zfar);
    SIM_MUL_MATRIX(mat);
}

void sim_look_at(float eyeX, float eyeY, float eyeZ,
                 float targetX, float targetY, float targetZ,
                 float upX, float upY, float upZ) {
    hmm_mat4 mat = HMM_LookAt(HMM_Vec3(eyeX, eyeY, eyeZ),
                              HMM_Vec3(targetX, targetY, targetZ),
                              HMM_Vec3(upX, upY, upZ));
    SIM_MUL_MATRIX(mat);
}

void sim_clear_color(float r, float g, float b, float a) {
    sim.state.clear_color = (sg_color){r, g, b, a};
}

void sim_viewport(int x, int y, int width, int height) {
    sim_rect_t *rect = malloc(sizeof(sim_rect_t));
    rect->x = x;
    rect->y = y;
    rect->w = width;
    rect->h = height;
    sim_push_command(SIM_CMD_VIEWPORT, rect);
}

void sim_scissor_rect(int x, int y, int width, int height) {
    sim_rect_t *rect = malloc(sizeof(sim_rect_t));
    rect->x = x;
    rect->y = y;
    rect->w = width;
    rect->h = height;
    sim_push_command(SIM_CMD_SCISSOR_RECT, rect);
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
    assert(!sim.state.draw_call.vertices && !sim.state.draw_call.instances);
    if (sim.state.draw_call.vertices)
        sim_end();
    sim.state.draw_call.vertices = malloc(0);
    sim.state.draw_call.vcount = 0;
    sim.state.draw_call.instances = malloc(0);
    sim.state.draw_call.icount = 0;
    switch (mode) {
        default:
            mode = SIM_DRAW_TRIANGLES;
        case SIM_DRAW_TRIANGLES:
        case SIM_DRAW_POINTS:
        case SIM_DRAW_LINES:
        case SIM_DRAW_LINE_STRIP:
        case SIM_DRAW_TRIANGLE_STRIP:
            sim.state.pip_desc.primitive_type = mode;
            break;
    }
}

void sim_vertex2i(int x, int y) {
    sim_vertex3f((float)x, (float)y, 0.f);
}

void sim_vertex2f(float x, float y) {
    sim_vertex3f(x, y, 0.f);
}

void sim_vertex3f(float x, float y, float z) {
    sim.state.current_vertex.position = HMM_Vec4(x, y, z, 1.f);
    sim_push_vertex();
}

void sim_texcoord2f(float x, float y) {
    sim.state.current_vertex.texcoord = HMM_Vec2(x, y);
}

void sim_normal3f(float x, float y, float z) {
    sim.state.current_vertex.normal = HMM_Vec3(x, y, z);
}

void sim_color4ub(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    sim.state.current_vertex.color = HMM_Vec4((float)r / 255.f,
                                              (float)g / 255.f,
                                              (float)b / 255.f,
                                              (float)a / 255.f);
}

void sim_color3f(float x, float y, float z) {
    sim.state.current_vertex.color = HMM_Vec4(x, y, z, 1.f);
}

void sim_color4f(float x, float y, float z, float w) {
    sim.state.current_vertex.color = HMM_Vec4(x, y, z, w);
}

void sim_draw(void) {
    sim.state.draw_call.instances = realloc(sim.state.draw_call.instances, ++sim.state.draw_call.icount * sizeof(sim_vs_inst_t));
    sim_vs_inst_t *inst = &sim.state.draw_call.instances[sim.state.draw_call.icount-1];
    hmm_mat4 *m = sim_matrix_stack_head(SIM_MATRIXMODE_MODELVIEW);
    hmm_mat4 model = m ? *m : HMM_Mat4();
    inst->x = HMM_Vec4(model.Elements[0][0],
                       model.Elements[1][0],
                       model.Elements[2][0],
                       model.Elements[3][0]);
    inst->y = HMM_Vec4(model.Elements[0][1],
                       model.Elements[1][1],
                       model.Elements[2][1],
                       model.Elements[3][1]);
    inst->z = HMM_Vec4(model.Elements[0][2],
                       model.Elements[1][2],
                       model.Elements[2][2],
                       model.Elements[3][2]);
    inst->w = HMM_Vec4(model.Elements[0][3],
                       model.Elements[1][3],
                       model.Elements[2][3],
                       model.Elements[3][3]);
}

void sim_end(void) {
    assert(sim.state.draw_call.vertices && sim.state.draw_call.vcount);
    assert(sim.state.draw_call.instances && sim.state.draw_call.icount);
    
    sg_buffer_desc b0 = {
        .data = (sg_range) {
            .ptr = sim.state.draw_call.vertices,
            .size = sim.state.draw_call.vcount * sizeof(sim_vertex_t)
        }
    };
    sg_buffer_desc b1 = {
        .size = sizeof(sim_vs_inst_t),
        .usage = SG_USAGE_STREAM
    };
    sim.state.draw_call.bind = (sg_bindings) {
        .vertex_buffers[0] = sg_make_buffer(&b0),
        .vertex_buffers[1] = sg_make_buffer(&b1),
    };
    sg_range r0 = {
        .ptr = sim.state.draw_call.instances,
        .size = sim.state.draw_call.icount * sizeof(sim_vs_inst_t)
    };
    sg_update_buffer(sim.state.draw_call.bind.vertex_buffers[1], &r0);
    
    sim.state.draw_call.pip = sg_make_pipeline(&sim.state.pip_desc);
    sim.state.draw_call.projection = *sim_matrix_stack_head(SIM_MATRIXMODE_PROJECTION);
    sim.state.draw_call.texture_matrix = *sim_matrix_stack_head(SIM_MATRIXMODE_TEXTURE);
    
    sim_draw_call_t *draw_call = malloc(sizeof(sim_draw_call_t));
    memcpy(draw_call, &sim.state.draw_call, sizeof(sim_draw_call_t));
    sim_push_command(SIM_CMD_DRAW_CALL, draw_call);
    
    free(sim.state.draw_call.vertices);
    sim.state.draw_call.vcount = 0;
    free(sim.state.draw_call.instances);
    sim.state.draw_call.icount = 0;
    sg_pipeline tmp = sim.state.draw_call.pip;
    memset(&sim.state.draw_call, 0, sizeof(sim_draw_call_t));
    sim.state.draw_call.pip = tmp;
}

int sim_empty_texture(int width, int height) {
    assert(width && height);
    sg_image_desc desc = {
        .width = width,
        .height = height
    };
    return sg_make_image(&desc).id;
}

void sim_push_texture(int texture) {
    sg_image tmp = {.id = texture};
    assert(sg_query_image_state(tmp) == SG_RESOURCESTATE_VALID);
    sim.state.current_texture = tmp;
}

static int does_file_exist(const char *path) {
    return !access(path, F_OK);
}

static const char* file_extension(const char *path) {
    const char *dot = strrchr(path, '.');
    return !dot || dot == path ? NULL : dot + 1;
}

void sim_load_texture_path(const char *path) {
    assert(sg_query_image_state(sim.state.current_texture) == SG_RESOURCESTATE_VALID);
    assert(does_file_exist(path));
#define VALID_EXTS_SZ 11
    static const char *valid_extensions[VALID_EXTS_SZ] = {
        "jpg", "jpeg", "png", "bmp", "psd", "tga", "hdr", "pic", "ppm", "pgm", "qoi"
    };
    const char *ext = file_extension(path);
    unsigned long ext_length = strlen(ext);
    char *dup = strdup(ext);
    for (int i = 0; i < ext_length; i++)
        if (dup[i] >= 'A' && dup[i] <= 'Z')
            dup[i] += 32;
    int found = 0;
    for (int i = 0; i < VALID_EXTS_SZ; i++) {
        if (!strncmp(dup, valid_extensions[i], ext_length)) {
            found = 1;
            goto FOUND;
        }
    }
    
FOUND:
    free(dup);
    if (!found)
        return;
    
    size_t sz = -1;
    FILE *fh = fopen(path, "rb");
    assert(fh);
    fseek(fh, 0, SEEK_END);
    sz = ftell(fh);
    fseek(fh, 0, SEEK_SET);
    
    unsigned char *data = malloc(sz * sizeof(unsigned char));
    fread(data, sz, 1, fh);
    fclose(fh);
    sim_load_texture_memory(data, (int)sz);
    free(data);
}

#define QOI_MAGIC (((unsigned int)'q') << 24 | ((unsigned int)'o') << 16 | ((unsigned int)'i') <<  8 | ((unsigned int)'f'))

static int check_if_qoi(unsigned char *data) {
    return (data[0] << 24 | data[1] << 16 | data[2] << 8 | data[3]) == QOI_MAGIC;
}

#define RGBA(R, G, B, A) (((unsigned int)(A) << 24) | ((unsigned int)(R) << 16) | ((unsigned int)(G) << 8) | (B))

static int* load_texture_data(unsigned char *data, int data_size, int *w, int *h) {
    assert(data && data_size);
    int _w, _h, c;
    unsigned char *in = NULL;
    if (check_if_qoi(data)) {
        qoi_desc desc;
        in = qoi_decode(data, data_size, &desc, 4);
        _w = desc.width;
        _h = desc.height;
    } else
        in = stbi_load_from_memory(data, data_size, &_w, &_h, &c, 4);
    assert(in && _w && _h);
    
    int *buf = malloc(_w * _h * sizeof(int));
    for (int x = 0; x < _w; x++)
        for (int y = 0; y < _h; y++) {
            unsigned char *p = in + (x + _w * y) * 4;
            buf[y * _w + x] = RGBA(p[0], p[1], p[2], p[3]);
        }
    free(in);
    if (w)
        *w = _w;
    if (h)
        *h = _h;
    return buf;
}

void sim_load_texture_memory(unsigned char *data, int data_size) {
    assert(data && data_size);
    assert(sg_query_image_state(sim.state.current_texture) == SG_RESOURCESTATE_VALID);
    
    int w, h;
    int *tmp = load_texture_data(data, data_size, &w, &h);
    assert(tmp && w && h);
    sg_image_data desc = {
        .subimage[0][0] = (sg_range) {
            .ptr = tmp,
            .size = w * h * sizeof(int)
        }
    };
    sg_update_image(sim.state.current_texture, &desc);
    free(tmp);
}

void sim_release_texture(int texture) {
    sg_image tmp = {.id = texture};
    if (sg_query_image_state(tmp) == SG_RESOURCESTATE_VALID)
        sg_destroy_image(tmp);
}
