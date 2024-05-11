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

// Taken from: https://gist.github.com/61131/7a22ac46062ee292c2c8bd6d883d28de
#define N_ARGS(...) _NARG_(__VA_ARGS__, _RSEQ())
#define _NARG_(...) _SEQ(__VA_ARGS__)
#define _SEQ(_1, _2, _3, _4, _5, _6, _7, _8, _9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55,_56,_57,_58,_59,_60,_61,_62,_63,_64,_65,_66,_67,_68,_69,_70,_71,_72,_73,_74,_75,_76,_77,_78,_79,_80,_81,_82,_83,_84,_85,_86,_87,_88,_89,_90,_91,_92,_93,_94,_95,_96,_97,_98,_99,_100,_101,_102,_103,_104,_105,_106,_107,_108,_109,_110,_111,_112,_113,_114,_115,_116,_117,_118,_119,_120,_121,_122,_123,_124,_125,_126,_127,N,...) N
#define _RSEQ() 127,126,125,124,123,122,121,120,119,118,117,116,115,114,113,112,111,110,109,108,107,106,105,104,103,102,101,100,99,98,97,96,95,94,93,92,91,90,89,88,87,86,85,84,83,82,81,80,79,78,77,76,75,74,73,72,71,70,69,68,67,66,65,64,63,62,61,60,59,58,57,56,55,54,53,52,51,50,49,48,47,46,45,44,43,42,41,40,39,38,37,36,35,34,33,32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0

enum {
    SIM_KEY_INVALID          = 0,
    SIM_KEY_SPACE            = 32,
    SIM_KEY_APOSTROPHE       = 39,  /* ' */
    SIM_KEY_COMMA            = 44,  /* , */
    SIM_KEY_MINUS            = 45,  /* - */
    SIM_KEY_PERIOD           = 46,  /* . */
    SIM_KEY_SLASH            = 47,  /* / */
    SIM_KEY_0                = 48,
    SIM_KEY_1                = 49,
    SIM_KEY_2                = 50,
    SIM_KEY_3                = 51,
    SIM_KEY_4                = 52,
    SIM_KEY_5                = 53,
    SIM_KEY_6                = 54,
    SIM_KEY_7                = 55,
    SIM_KEY_8                = 56,
    SIM_KEY_9                = 57,
    SIM_KEY_SEMICOLON        = 59,  /* ; */
    SIM_KEY_EQUAL            = 61,  /* = */
    SIM_KEY_A                = 65,
    SIM_KEY_B                = 66,
    SIM_KEY_C                = 67,
    SIM_KEY_D                = 68,
    SIM_KEY_E                = 69,
    SIM_KEY_F                = 70,
    SIM_KEY_G                = 71,
    SIM_KEY_H                = 72,
    SIM_KEY_I                = 73,
    SIM_KEY_J                = 74,
    SIM_KEY_K                = 75,
    SIM_KEY_L                = 76,
    SIM_KEY_M                = 77,
    SIM_KEY_N                = 78,
    SIM_KEY_O                = 79,
    SIM_KEY_P                = 80,
    SIM_KEY_Q                = 81,
    SIM_KEY_R                = 82,
    SIM_KEY_S                = 83,
    SIM_KEY_T                = 84,
    SIM_KEY_U                = 85,
    SIM_KEY_V                = 86,
    SIM_KEY_W                = 87,
    SIM_KEY_X                = 88,
    SIM_KEY_Y                = 89,
    SIM_KEY_Z                = 90,
    SIM_KEY_LEFT_BRACKET     = 91,  /* [ */
    SIM_KEY_BACKSLASH        = 92,  /* \ */
    SIM_KEY_RIGHT_BRACKET    = 93,  /* ] */
    SIM_KEY_GRAVE_ACCENT     = 96,  /* ` */
    SIM_KEY_WORLD_1          = 161, /* non-US #1 */
    SIM_KEY_WORLD_2          = 162, /* non-US #2 */
    SIM_KEY_ESCAPE           = 256,
    SIM_KEY_ENTER            = 257,
    SIM_KEY_TAB              = 258,
    SIM_KEY_BACKSPACE        = 259,
    SIM_KEY_INSERT           = 260,
    SIM_KEY_DELETE           = 261,
    SIM_KEY_RIGHT            = 262,
    SIM_KEY_LEFT             = 263,
    SIM_KEY_DOWN             = 264,
    SIM_KEY_UP               = 265,
    SIM_KEY_PAGE_UP          = 266,
    SIM_KEY_PAGE_DOWN        = 267,
    SIM_KEY_HOME             = 268,
    SIM_KEY_END              = 269,
    SIM_KEY_CAPS_LOCK        = 280,
    SIM_KEY_SCROLL_LOCK      = 281,
    SIM_KEY_NUM_LOCK         = 282,
    SIM_KEY_PRINT_SCREEN     = 283,
    SIM_KEY_PAUSE            = 284,
    SIM_KEY_F1               = 290,
    SIM_KEY_F2               = 291,
    SIM_KEY_F3               = 292,
    SIM_KEY_F4               = 293,
    SIM_KEY_F5               = 294,
    SIM_KEY_F6               = 295,
    SIM_KEY_F7               = 296,
    SIM_KEY_F8               = 297,
    SIM_KEY_F9               = 298,
    SIM_KEY_F10              = 299,
    SIM_KEY_F11              = 300,
    SIM_KEY_F12              = 301,
    SIM_KEY_F13              = 302,
    SIM_KEY_F14              = 303,
    SIM_KEY_F15              = 304,
    SIM_KEY_F16              = 305,
    SIM_KEY_F17              = 306,
    SIM_KEY_F18              = 307,
    SIM_KEY_F19              = 308,
    SIM_KEY_F20              = 309,
    SIM_KEY_F21              = 310,
    SIM_KEY_F22              = 311,
    SIM_KEY_F23              = 312,
    SIM_KEY_F24              = 313,
    SIM_KEY_F25              = 314,
    SIM_KEY_KP_0             = 320,
    SIM_KEY_KP_1             = 321,
    SIM_KEY_KP_2             = 322,
    SIM_KEY_KP_3             = 323,
    SIM_KEY_KP_4             = 324,
    SIM_KEY_KP_5             = 325,
    SIM_KEY_KP_6             = 326,
    SIM_KEY_KP_7             = 327,
    SIM_KEY_KP_8             = 328,
    SIM_KEY_KP_9             = 329,
    SIM_KEY_KP_DECIMAL       = 330,
    SIM_KEY_KP_DIVIDE        = 331,
    SIM_KEY_KP_MULTIPLY      = 332,
    SIM_KEY_KP_SUBTRACT      = 333,
    SIM_KEY_KP_ADD           = 334,
    SIM_KEY_KP_ENTER         = 335,
    SIM_KEY_KP_EQUAL         = 336,
    SIM_KEY_LEFT_SHIFT       = 340,
    SIM_KEY_LEFT_CONTROL     = 341,
    SIM_KEY_LEFT_ALT         = 342,
    SIM_KEY_LEFT_SUPER       = 343,
    SIM_KEY_RIGHT_SHIFT      = 344,
    SIM_KEY_RIGHT_CONTROL    = 345,
    SIM_KEY_RIGHT_ALT        = 346,
    SIM_KEY_RIGHT_SUPER      = 347,
    SIM_KEY_MENU             = 348,
};

