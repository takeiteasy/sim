/* sim.h -- https://github.com/takeiteasy/sim
 
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

#ifndef SIM_H
#define SIM_H
#include "sokol_gfx.h"

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(__EMSCRIPTEN__) || defined(EMSCRIPTEN)
#include <emscripten.h>
#define SIM_EMSCRIPTEN
#endif

#define SIM_POSIX
#if defined(macintosh) || defined(Macintosh) || (defined(__APPLE__) && defined(__MACH__))
#define SIM_MAC
#elif defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__WINDOWS__)
#define SIM_WINDOWS
#if !defined(SIM_FORCE_POSIX)
#undef SIM_POSIX
#endif
#elif defined(__gnu_linux__) || defined(__linux__) || defined(__unix__)
#define SIM_LINUX
#else
#error "Unsupported operating system"
#endif

#if defined(SIM_WINDOWS) && !defined(SIM_NO_EXPORT)
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

typedef enum {
    KEY_INVALID          = 0,
    KEY_SPACE            = 32,
    KEY_APOSTROPHE       = 39,  /* ' */
    KEY_COMMA            = 44,  /* , */
    KEY_MINUS            = 45,  /* - */
    KEY_PERIOD           = 46,  /* . */
    KEY_SLASH            = 47,  /* / */
    KEY_0                = 48,
    KEY_1                = 49,
    KEY_2                = 50,
    KEY_3                = 51,
    KEY_4                = 52,
    KEY_5                = 53,
    KEY_6                = 54,
    KEY_7                = 55,
    KEY_8                = 56,
    KEY_9                = 57,
    KEY_SEMICOLON        = 59,  /* ; */
    KEY_EQUAL            = 61,  /* = */
    KEY_A                = 65,
    KEY_B                = 66,
    KEY_C                = 67,
    KEY_D                = 68,
    KEY_E                = 69,
    KEY_F                = 70,
    KEY_G                = 71,
    KEY_H                = 72,
    KEY_I                = 73,
    KEY_J                = 74,
    KEY_K                = 75,
    KEY_L                = 76,
    KEY_M                = 77,
    KEY_N                = 78,
    KEY_O                = 79,
    KEY_P                = 80,
    KEY_Q                = 81,
    KEY_R                = 82,
    KEY_S                = 83,
    KEY_T                = 84,
    KEY_U                = 85,
    KEY_V                = 86,
    KEY_W                = 87,
    KEY_X                = 88,
    KEY_Y                = 89,
    KEY_Z                = 90,
    KEY_LEFT_BRACKET     = 91,  /* [ */
    KEY_BACKSLASH        = 92,  /* \ */
    KEY_RIGHT_BRACKET    = 93,  /* ] */
    KEY_GRAVE_ACCENT     = 96,  /* ` */
    KEY_WORLD_1          = 161, /* non-US #1 */
    KEY_WORLD_2          = 162, /* non-US #2 */
    KEY_ESCAPE           = 256,
    KEY_ENTER            = 257,
    KEY_TAB              = 258,
    KEY_BACKSPACE        = 259,
    KEY_INSERT           = 260,
    KEY_DELETE           = 261,
    KEY_RIGHT            = 262,
    KEY_LEFT             = 263,
    KEY_DOWN             = 264,
    KEY_UP               = 265,
    KEY_PAGE_UP          = 266,
    KEY_PAGE_DOWN        = 267,
    KEY_HOME             = 268,
    KEY_END              = 269,
    KEY_CAPS_LOCK        = 280,
    KEY_SCROLL_LOCK      = 281,
    KEY_NUM_LOCK         = 282,
    KEY_PRINT_SCREEN     = 283,
    KEY_PAUSE            = 284,
    KEY_F1               = 290,
    KEY_F2               = 291,
    KEY_F3               = 292,
    KEY_F4               = 293,
    KEY_F5               = 294,
    KEY_F6               = 295,
    KEY_F7               = 296,
    KEY_F8               = 297,
    KEY_F9               = 298,
    KEY_F10              = 299,
    KEY_F11              = 300,
    KEY_F12              = 301,
    KEY_F13              = 302,
    KEY_F14              = 303,
    KEY_F15              = 304,
    KEY_F16              = 305,
    KEY_F17              = 306,
    KEY_F18              = 307,
    KEY_F19              = 308,
    KEY_F20              = 309,
    KEY_F21              = 310,
    KEY_F22              = 311,
    KEY_F23              = 312,
    KEY_F24              = 313,
    KEY_F25              = 314,
    KEY_KP_0             = 320,
    KEY_KP_1             = 321,
    KEY_KP_2             = 322,
    KEY_KP_3             = 323,
    KEY_KP_4             = 324,
    KEY_KP_5             = 325,
    KEY_KP_6             = 326,
    KEY_KP_7             = 327,
    KEY_KP_8             = 328,
    KEY_KP_9             = 329,
    KEY_KP_DECIMAL       = 330,
    KEY_KP_DIVIDE        = 331,
    KEY_KP_MULTIPLY      = 332,
    KEY_KP_SUBTRACT      = 333,
    KEY_KP_ADD           = 334,
    KEY_KP_ENTER         = 335,
    KEY_KP_EQUAL         = 336,
    KEY_LEFT_SHIFT       = 340,
    KEY_LEFT_CONTROL     = 341,
    KEY_LEFT_ALT         = 342,
    KEY_LEFT_SUPER       = 343,
    KEY_RIGHT_SHIFT      = 344,
    KEY_RIGHT_CONTROL    = 345,
    KEY_RIGHT_ALT        = 346,
    KEY_RIGHT_SUPER      = 347,
    KEY_MENU             = 348,
} sim_key_t;

