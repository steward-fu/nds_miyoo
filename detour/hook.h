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

#ifndef __DETOUR_H__
#define __DETOUR_H__

    #define VAR_SYSTEM                                  0x083f4000
    #define VAR_SYSTEM_GAMECARD_NAME                    0x0847e8e8
    #define VAR_SYSTEM_SAVESTATE_NUM                    0x08479780
    #define VAR_SDL_SCREEN_RENDERER                     0x0aee9568
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
    #define FUN_SET_SCREEN_MENU_OFF                     0x080a8240
    #define FUN_LOAD_STATE                              0x080951c0
    #define FUN_SAVE_STATE                              0x0809580c
    #define FUN_BLIT_SCREEN_MENU                        0x080a62d8
    #define FUN_INITIALIZE_BACKUP                       0x08092f40
    #define FUN_SET_SCREEN_MENU_OFF                     0x080a8240
    #define FUN_GET_SCREEN_PTR                          0x080a890c
    #define FUN_SPU_ADPCM_DECODE_BLOCK                  0x0808d268
    #define FUN_RENDER_SCANLINE_TILED_4BPP              0x080bcf74
    #define FUN_RENDER_POLYGON_SETUP_PERSPECTIVE_STEPS  0x080c1cd4

    #define CODE_FAST_FORWARD                           0x08006ad0
    #define ALIGN_ADDR(addr)                            ((void*)((size_t)(addr) & ~(page_size - 1)))

    typedef enum _backup_type_enum {
        BACKUP_TYPE_NONE   = 0,
        BACKUP_TYPE_FLASH  = 1,
        BACKUP_TYPE_EEPROM = 2,
        BACKUP_TYPE_NAND   = 3
    } backup_type_enum;

    typedef struct _backup_struct {
        uint32_t dirty_page_bitmap[2048];
        char file_path[1024];
        backup_type_enum type;
        uint32_t access_address;
        uint32_t address_mask;
        uint32_t fix_file_size;
        uint8_t *data;
        uint8_t jedec_id[4];
        uint32_t write_frame_counter;
        uint16_t mode;
        uint8_t state;
        uint8_t status;
        uint8_t address_bytes;
        uint8_t state_step;
        uint8_t firmware;
        uint8_t footer_written;
    } backup_struct;

    typedef struct _spu_channel_struct {
        int16_t adpcm_sample_cache[64];
        uint64_t sample_offset;
        uint64_t frequency_step;
        uint32_t adpcm_cache_block_offset;
        uint8_t *io_region;
        uint8_t *samples;
        uint32_t sample_address;
        uint32_t sample_length;
        uint32_t loop_wrap;
        int16_t volume_multiplier_left;
        int16_t volume_multiplier_right;
        int16_t adpcm_loop_sample;
        int16_t adpcm_sample;
        uint8_t format;
        uint8_t dirty_bits;
        uint8_t active;
        uint8_t adpcm_loop_index;
        uint8_t adpcm_current_index;
        uint8_t adpcm_looped;
        uint8_t capture_timer;
    } spu_channel_struct;

    typedef void (*nds_free)(void *ptr);
    typedef void (*nds_set_screen_menu_off)(void);
    typedef void (*nds_quit)(void *system);
    typedef void (*nds_screen_copy16)(uint16_t *dest, uint32_t screen_number);
    typedef void (*nds_spu_adpcm_decode_block)(spu_channel_struct *channel);

    typedef void* (*nds_get_screen_ptr)(uint32_t screen_number);
    typedef void* (*nds_realloc)(void *ptr, size_t size);
    typedef void* (*nds_malloc)(size_t size);

    typedef int32_t (*nds_load_state_index)(void *system, uint32_t index, uint16_t *snapshot_top, uint16_t *snapshot_bottom, uint32_t snapshot_only);
    typedef int32_t (*nds_save_state_index)(void *system, uint32_t index, uint16_t *snapshot_top, uint16_t *snapshot_bottom);
    typedef int32_t (*nds_load_state)(void *system, const char *path, uint16_t *snapshot_top, uint16_t *snapshot_bottom, uint32_t snapshot_only);
    typedef int32_t (*nds_save_state)(void *system, const char *dir, char *filename, uint16_t *snapshot_top, uint16_t *snapshot_bottom);

    int detour_init(size_t page_size, const char *path);
    int detour_quit(void);
    int detour_hook(uint32_t old_func, uint32_t new_func);

    int dtr_quit(void);
    int dtr_savestate(int slot);
    int dtr_loadstate(int slot);
    int dtr_fastforward(uint8_t v);

#endif