enum {
    SIM_BUTTON_LEFT    = 0x0,
    SIM_BUTTON_RIGHT   = 0x1,
    SIM_BUTTON_MIDDLE  = 0x2,
    SIM_BUTTON_INVALID = 0x100,
};

enum {
    SIM_MODIFIER_SHIFT = 0x1,
    SIM_MODIFIER_CTRL  = 0x2,
    SIM_MODIFIER_ALT   = 0x4,
    SIM_MODIFIER_SUPER = 0x8,
    SIM_MODIFIER_LMB   = 0x100,
    SIM_MODIFIER_RMB   = 0x200,
    SIM_MODIFIER_MMB   = 0x400
};

enum {
    SIM_MATRIXMODE_MODELVIEW  = 0,
    SIM_MATRIXMODE_PROJECTION,
    SIM_MATRIXMODE_TEXTURE,
    SIM_MATRIXMODE_COUNT
};

enum {
    SIM_DRAW_POINTS = 0x0001,
    SIM_DRAW_LINES = 0x0002,
    SIM_DRAW_LINE_STRIP = 0x0003,
    SIM_DRAW_TRIANGLES = 0x0004,
    SIM_DRAW_TRIANGLE_STRIP = 0x0005
};

enum {
    SIM_CMP_DEFAULT = 0,
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
    SIM_BLEND_DEFAULT = 0,
    SIM_BLEND_NONE,
    SIM_BLEND_BLEND,
    SIM_BLEND_ADD,
    SIM_BLEND_MOD,
    SIM_BLEND_MUL,
};

