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

static size_t page_size = 4096;
miyoo_hook myhook = { 0 };

extern int drastic_save_load_state_hook;
extern char drastic_save_load_state_path[MAX_PATH];

#ifdef UT
TEST_GROUP(detour_hook);

TEST_SETUP(detour_hook)
{
}

TEST_TEAR_DOWN(detour_hook)
{
}
#endif

int unlock_protected_area(uintptr_t addr)
{
    if (!addr) {
        err(DTR"invalid parameter(0x%x) in %s\n", addr, __func__);
        return -1;
    }

#if !defined(UT)
    mprotect(ALIGN_ADDR(addr), page_size, PROT_READ | PROT_WRITE);
#endif

    return 0;
}

#if defined(UT)
TEST(detour_hook, unlock_protected_area)
{
    TEST_ASSERT_EQUAL_INT(-1, unlock_protected_area(0));
    TEST_ASSERT_EQUAL_INT(0, unlock_protected_area(0xdeadbeef));
}
#endif

int add_hook_point(uintptr_t func, void *cb)
{
    if (!func || !cb) {
        err(DTR"invalid parameters(0x%x, 0x%x) in %s\n", func, cb, __func__);
        return -1;
    }

#if !defined(UT)
    uintptr_t addr = (uintptr_t)cb;
    volatile uint8_t *base = (uint8_t *)func;

    unlock_protected_area(func);
    base[0] = 0x04;
    base[1] = 0xf0;
    base[2] = 0x1f;
    base[3] = 0xe5;
    base[4] = addr >> 0;
    base[5] = addr >> 8;
    base[6] = addr >> 16;
    base[7] = addr >> 24;
#endif

    return 0;
}

#if defined(UT)
TEST(detour_hook, add_hook_point)
{
    TEST_ASSERT_EQUAL_INT(-1, add_hook_point(0, 0));
    TEST_ASSERT_EQUAL_INT(0, add_hook_point(0xdeadbeef, (void *)0xdeadbeef));
}
#endif

static int init_hook_table(void)
{
    myhook.var.system.base = (uint32_t*)0x083f4000;
    myhook.var.system.gamecard_name = (uint32_t*)0x0847e8e8;
    myhook.var.system.savestate_num = (uint32_t*)0x08479780;

    myhook.var.sdl.bpp = (uint32_t*)0x0aee957c;
    myhook.var.sdl.need_init = (uint32_t*)0x0aee95a0;
    myhook.var.sdl.window = (uint32_t*)0x0aee9564;
    myhook.var.sdl.renderer = (uint32_t*)0x0aee9568;

    myhook.var.sdl.screen[0].show = (uint32_t*)0x0aee9544;
    myhook.var.sdl.screen[0].hres_mode = (uint32_t*)0x0aee9545;
    myhook.var.sdl.screen[0].texture = (uint32_t*)0x0aee952c;
    myhook.var.sdl.screen[0].pixels = (uint32_t*)0x0aee9530;
    myhook.var.sdl.screen[0].x = (uint32_t*)0x0aee9534;
    myhook.var.sdl.screen[0].y = (uint32_t*)0x0aee9538;

    myhook.var.sdl.screen[1].show = (uint32_t*)0x0aee9560;
    myhook.var.sdl.screen[1].hres_mode = (uint32_t*)0x0aee9561;
    myhook.var.sdl.screen[1].texture = (uint32_t*)0x0aee9548;
    myhook.var.sdl.screen[1].pixels = (uint32_t*)0x0aee954c;
    myhook.var.sdl.screen[1].x = (uint32_t*)0x0aee9550;
    myhook.var.sdl.screen[1].y = (uint32_t*)0x0aee9554;

    myhook.var.adpcm.step_table = (uint32_t*)0x0815a600;
    myhook.var.adpcm.index_step_table = (uint32_t*)0x0815a6b8;
    myhook.var.desmume_footer_str = (uint32_t*)0x0815a740;

    myhook.var.pcm_handler = (uint32_t*)0x083e532c;
    myhook.var.fast_forward = (uint32_t*)0x08006ad0;

    myhook.fun.free = 0x08003e58;
    myhook.fun.realloc = 0x0800435c;
    myhook.fun.malloc = 0x080046e0;
    myhook.fun.screen_copy16 = 0x080a59d8;
    myhook.fun.print_string = 0x080a5398;
    myhook.fun.load_state_index = 0x08095ce4;
    myhook.fun.save_state_index = 0x08095c10;
    myhook.fun.quit = 0x08006444;
    myhook.fun.savestate_pre = 0x08095a80;
    myhook.fun.savestate_post = 0x08095154;
    myhook.fun.update_screen = 0x080a83c0;
    myhook.fun.load_state = 0x080951c0;
    myhook.fun.save_state = 0x0809580c;
    myhook.fun.blit_screen_menu = 0x080a62d8;
    myhook.fun.initialize_backup = 0x08092f40;
    myhook.fun.set_screen_menu_off = 0x080a8240;
    myhook.fun.get_screen_ptr = 0x080a890c;
    myhook.fun.spu_adpcm_decode_block = 0x0808d268;
    myhook.fun.render_scanline_tiled_4bpp = 0x080bcf74;
    myhook.fun.render_polygon_setup_perspective_steps = 0x080c1cd4;

    return 0;
}

