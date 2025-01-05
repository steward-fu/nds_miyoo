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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <sys/mman.h>

#if defined(UT)
#include "unity_fixture.h"
#endif

#include "cfg.h"
#include "log.h"
#include "hook.h"
#include "drastic.h"

int drastic_save_load_state_hook = 0;
char drastic_save_load_state_path[MAX_PATH] = { 0 };

extern miyoo_hook myhook;

#ifdef UT
TEST_GROUP(detour_drastic);

TEST_SETUP(detour_drastic)
{
}

TEST_TEAR_DOWN(detour_drastic)
{
}
#endif

int invoke_drastic_save_state(int slot)
{
    if ((slot < 0) || (slot > MAX_SLOT)) {
        err(DTR"invalid parameter(0x%x) in %s\n", slot, __func__);
        return -1;
    }

#if !defined(UT)
    char buf[255] = {0};
    nds_screen_copy16 _func0 = (nds_screen_copy16)myhook.fun.screen_copy16;

    void *d0 = malloc(0x18000);
    void *d1 = malloc(0x18000);

    if ((d0 != NULL) && (d1 != NULL)) {
        _func0(d0, 0);
        _func0(d1, 1);

        if (drastic_save_load_state_hook == 0) {
            nds_save_state_index _func1 =
                (nds_save_state_index)myhook.fun.save_state_index;

            _func1((void *)myhook.var.system.base, slot, d0, d1);
        }
        else {
            nds_save_state _func1 = (nds_save_state)myhook.fun.save_state;

            sprintf(buf, "%s_%d.dss", myhook.var.system.gamecard_name, slot);
            _func1(
                (void *)myhook.var.system.base,
                drastic_save_load_state_path,
                buf,
                d0,
                d1
            );
        }
    }
    if (d0 != NULL) {
        free(d0);
    }
    if (d1 != NULL) {
        free(d1);
    }
#endif

    return 0;
}

#if defined(UT)
TEST(detour_drastic, invoke_drastic_save_state)
{
    TEST_ASSERT_EQUAL_INT(-1, invoke_drastic_save_state(-1));
    TEST_ASSERT_EQUAL_INT(0, invoke_drastic_save_state(0));
    TEST_ASSERT_EQUAL_INT(-1, invoke_drastic_save_state(MAX_SLOT + 1));
}
#endif

int invoke_drastic_load_state(int slot)
{
    if ((slot < 0) || (slot > MAX_SLOT)) {
        err(DTR"invalid parameter(0x%x) in %s\n", slot, __func__);
        return -1;
    }

#if !defined(UT)
    char buf[255] = {0};

    if (drastic_save_load_state_hook == 0) {
        nds_load_state_index _func =
            (nds_load_state_index)myhook.fun.load_state_index;

        _func((void *)myhook.var.system.base, slot, 0, 0, 0);
    }
    else {
        nds_load_state _func = (nds_load_state)myhook.fun.load_state;

        sprintf(
            buf,
            "%s/%s_%d.dss",
            drastic_save_load_state_path,
            myhook.var.system.gamecard_name,
            slot
        );

        _func((void *)myhook.var.system.base, buf, 0, 0, 0);
    }
#endif

    return 0;
}

#if defined(UT)
TEST(detour_drastic, invoke_drastic_load_state)
{
    TEST_ASSERT_EQUAL_INT(-1, invoke_drastic_load_state(-1));
    TEST_ASSERT_EQUAL_INT(0, invoke_drastic_load_state(0));
    TEST_ASSERT_EQUAL_INT(-1, invoke_drastic_load_state(MAX_SLOT + 1));
}
#endif

int invoke_drastic_quit(void)
{
#if !defined(UT)
    nds_quit _func = (nds_quit)myhook.fun.quit;
    _func((void*)myhook.var.system.base);
#endif

    return 0;
}

#if defined(UT)
TEST(detour_drastic, invoke_drastic_quit)
{
    TEST_ASSERT_EQUAL_INT(0, invoke_drastic_quit());
}
#endif

int set_fast_forward(uint8_t v)
{
#if !defined(UT)
    uint32_t *ff = (uint32_t*)myhook.var.fast_forward;

    unlock_protected_area((uintptr_t)myhook.var.fast_forward);
    *ff = 0xe3a03000 | v;
#endif

    return 0;
}