enum {
    SIM_CULL_DEFAULT = 0,
    SIM_CULL_NONE,
    SIM_CULL_FRONT,
    SIM_CULL_BACK
};

enum {
    SIM_FILTER_DEFAULT = 0,
    SIM_FILTER_NONE,
    SIM_FILTER_NEAREST,
    SIM_FILTER_LINEAR
};

enum {
    SIM_WRAP_DEFAULT = 0,
    SIM_WRAP_REPEAT,
    SIM_WRAP_CLAMP_TO_EDGE,
    SIM_WRAP_CLAMP_TO_BORDER,
    SIM_WRAP_MIRRORED_REPEAT
};

EXPORT int sim_run(int window_width,
                   int window_height,
                   const char *title,
                   void(*init)(void),
                   void(*loop)(double),
                   void(*deinit)(void));

EXPORT int sim_window_width(void);
EXPORT int sim_window_height(void);
EXPORT int sim_is_window_fullscreen(void);
EXPORT void sim_toggle_fullscreen(void);
EXPORT int sim_is_cursor_visible(void);
EXPORT void sim_toggle_cursor_visible(void);
EXPORT int sim_is_cursor_locked(void);
EXPORT void sim_toggle_cursor_lock(void);

EXPORT int sim_is_key_down(int key);
EXPORT int sim_is_key_held(int key);
EXPORT int sim_was_key_pressed(int key);
EXPORT int sim_was_key_released(int key);
EXPORT int sim_are_keys_down(int n, ...);
#define SIM_ARE_KEYS_DOWN(...) sim_are_keys_down(N_ARGS(__VA_ARGS__), __VA_ARGS__)
EXPORT int sim_any_keys_down(int n, ...);
#define SIM_ANY_KEYS_DOWN(...) sim_any_keys_down(N_ARGS(__VA_ARGS__), __VA_ARGS__)
EXPORT int sim_is_button_down(int button);
EXPORT int sim_is_button_held(int button);
EXPORT int sim_was_button_pressed(int button);
EXPORT int sim_was_button_released(int button);
EXPORT int sim_are_buttons_down(int n, ...);
#define SIM_ARE_BUTTONS_DOWN(...) sim_are_buttons_down(N_ARGS(__VA_ARGS__), __VA_ARGS__)
EXPORT int sim_any_buttons_down(int n, ...);
#define SIM_ANY_BUTTONS_DOWN(...) sim_any_buttons_down(N_ARGS(__VA_ARGS__), __VA_ARGS__)
EXPORT int sim_has_mouse_move(void);
EXPORT int sim_cursor_x(void);
EXPORT int sim_cursor_y(void);
EXPORT int sim_cursor_delta_x(void);
EXPORT int sim_cursor_delta_y(void);
EXPORT int sim_has_wheel_moved(void);
EXPORT float sim_scroll_x(void);
EXPORT float sim_scroll_y(void);

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
EXPORT void sim_perspective(float fovy, float aspect_ratio, float znear, float zfar);
EXPORT void sim_look_at(float eyeX, float eyeY, float eyeZ, float targetX, float targetY, float targetZ, float upX, float upY, float upZ);

EXPORT void sim_clear_color(float r, float g, float b, float a);
EXPORT void sim_viewport(int x, int y, int width, int height);
EXPORT void sim_scissor_rect(int x, int y, int width, int height);
EXPORT void sim_blend_mode(int mode);
EXPORT void sim_depth_func(int func);
EXPORT void sim_cull_mode(int mode);

EXPORT void sim_begin(int mode);
EXPORT void sim_vertex2i(int x, int y);
EXPORT void sim_vertex2f(float x, float y);
EXPORT void sim_vertex3f(float x, float y, float z);
EXPORT void sim_texcoord2f(float x, float y);
EXPORT void sim_normal3f(float x, float y, float z);
EXPORT void sim_color4ub(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
EXPORT void sim_color3f(float x, float y, float z);
EXPORT void sim_color4f(float x, float y, float z, float w);
EXPORT void sim_draw(void);
EXPORT void sim_end(void);

EXPORT int sim_empty_texture(int width, int height);
EXPORT void sim_push_texture(int texture);
EXPORT void sim_pop_texture(void);
EXPORT int sim_load_texture_path(const char *path);
EXPORT int sim_load_texture_memory(unsigned char *data, int data_size);
EXPORT void sim_set_texture_filter(int min, int mag);
EXPORT void sim_set_texture_wrap(int wrap_u, int wrap_v);
EXPORT void sim_release_texture(int texture);

#undef EXPORT

#if defined(__cplusplus)
}
#endif
#endif