typedef enum {
    BUTTON_LEFT    = 0x0,
    BUTTON_RIGHT   = 0x1,
    BUTTON_MIDDLE  = 0x2,
    BUTTON_INVALID = 0x100,
} sim_button_t;

typedef enum {
    MODIFIER_SHIFT = 0x1,
    MODIFIER_CTRL  = 0x2,
    MODIFIER_ALT   = 0x4,
    MODIFIER_SUPER = 0x8,
    MODIFIER_LMB   = 0x100,
    MODIFIER_RMB   = 0x200,
    MODIFIER_MMB   = 0x400
} sim_modifier_t;

enum {
    SIM_MODELVIEW  = 0x0001,
    SIM_PROJECTION = 0x0002,
    SIM_TEXTURE    = 0x0003
};

enum {
    SIM_POINTS = 0x0001,
    SIM_LINES = 0x0002,
    SIM_LINE_STRIP = 0x0003,
    SIM_TRIANGLES = 0x0004,
    SIM_TRIANGLE_STRIP = 0x0005
};

enum {
    SIM_TEXTURE_2D = 0x0001,
    SIM_TEXTURE_3D = 0x0002
};

enum  {
    SIM_INVALID,
    SIM_FLOAT,
    SIM_FLOAT2,
    SIM_FLOAT3,
    SIM_FLOAT4,
    SIM_BYTE4,
    SIM_BYTE4N,
    SIM_UBYTE4,
    SIM_UBYTE4N,
    SIM_SHORT2,
    SIM_SHORT2N,
    SIM_USHORT2N,
    SIM_SHORT4,
    SIM_SHORT4N,
    SIM_USHORT4N,
    SIM_UINT10_N2,
    SIM_HALF2,
    SIM_HALF4,
    SIM_NUM
};

enum {
    SIM_CMP_DEFAULT,
    SIM_CMP_NEVER,
    SIM_CMP_LESS,
    SIM_CMP_EQUAL,
    SIM_CMP_LESS_EQUAL,
    SIM_CMP_GREATER,
    SIM_CMP_NOT_EQUAL,
    SIM_CMP_GREATER_EQUAL,
    SIM_CMP_ALWAYS,
    SIM_CMP_NUM,
};