#if defined(UT)
TEST(detour_drastic, set_fast_forward)
{
    TEST_ASSERT_EQUAL_INT(0, set_fast_forward(0));
}
#endif

int32_t drastic_load_state_index(
    void *system,
    uint32_t index,
    uint16_t *snapshot_top,
    uint16_t *snapshot_bottom,
    uint32_t snapshot_only)
{
    int32_t r = 0;

    if (!system || !snapshot_top || !snapshot_bottom) {
        err(
            DTR"invalid parameters(0x%x, 0x%x, 0x%x, 0x%x, 0x%x) in %s\n", 
            system,
            index,
            snapshot_top,
            snapshot_bottom,
            snapshot_only,
            __func__
        );

        return -1;
    }

#if !defined(UT)
    char buf[320] = {0};
    nds_load_state _func = (nds_load_state)myhook.fun.load_state;

    sprintf(
        buf,
        "%s/%s_%d.dss",
        drastic_save_load_state_path,
        (char*)myhook.var.system.gamecard_name,
        index
    );

    r = _func(
            (void*)myhook.var.system.base,
            buf,
            snapshot_top,
            snapshot_bottom,
            snapshot_only
        );
#endif

    return r;
}

#if defined(UT)
TEST(detour_drastic, drastic_load_state_index)
{
    uint16_t t[128] = { 0 };

    TEST_ASSERT_EQUAL_INT(-1, drastic_load_state_index(NULL, 0, NULL, NULL, 0));
    TEST_ASSERT_EQUAL_INT(-1, drastic_load_state_index(t, 0, NULL, NULL, 0));
    TEST_ASSERT_EQUAL_INT(-1, drastic_load_state_index(t, 0, t, NULL, 0));

    TEST_ASSERT_EQUAL_INT(0, drastic_load_state_index(t, 0, t, t, 0));
}
#endif

int32_t drastic_save_state_index(
    void *system,
    uint32_t index,
    uint16_t *snapshot_top,
    uint16_t *snapshot_bottom)
{
    int32_t r = 0;

    if (!system || !snapshot_top || !snapshot_bottom) {
        err(
            DTR"invalid parameters(0x%x, 0x%x, 0x%x, 0x%x) in %s\n", 
            system,
            index,
            snapshot_top,
            snapshot_bottom,
            __func__
        );
        return -1;
    }

#if !defined(UT)
    char buf[320] = {0};
    nds_save_state _func1 = (nds_save_state)myhook.fun.save_state;

    sprintf(buf, "%s_%d.dss", (char*)myhook.var.system.gamecard_name, index);
    r = _func1(
        (void*)myhook.var.system.base,
        drastic_save_load_state_path,
        buf,
        snapshot_top,
        snapshot_bottom
    );
#endif

    return r;
}

#if defined(UT)
TEST(detour_drastic, drastic_save_state_index)
{
    uint16_t t[128] = { 0 };

    TEST_ASSERT_EQUAL_INT(-1, drastic_save_state_index(NULL, 0, NULL, NULL));
    TEST_ASSERT_EQUAL_INT(-1, drastic_save_state_index(t, 0, NULL, NULL));
    TEST_ASSERT_EQUAL_INT(-1, drastic_save_state_index(t, 0, t, NULL));

    TEST_ASSERT_EQUAL_INT(0, drastic_save_state_index(t, 0, t, t));
}
#endif

