//
// NDS Emulator (DraStic) for Miyoo Handheld
// Steward Fu <steward.fu@gmail.com>
//
// This software is provided 'as-is', without any express or implied warranty.
// In no event will the authors be held liable for any damages arising from
// the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it freely,
// subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not claim
//    that you wrote the original software. If you use this software in a product,
//    an acknowledgment in the product documentation would be appreciated
//    but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

#ifndef __DETOUR_DRASTIC_H__
#define __DETOUR_DRASTIC_H__

    #define MAX_SLOT 20

    typedef enum _backup_type_enum {
        BACKUP_TYPE_NONE = 0,
        BACKUP_TYPE_FLASH = 1,
        BACKUP_TYPE_EEPROM = 2,
        BACKUP_TYPE_NAND = 3
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

    typedef int32_t (*nds_load_state_index)(
        void *system,
        uint32_t index,
        uint16_t *snapshot_top,
        uint16_t *snapshot_bottom,
        uint32_t snapshot_only);

    typedef int32_t (*nds_save_state_index)(
        void *system,
        uint32_t index,
        uint16_t *snapshot_top,
        uint16_t *snapshot_bottom);

    typedef int32_t (*nds_load_state)(
        void *system, const char *path,
        uint16_t *snapshot_top,
        uint16_t *snapshot_bottom,
        uint32_t snapshot_only);

    typedef int32_t (*nds_save_state)(
        void *system,
        const char *dir,
        char *filename,
        uint16_t *snapshot_top,
        uint16_t *snapshot_bottom);

    int set_fast_forward(uint8_t v);
    int invoke_drastic_quit(void);
    int invoke_drastic_save_state(int slot);
    int invoke_drastic_load_state(int slot);

    void drastic_initialize_backup(backup_struct *backup, backup_type_enum backup_type, uint8_t *data, uint32_t size, char *path);

    int32_t drastic_save_state_index(void *system, uint32_t index, uint16_t *snapshot_top, uint16_t *snapshot_bottom);

    int32_t drastic_load_state_index(void *system, uint32_t index, uint16_t *snapshot_top, uint16_t *snapshot_bottom, uint32_t snapshot_only);

#endif

