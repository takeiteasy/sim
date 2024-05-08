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
};

enum {
    BUTTON_LEFT    = 0x0,
    BUTTON_RIGHT   = 0x1,
    BUTTON_MIDDLE  = 0x2,
    BUTTON_INVALID = 0x100,
};

enum {
    MODIFIER_SHIFT = 0x1,
    MODIFIER_CTRL  = 0x2,
    MODIFIER_ALT   = 0x4,
    MODIFIER_SUPER = 0x8,
    MODIFIER_LMB   = 0x100,
    MODIFIER_RMB   = 0x200,
    MODIFIER_MMB   = 0x400
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
    SIM_TEXTURE_2D = 0x0001,
    SIM_TEXTURE_3D = 0x0002
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
    SIM_BLEND_NONE = 0,
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
EXPORT void sim_flush(void);
EXPORT void sim_end(void);

#undef EXPORT

#if defined(__cplusplus)
}
#endif
#endif