enum {
    SIM_BLEND_NONE = 0,
    SIM_BLEND_BLEND,
    SIM_BLEND_ADD,
    SIM_BLEND_MOD,
    SIM_BLEND_MUL,
};

#define SIM_CALLBACKS        \
    X(init,         (void*)) \
    X(frame,        (void*)) \
    X(exit,         (void*))

EXPORT void sim_set_userdata(void *userdata);
#define X(NAME, ARGS) void(*NAME##_callback)ARGS,
EXPORT void sim_set_callbacks(SIM_CALLBACKS void *userdata);
#undef X
#define X(NAME, ARGS) \
EXPORT void sim_set_##NAME##_callback(void(*NAME##_callback)ARGS);
SIM_CALLBACKS
#undef X
EXPORT int sim_run(int argc, const char *argv[], void(*loop)(float));

EXPORT int sim_window_width(void);
EXPORT int sim_window_height(void);
EXPORT int sim_is_window_fullscreen(void);
EXPORT void sim_toggle_fullscreen(void);
EXPORT int sim_is_cursor_visible(void);
EXPORT void sim_toggle_cursor_visible(void);
EXPORT int sim_is_cursor_locked(void);
EXPORT void sim_toggle_cursor_lock(void);

EXPORT int sim_is_key_down(sim_key_t key);
EXPORT int sim_is_key_held(sim_key_t key);
EXPORT int sim_was_key_pressed(sim_key_t key);
EXPORT int sim_was_key_released(sim_key_t key);
EXPORT int sim_are_keys_down(int n, ...);
EXPORT int sim_any_keys_down(int n, ...);
EXPORT int sim_is_button_down(sim_button_t button);
EXPORT int sim_is_button_held(sim_button_t button);
EXPORT int sim_was_button_pressed(sim_button_t button);
EXPORT int sim_was_button_released(sim_button_t button);
EXPORT int sim_are_buttons_down(int n, ...);
EXPORT int sim_any_buttons_down(int n, ...);
EXPORT int sim_has_mouse_move(void);
EXPORT int sim_cursor_x(void);
EXPORT int sim_cursor_y(void);
EXPORT int sim_cursor_delta_x(void);
EXPORT int sim_cursor_delta_y(void);
EXPORT int sim_has_wheel_moved(void);
EXPORT float sim_scroll_x(void);
EXPORT float sim_scroll_y(void);

EXPORT void* sim_current_state(void);
EXPORT void* sim_pop_state(void);
EXPORT void sim_push_state(void *state);

EXPORT void sim_matrix_mode(int mode);
EXPORT void sim_push_matrix(void);
EXPORT void sim_pop_matrix(void);
EXPORT void sim_load_identity(void);
EXPORT void sim_mul_matrix(const float *mat);
EXPORT void sim_translate(float x, float y, float z);
EXPORT void sim_rotate(float angle, float x, float y, float z);
EXPORT void sim_scale(float x, float y, float z);
EXPORT void sim_frustum(double left, double right, double bottom, double top, double znear, double zfar);
EXPORT void sim_ortho(double left, double right, double bottom, double top, double znear, double zfar);
EXPORT void sim_look_at(float eyeX, float eyeY, float eyeZ, float targetX, float targetY, float targetZ, float upX, float upY, float upZ);

EXPORT void sim_begin(int mode);
EXPORT void sim_end(void);
EXPORT void sim_commit(void);

EXPORT int sim_empty_texture(int width, int height);
EXPORT int sim_texture_from_file(const char *path);
EXPORT int sim_texture_from_memory(unsigned char *data, size_t size_of_data);
EXPORT int sim_check_texture(int texture);
EXPORT void sim_release_texture(int texture);

#undef EXPORT

#if defined(__cplusplus)
}
#endif
#endif