#if defined(UT)
TEST(detour_hook, init_hook_table)
{
    TEST_ASSERT_EQUAL_INT(0, init_hook_table());
    TEST_ASSERT_NOT_NULL(myhook.var.system.base);
    TEST_ASSERT_NOT_NULL(myhook.var.sdl.window);
    TEST_ASSERT_NOT_NULL(myhook.var.sdl.screen[0].show);
    TEST_ASSERT_NOT_NULL(myhook.var.sdl.screen[1].show);
    TEST_ASSERT_NOT_NULL(myhook.var.adpcm.step_table);
    TEST_ASSERT_NOT_NULL(myhook.var.fast_forward);
    TEST_ASSERT_TRUE(!!myhook.fun.free);
}
#endif

int init_detour_hook(void)
{
    if (init_hook_table() < 0) {
        return -1;
    }
    return 0;
}

#if defined(UT)
TEST(detour_hook, init_detour_hook)
{
    TEST_ASSERT_EQUAL_INT(0, init_detour_hook());
}
#endif

int restore_detour_hook(void)
{
    return 0;
}

#if defined(UT)
TEST(detour_hook, restore_detour_hook)
{
    TEST_ASSERT_EQUAL_INT(0, restore_detour_hook());
}
#endif

int set_page_size(size_t ps)
{
    if (ps <= 0) {
        return -1;
    }

    page_size = ps;
    return 0;
}

#if defined(UT)
TEST(detour_hook, set_page_size)
{
    const size_t PS = 4096;

    TEST_ASSERT_EQUAL_INT(-1, set_page_size(0));
    TEST_ASSERT_EQUAL_INT(0, set_page_size(PS));
    TEST_ASSERT_EQUAL_INT(PS, page_size);
}
#endif

int add_save_load_state_handler(const char *state_path)
{
    drastic_save_load_state_hook = 0;

    if (!state_path) {
        err(DTR"invalid parameter(0x%x) in %s\n", state_path, __func__);
        return -1;
    }

    if (state_path[0]) {
#if !(UT)
        if (add_hook_point(myhook.fun.load_state_index,
            drastic_load_state_index) < 0)
        {
            err(DTR"failed to add load_state_index hook in %s\n", __func__);
            return -1;
        }

        if (add_hook_point(myhook.fun.save_state_index,
            drastic_save_state_index) < 0)
        {
            err(DTR"failed to add save_state_index hook in %s\n", __func__);
            return -1;
        }

        if (add_hook_point(myhook.fun.initialize_backup,
            drastic_initialize_backup) < 0)
        {
            err(DTR"failed to add initialize_backup hook in %s\n", __func__);
            return -1;
        }
#endif
        drastic_save_load_state_hook = 1;
        strncpy(drastic_save_load_state_path, state_path, sizeof(drastic_save_load_state_path));
        return 0;
    }
    return -1;
}

#if defined(UT)
TEST(detour_hook, add_save_load_state_handler)
{
    const char *TEST_PATH = "/tmp";

    TEST_ASSERT_EQUAL_INT(-1, add_save_load_state_handler(NULL));
    TEST_ASSERT_EQUAL_INT(0, add_save_load_state_handler(TEST_PATH));
    TEST_ASSERT_EQUAL_INT(1, drastic_save_load_state_hook);
    TEST_ASSERT_EQUAL_STRING(TEST_PATH, drastic_save_load_state_path);
}
#endif

#if defined(UT)
TEST_GROUP_RUNNER(detour_hook)
{
    RUN_TEST_CASE(detour_hook, unlock_protected_area);
    RUN_TEST_CASE(detour_hook, add_hook_point);
    RUN_TEST_CASE(detour_hook, init_hook_table);
    RUN_TEST_CASE(detour_hook, init_detour_hook);
    RUN_TEST_CASE(detour_hook, restore_detour_hook);
    RUN_TEST_CASE(detour_hook, set_page_size);
    RUN_TEST_CASE(detour_hook, add_save_load_state_handler);
}
#endif

