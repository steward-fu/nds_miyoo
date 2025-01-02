//
//    NDS Emulator (DraStic) for Miyoo Handheld
//
//    This software is provided 'as-is', without any express or implied
//    warranty. In no event will the authors be held liable for any damages
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

#ifndef __HOOK_H__
#define __HOOK_H__

    #define ALIGN_ADDR(addr) ((void*)((size_t)(addr) & ~(page_size - 1)))

    typedef struct _system {
        uint32_t *base;
        uint32_t *gamecard_name;
        uint32_t *savestate_num;
    } system_t;

    typedef struct _screen {
        uint32_t *show;
        uint32_t *hres_mode;
        uint32_t *texture;
        uint32_t *pixels;
        uint32_t *x;
        uint32_t *y;
    } screen_t;

    typedef struct _sdl {
        uint32_t *bpp;
        uint32_t *need_init;
        uint32_t *window;
        uint32_t *renderer;
        screen_t screen[2];
    } sdl_t;

    typedef struct _adpcm {
        uint32_t *step_table;
        uint32_t *index_step_table;
    } adpcm_t;

    typedef struct _var {
        system_t system;

        sdl_t sdl;

        adpcm_t adpcm;

        uint32_t *pcm_handler;
        uint32_t *fast_forward;
        uint32_t *desmume_footer_str;
    } var_t;

    typedef struct _fun {
        uintptr_t free;
        uintptr_t realloc;
        uintptr_t malloc;
        uintptr_t screen_copy16;
        uintptr_t print_string;
        uintptr_t load_state_index;
        uintptr_t save_state_index;
        uintptr_t quit;
        uintptr_t savestate_pre;
        uintptr_t savestate_post;
        uintptr_t update_screen;
        uintptr_t load_state;
        uintptr_t save_state;
        uintptr_t blit_screen_menu;
        uintptr_t initialize_backup;
        uintptr_t set_screen_menu_off;
        uintptr_t get_screen_ptr;
        uintptr_t spu_adpcm_decode_block;
        uintptr_t render_scanline_tiled_4bpp;
        uintptr_t render_polygon_setup_perspective_steps;
    } fun_t;

    typedef struct _hook_table {
        fun_t fun;
        var_t var;
    } hook_table_t;

    int init_detour_hook(void);
    int restore_detour_hook(void);
    int add_adpcm_decode_hook(void *cb);
    int add_save_load_state_handler(const char *path);
    int set_page_size(size_t ps);
    uint32_t* get_adpcm_step_table(void);
    uint32_t* get_adpcm_index_step_table(void);

#endif