void drastic_initialize_backup(
    backup_struct *backup,
    backup_type_enum backup_type,
    uint8_t *data,
    uint32_t size,
    char *path)
{
#if !defined(UT)
    char *data_file_name = NULL;
    FILE *__stream = NULL;
    FILE *data_file = NULL;
    size_t sVar1 = 0;
    long __off = 0;
    uint32_t uVar2 = 0;
    uint32_t backup_file_size = 0;
    uint32_t uVar3 = 0;
    void *pvVar4 = NULL;
    uint8_t *desmume_footer_ptr = NULL;
    uint8_t uVar5 = 0;
    uint32_t truncate_size = 0;
    uint32_t desmume_footer_position = 0;
    uint32_t clean_pages_loaded = 0;

    if (path != NULL) {
        data_file_name = malloc(MAX_PATH);
        memset(data_file_name, 0, MAX_PATH);
        sprintf(
            data_file_name,
            "%s/%s.dsv",
            drastic_save_load_state_path,
            myhook.var.system.gamecard_name
        );
    }
    backup->type = backup_type;
    backup->address_mask = size - 1;
    backup->fix_file_size = 0;
    backup->data = data;
    backup->firmware = 0;
    if (backup_type != BACKUP_TYPE_FLASH) {
        if (backup_type == BACKUP_TYPE_NONE) {
            backup->address_bytes = 0;
            goto LAB_08092f94;
        }

        if (backup_type != BACKUP_TYPE_EEPROM) {
            goto LAB_08092f94;
        }

        if (size < 0x10001) {
            uVar5 = 0x02;
            if (size < 0x201) {
                uVar5 = 0x01;
            }
            backup->address_bytes = uVar5;
            goto LAB_08092f94;
        }
    }
    backup->address_bytes = 3;

LAB_08092f94:
    if (data_file_name == (char *)0x0) {
        backup->file_path[0] = 0;
    }
    else {
        __stream = fopen(data_file_name,"rb");
        backup->footer_written = '\0';
        if (__stream == (FILE *)0x0) {
            warn(DTR"failed to load backup file in %s\n", __func__);
            memset(data, size, 0xff);
            memset(backup, 0xff, size >> 0xc);
        }
        else {
            sVar1 = fread(data, size, 1, __stream);
            if (sVar1 != 1) {
                warn(DTR"failed to load entire size in %s\n", __func__);
            }

            __off = ftell(__stream);
            fseek(__stream, 0, 2);
            uVar2 = ftell(__stream);
            fseek(__stream, __off, 0);
            fclose(__stream);
            info(
                DTR"loading backup file %s, %d bytes in %s\n",
                data_file_name,
                uVar2,
                __func__
            );

            if (size + 0x7a != uVar2) {
                backup->fix_file_size = size + 0x7a;
            }

            if (uVar2 < size) {
                uVar3 = uVar2 - 0x400 & ~((int)(uVar2 - 0x400) >> 0x1f);
                pvVar4 = memmem(
                    data + uVar3,
                    uVar2 - uVar3,
                    (const void *)myhook.var.desmume_footer_str,
                    0x52
                );

                if (pvVar4 != (void *)0x0) {
                    uVar2 = (int)pvVar4 - (int)data;
                    info(
                        DTR"found DeSmuME footer at %d. Truncating in %s\n",
                        uVar2,
                        __func__
                    );
                }
                uVar3 = uVar2 >> 0xe;
                warn(
                    DTR"backup file less than full size "
                    "(should be %d, loaded %d) in %s\n",
                    size,
                    uVar2,
                    __func__
                );

                memset(data + uVar2, size - uVar2, 0xff);
                memset(backup, 0, uVar3 * 4);
                memset(
                    backup->dirty_page_bitmap + uVar3,
                    0xff,
                    ((size + 0x3fff >> 0xe) - uVar3) * 4
                );
            }
            else {
                memset(backup, 0, size + 0x3fff >> 0xe);
            }
        }
        strncpy(backup->file_path, data_file_name, 0x3ff);
        backup->file_path[0x3ff] = 0;
    }
    backup->status = 0;

    if (data_file_name) {
        free(data_file_name);
    }
#endif
}

#if defined(UT)
TEST(detour_drastic, drastic_initialize_backup)
{
    TEST_PASS();
}
#endif

#if defined(UT)
TEST_GROUP_RUNNER(detour_drastic)
{
    RUN_TEST_CASE(detour_drastic, invoke_drastic_save_state);
    RUN_TEST_CASE(detour_drastic, invoke_drastic_load_state);
    RUN_TEST_CASE(detour_drastic, invoke_drastic_quit);
    RUN_TEST_CASE(detour_drastic, set_fast_forward);
    RUN_TEST_CASE(detour_drastic, drastic_load_state_index);
    RUN_TEST_CASE(detour_drastic, drastic_save_state_index);
    RUN_TEST_CASE(detour_drastic, drastic_initialize_backup);
}
#endif

