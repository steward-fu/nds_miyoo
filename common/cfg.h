//
//    NDS Emulator (DraStic) for Miyoo Handheld
//
//    This software is provided 'as-is', without any express or implied
//    warranty.  In no event will the authors be held liable for any damages
//    arising from the use of this software.
//
//    Permission is granted to anyone to use this software for any purpose,
//    including commercial applications, and to alter it and redistribute it
//    freely, subject to the following restrictions:
//
//    1. The origin of this software must not be misrepresented; you must not
//       claim that you wrote the original software. If you use this software
//       in a product, an acknowledgment in the product documentation would be
//       appreciated but is not required.
//    2. Altered source versions must be plainly marked as such, and must not be
//       misrepresented as being the original software.
//    3. This notice may not be removed or altered from any source distribution.
//

#ifndef __CFG_H__
#define __CFG_H__

#if defined(A30)
#define JSON_SYS_FOLDER                     "/config"
#endif

#if defined(MINI)
#define JSON_SYS_FOLDER                     "/appconfigs"
#endif

#if defined(UT)
#define JSON_SYS_FOLDER                     "."
#endif

#define JSON_SYS_FILE                       "system.json"
#define JSON_SYS_PATH                       JSON_SYS_FOLDER "/" JSON_SYS_FILE
#define JSON_SYS_VOLUME                     "vol"

#define JSON_CFG_FOLDER                     "miyoo"
#define JSON_CFG_FILE                       "settings.json"
#define JSON_CFG_PATH                       JSON_CFG_FOLDER "/" JSON_CFG_FILE
#define JSON_CFG_PEN_IMAGE                  "pen_image"
#define JSON_CFG_PEN_POSITION               "pen_position"
#define JSON_CFG_BORDER_IMAGE               "border_image"
#define JSON_CFG_SDL_VERSION                "sdl_version"
#define JSON_CFG_FONT_STYLE                 "font_style"
#define JSON_CFG_HALF_VOLUME                "half_volume"
#define JSON_CFG_DISPLAY_LAYOUT             "display_layout"
#define JSON_CFG_DISPLAY_LAYOUT_ALT         "display_layout_alt"
#define JSON_CFG_DISPLAY_LAYOUT_ALPHA       "display_layout_alpha"
#define JSON_CFG_DISPLAY_LAYOUT_BORDER      "display_layout_border"
#define JSON_CFG_DISPLAY_LAYOUT_POSITION    "display_layout_position"
#define JSON_CFG_MOVE_SPEED_X               "pen_move_speed_x"
#define JSON_CFG_MOVE_SPEED_Y               "pen_move_speed_y"
#define JSON_CFG_AUTO_SAVE_LOAD             "auto_save_load"
#define JSON_CFG_AUTO_SAVE_LOAD_SLOT        "auto_save_load_slot"
#define JSON_CFG_CPU_MAX_CORE               "cpu_max_core"
#define JSON_CFG_CPU_MAX_FREQ               "cpu_max_freq"
#define JSON_CFG_CPU_MIN_CORE               "cpu_min_core"
#define JSON_CFG_CPU_MIN_FREQ               "cpu_min_freq"
#define JSON_CFG_KEY_SWAP_L1_L2             "key_swap_l1_l2"
#define JSON_CFG_KEY_SWAP_R1_R2             "key_swap_r1_r2"

#if !defined(MAX_PATH)
#define MAX_PATH 128
#endif

#define CHECK_POINTER_AND_RETURN_IF_ERROR(_x_) \
if (!_x_) { \
    err(COM"invalid parameter in %s()\n", __func__); \
    return -1; \
}

#define MAX_AUTO_SAVE_LOAD_SLOT         10
#define MAX_DISPLAY_LAYOUT              18
#define MAX_DISPLAY_LAYOUT_ALT          18
#define MAX_DISPLAY_LAYOUT_ALPHA        10
#define MAX_DISPLAY_LAYOUT_BORDER       1
#define MAX_DISPLAY_LAYOUT_POSITION     3
#define MAX_PEN_POSITION                1
#define MAX_PEN_MOVE_SPEED              1000000
#define MAX_CPU_CORE                    4
#define MAX_CPU_FREQ                    3000
#define MIN_CPU_CORE                    1
#define MIN_CPU_FREQ                    300
#define CFG_SDL_VERSION                 "v2.0"
#define DEFAULT_FONT_STYLE              "font/simsun.ttf"
#define DEFAULT_PEN_IMAGE               "pen/4_lb.png"
#define DEFAULT_BORDER_IMAGE            "border/640x480/1/2.png"

struct json_config {
    int volume;
    int half_volume;
    char sdl_version[MAX_PATH];
    char font_style[MAX_PATH];
    char border_image[MAX_PATH];
    char json_path[MAX_PATH + 32];

    struct _display {
        int alt;
        int alpha;
        int layout;
        int border;
        int position;
    } display;

    struct _auto_save_load {
        int slot;
        int enable;
    } auto_save_load;

    struct _pen {
        struct _move_speed {
            int x;
            int y;
        } move_speed;

        int position;
        char image_path[MAX_PATH];
    } pen;

    struct _cpu {
        int max_core;
        int max_freq;
        int min_core;
        int min_freq;
    } cpu;

    struct _key {
        int swap_l1l2;
        int swap_r1r2;
    } key;
};

int init_cfg(void);
int read_cfg_json_file(void);
int read_sys_json_file(void);
int get_sys_volume(void);
int set_sys_volume(int vol);
int get_cfg_half_volume(void);
int set_cfg_half_volume(int enable);
int get_cfg_auto_save_load(void);
int set_cfg_auto_save_load(int enable);
int get_cfg_auto_save_load_slot(void);
int set_cfg_auto_save_load_slot(int slot);
int set_cfg_pen_image(const char *path);
int get_cfg_pen_image(char *ret, int ret_size);
int get_cfg_pen_position(void);
int set_cfg_pen_position(int pos);
int set_cfg_font_style(const char *path);
int get_cfg_font_style(char *ret, int ret_size);
int get_cfg_display_layout(void);
int set_cfg_display_layout(int idx);
int get_cfg_display_layout_alt(void);
int set_cfg_display_layout_alt(int idx);
int get_cfg_display_layout_alpha(void);
int set_cfg_display_layout_alpha(int idx);
int get_cfg_display_layout_border(void);
int set_cfg_display_layout_border(int idx);
int get_cfg_display_layout_position(void);
int set_cfg_display_layout_position(int idx);
int set_cfg_border_image(const char *path);
int get_cfg_border_image(char *ret, int ret_size);
int get_cfg_pen_move_speed_x(void);
int set_cfg_pen_move_speed_x(int speed);
int get_cfg_pen_move_speed_y(void);
int set_cfg_pen_move_speed_y(int speed);
int get_cfg_cpu_max_core(void);
int set_cfg_cpu_max_core(int core);
int get_cfg_cpu_max_freq(void);
int set_cfg_cpu_max_freq(int freq);
int get_cfg_cpu_min_core(void);
int set_cfg_cpu_min_core(int core);
int get_cfg_cpu_min_freq(void);
int set_cfg_cpu_min_freq(int freq);
int get_cfg_key_swap_l1l2(void);
int set_cfg_key_swap_l1l2(int swap);

#endif

