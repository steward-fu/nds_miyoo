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

    #define VAR_SYSTEM                                  0x083f4000
    #define VAR_SYSTEM_GAMECARD_NAME                    0x0847e8e8
    #define VAR_SYSTEM_SAVESTATE_NUM                    0x08479780
    #define VAR_SDL_SCREEN_BPP                          0x0aee957c
    #define VAR_SDL_SCREEN_NEED_INIT                    0x0aee95a0
    #define VAR_SDL_SCREEN_WINDOW                       0x0aee9564
    #define VAR_SDL_SCREEN_RENDERER                     0x0aee9568

    #define VAR_SDL_SCREEN0_SHOW                        0x0aee9544
    #define VAR_SDL_SCREEN0_HRES_MODE                   0x0aee9545
    #define VAR_SDL_SCREEN0_TEXTURE                     0x0aee952c
    #define VAR_SDL_SCREEN0_PIXELS                      0x0aee9530
    #define VAR_SDL_SCREEN0_X                           0x0aee9534
    #define VAR_SDL_SCREEN0_Y                           0x0aee9538

    #define VAR_SDL_SCREEN1_SHOW                        0x0aee9560
    #define VAR_SDL_SCREEN1_HRES_MODE                   0x0aee9561
    #define VAR_SDL_SCREEN1_TEXTURE                     0x0aee9548
    #define VAR_SDL_SCREEN1_PIXELS                      0x0aee954c
    #define VAR_SDL_SCREEN1_X                           0x0aee9550
    #define VAR_SDL_SCREEN1_Y                           0x0aee9554

    #define VAR_ADPCM_STEP_TABLE                        0x0815a600
    #define VAR_ADPCM_INDEX_STEP_TABLE                  0x0815a6b8
    #define VAR_DESMUME_FOOTER_STR                      0x0815a740

    #define VAR_PCM_HANDLER                             0x083e532c
    #define VAR_FAST_FORWARD                            0x08006ad0

    #define FUN_FREE                                    0x08003e58
    #define FUN_REALLOC                                 0x0800435c
    #define FUN_MALLOC                                  0x080046e0
    #define FUN_SCREEN_COPY16                           0x080a59d8
    #define FUN_PRINT_STRING                            0x080a5398
    #define FUN_LOAD_STATE_INDEX                        0x08095ce4
    #define FUN_SAVE_STATE_INDEX                        0x08095c10
    #define FUN_QUIT                                    0x08006444
    #define FUN_SAVESTATE_PRE                           0x08095a80
    #define FUN_SAVESTATE_POST                          0x08095154
    #define FUN_UPDATE_SCREEN                           0x080a83c0
    #define FUN_LOAD_STATE                              0x080951c0
    #define FUN_SAVE_STATE                              0x0809580c
    #define FUN_BLIT_SCREEN_MENU                        0x080a62d8
    #define FUN_INITIALIZE_BACKUP                       0x08092f40
    #define FUN_SET_SCREEN_MENU_OFF                     0x080a8240
    #define FUN_GET_SCREEN_PTR                          0x080a890c
    #define FUN_SPU_ADPCM_DECODE_BLOCK                  0x0808d268
    #define FUN_RENDER_SCANLINE_TILED_4BPP              0x080bcf74
    #define FUN_RENDER_POLYGON_SETUP_PERSPECTIVE_STEPS  0x080c1cd4

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

#endif

