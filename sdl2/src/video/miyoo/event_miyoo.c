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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <dirent.h>
#include <linux/input.h>

#include "../../SDL_internal.h"
#include "../../events/SDL_events_c.h"
#include "../../core/linux/SDL_evdev.h"
#include "../../thread/SDL_systhread.h"

#include "video_miyoo.h"
#include "event_miyoo.h"
#include "../../joystick/miyoo/joystick_miyoo.h"

#include "snd.h"
#include "log.h"
#include "hook.h"
#include "cfg.pb.h"
#include "drastic.h"

#if defined(UT)
#include "unity_fixture.h"
#endif

miyoo_event myevent = { 0 };

extern miyoo_joystick myjoy;
extern miyoo_settings mycfg;

extern GFX gfx;
extern NDS nds;
extern MiyooVideoInfo vid;
extern int pixel_filter;
extern int FB_W;
extern int FB_H;

#if defined(UT)
TEST_GROUP(sdl2_event_miyoo);

TEST_SETUP(sdl2_event_miyoo)
{
    reset_config_settings();
}

TEST_TEAR_DOWN(sdl2_event_miyoo)
{
}
#endif

static void rectify_pen_position(void)
{
    if (myevent.pen.x < 0) {
        myevent.pen.x = 0;
    }
    if (myevent.pen.x >= myevent.pen.max_x) {
        myevent.pen.x = myevent.pen.max_x;
    }
    if (myevent.pen.y < 0) {
        myevent.pen.y = 0;
    }
    if (myevent.pen.y >= myevent.pen.max_y) {
        myevent.pen.y = myevent.pen.max_y;
    }
}

#if defined(UT)
TEST(sdl2_event_miyoo, rectify_pen_position)
{
    myevent.pen.x = 1000;
    myevent.pen.y = 2000;
    myevent.pen.max_x = 100;
    myevent.pen.max_y = 200;
    rectify_pen_position();
    TEST_ASSERT_EQUAL_INT(myevent.pen.max_x, myevent.pen.x);
    TEST_ASSERT_EQUAL_INT(myevent.pen.max_y, myevent.pen.y);

    myevent.pen.x = -1000;
    myevent.pen.y = -2000;
    myevent.pen.max_x = 100;
    myevent.pen.max_y = 200;
    rectify_pen_position();
    TEST_ASSERT_EQUAL_INT(0, myevent.pen.x);
    TEST_ASSERT_EQUAL_INT(0, myevent.pen.y);
}
#endif

static int portrait_screen_layout(int layout)
{
    if ((layout == NDS_SCREEN_LAYOUT_12) ||
        (layout == NDS_SCREEN_LAYOUT_13) ||
        (layout == NDS_SCREEN_LAYOUT_14) ||
        (layout == NDS_SCREEN_LAYOUT_15))
    {
        return 1;
    }
    return 0;
}

#if defined(UT)
TEST(sdl2_event_miyoo, portrait_screen_layout)
{
    TEST_ASSERT_EQUAL_INT(1, portrait_screen_layout(NDS_SCREEN_LAYOUT_12));
    TEST_ASSERT_EQUAL_INT(1, portrait_screen_layout(NDS_SCREEN_LAYOUT_13));
    TEST_ASSERT_EQUAL_INT(1, portrait_screen_layout(NDS_SCREEN_LAYOUT_14));
    TEST_ASSERT_EQUAL_INT(1, portrait_screen_layout(NDS_SCREEN_LAYOUT_15));
    TEST_ASSERT_EQUAL_INT(0, portrait_screen_layout(NDS_SCREEN_LAYOUT_10));
}
#endif

static int get_movement_interval(move_dir_t type)
{
    float move = 0.0;
    uint32_t div = 0;
    uint32_t yv = mycfg.pen.speed.y;
    uint32_t xv = mycfg.pen.speed.x;

    if (myevent.pen.lower_speed) {
        yv <<= 1;
        xv <<= 1;
    }

    if (portrait_screen_layout(nds.dis_mode)) {
        div = (type == MOVE_DIR_UP_DOWN) ? yv : xv;
    }
    else {
        div = (type == MOVE_DIR_LEFT_RIGHT) ? xv : yv;
    }

    if (div > 0) {
        move = ((float)clock() - myevent.pen.pre_ticks) / div;
    }

    if (move <= 0.0) {
        move = 1.0;
    }

    return (int)(1.0 * move);
}

#if defined(UT)
TEST(sdl2_event_miyoo, get_movement_interval)
{
    myevent.pen.pre_ticks = clock();
    TEST_ASSERT_LESS_THAN(1000, get_movement_interval(MOVE_DIR_UP_DOWN));

    myevent.pen.pre_ticks = clock();
    TEST_ASSERT_LESS_THAN(1000, get_movement_interval(MOVE_DIR_LEFT_RIGHT));
}
#endif

static uint32_t release_all_report_keys(void)
{
    int cc = 0;

    for (cc = 0; cc <= KEY_BIT_LAST; cc++) {
        if (myevent.key.cur_bits & 1) {
#if !defined(UT)
            SDL_SendKeyboardKey(
                SDL_RELEASED,
                SDL_GetScancodeFromKey(myevent.key.report_key[cc])
            );
#endif
        }
        myevent.key.cur_bits >>= 1;
    }
    myevent.key.cur_bits = 0;

    return myevent.key.cur_bits;
}

#if defined(UT)
TEST(sdl2_event_miyoo, release_all_report_keys)
{
    myevent.key.cur_bits = 0xffffffff;
    TEST_ASSERT_EQUAL_INT(0, release_all_report_keys());
    TEST_ASSERT_EQUAL_INT(0, myevent.key.cur_bits);
}
#endif

static int hit_hotkey(uint32_t bit)
{
    uint32_t mask = 0;
    uint32_t hotkey_bit = (mycfg.key.hotkey == _key__hotkey_select) ?
        KEY_BIT_SELECT : KEY_BIT_MENU;

    mask = (1 << bit) | (1 << hotkey_bit);
    return (myevent.key.cur_bits ^ mask) ? 0 : 1;
}

#if defined(UT)
TEST(sdl2_event_miyoo, hit_hotkey)
{
    uint32_t hotkey_bit = (mycfg.key.hotkey == _key__hotkey_select) ?
        KEY_BIT_SELECT : KEY_BIT_MENU;

    myevent.key.cur_bits = 0;
    TEST_ASSERT_EQUAL_INT(0, hit_hotkey(KEY_BIT_A));

    myevent.key.cur_bits = 1 << hotkey_bit;
    TEST_ASSERT_EQUAL_INT(0, hit_hotkey(KEY_BIT_B));

    myevent.key.cur_bits |= (1 << KEY_BIT_X);
    TEST_ASSERT_EQUAL_INT(1, hit_hotkey(KEY_BIT_X));

    myevent.key.cur_bits |= (1 << KEY_BIT_Y);
    TEST_ASSERT_EQUAL_INT(0, hit_hotkey(KEY_BIT_Y));
}
#endif

static uint32_t set_key_bit(uint32_t bit, int val)
{
    uint32_t hotkey_bit = (mycfg.key.hotkey == _key__hotkey_select) ?
        KEY_BIT_SELECT : KEY_BIT_MENU;

    if (val > 0) {
        if (hotkey_bit == bit) {
            myevent.key.cur_bits = 0;
        }
        myevent.key.cur_bits |= (1 << bit);
    }
    else {
        myevent.key.cur_bits &= ~(1 << bit);
    }

    return myevent.key.cur_bits;
}

#if defined(UT)
TEST(sdl2_event_miyoo, set_key_bit)
{
    myevent.key.cur_bits = 0;
    TEST_ASSERT_EQUAL_INT(0, set_key_bit(KEY_BIT_SELECT, 0));
    TEST_ASSERT_EQUAL_INT((1 << KEY_BIT_B), set_key_bit(KEY_BIT_B, 1));
    TEST_ASSERT_EQUAL_INT((1 << KEY_BIT_MENU), set_key_bit(KEY_BIT_MENU, 1));

    TEST_ASSERT_EQUAL_INT(
        (1 << KEY_BIT_MENU) | (1 << KEY_BIT_A),
        set_key_bit(KEY_BIT_A, 1));

    TEST_ASSERT_EQUAL_INT(
        (1 << KEY_BIT_MENU) | (1 << KEY_BIT_A),
        set_key_bit(KEY_BIT_SELECT, 0));

    TEST_ASSERT_EQUAL_INT((1 << KEY_BIT_A), set_key_bit(KEY_BIT_MENU, 0));
}
#endif

#if defined(A30) || defined(UT)
static int update_joystick_key(
    int update_x,
    int update_y,
    int pre_x,
    int pre_y)
{
    static int pre_up = 0;
    static int pre_down = 0;
    static int pre_left = 0;
    static int pre_right = 0;

    int r = 0;

    if (update_x) {
        if (pre_x < myevent.joy.threshold.left) {
            if (pre_left == 0) {
                r = 1;
                pre_left = 1;
                set_key_bit(KEY_BIT_LEFT, 1);
            }
        }
        else if (pre_x > myevent.joy.threshold.right) {
            if (pre_right == 0) {
                r = 1;
                pre_right = 1;
                set_key_bit(KEY_BIT_RIGHT, 1);
            }
        }
        else {
            if (pre_left != 0) {
                r = 1;
                pre_left = 0;
                set_key_bit(KEY_BIT_LEFT, 0);
            }
            if (pre_right != 0) {
                r = 1;
                pre_right = 0;
                set_key_bit(KEY_BIT_RIGHT, 0);
            }
        }
    }

    if (update_y) {
        if (pre_y < myevent.joy.threshold.up) {
            if (pre_up == 0) {
                r = 1;
                pre_up = 1;
                set_key_bit(KEY_BIT_UP, 1);
            }
        }
        else if (pre_y > myevent.joy.threshold.down) {
            if (pre_down == 0) {
                r = 1;
                pre_down = 1;
                set_key_bit(KEY_BIT_DOWN, 1);
            }
        }
        else {
            if (pre_up != 0) {
                r = 1;
                pre_up = 0;
                set_key_bit(KEY_BIT_UP, 0);
            }
            if (pre_down != 0) {
                r = 1;
                pre_down = 0;
                set_key_bit(KEY_BIT_DOWN, 0);
            }
        }
    }

    return r;
}

#if defined(UT)
TEST(sdl2_event_miyoo, update_joystick_pen)
{
    myevent.joy.threshold.left = -10;
    myevent.joy.threshold.right = 10;
    myevent.joy.threshold.up = -10;
    myevent.joy.threshold.down = 10;
    TEST_ASSERT_EQUAL_INT(0, update_joystick_key(0, 0, 100, 100));

    myevent.key.cur_bits = 0;
    TEST_ASSERT_EQUAL_INT(1, update_joystick_key(1, 0, 100, 100));
    TEST_ASSERT_EQUAL_INT((1 << KEY_BIT_RIGHT), myevent.key.cur_bits);

    TEST_ASSERT_EQUAL_INT(1, update_joystick_key(1, 0, 0, 0));
    TEST_ASSERT_EQUAL_INT(0, myevent.key.cur_bits);

    myevent.key.cur_bits = 0;
    TEST_ASSERT_EQUAL_INT(1, update_joystick_key(0, 1, 100, 100));
    TEST_ASSERT_EQUAL_INT((1 << KEY_BIT_DOWN), myevent.key.cur_bits);

    TEST_ASSERT_EQUAL_INT(1, update_joystick_key(0, 1, 0, 0));
    TEST_ASSERT_EQUAL_INT(0, myevent.key.cur_bits);

    myevent.key.cur_bits = 0;
    TEST_ASSERT_EQUAL_INT(1, update_joystick_key(1, 1, -100, -100));
    TEST_ASSERT_EQUAL_INT(
        (1 << KEY_BIT_LEFT) | (1 << KEY_BIT_UP),
        myevent.key.cur_bits
    );
}
#endif

static int update_joystick_pen(
    int update_x,
    int update_y,
    int pre_x,
    int pre_y)
{
    static int pre_up = 0;
    static int pre_down = 0;
    static int pre_left = 0;
    static int pre_right = 0;

    if (update_x) {
        if (pre_x < myevent.joy.threshold.left) {
            if (pre_left == 0) {
                pre_left = 1;
            }
        }
        else if (pre_x > myevent.joy.threshold.right) {
            if (pre_right == 0) {
                pre_right = 1;
            }
        }
        else {
            if (pre_left != 0) {
                pre_left = 0;
            }
            if (pre_right != 0) {
                pre_right = 0;
            }
        }
    }

    if (update_y) {
        if (pre_y < myevent.joy.threshold.up) {
            if (pre_up == 0) {
                pre_up = 1;
            }
        }
        else if (pre_y > myevent.joy.threshold.down) {
            if (pre_down == 0) {
                pre_down = 1;
            }
        }
        else {
            if (pre_up != 0) {
                pre_up = 0;
            }
            if (pre_down != 0) {
                pre_down = 0;
            }
        }
    }

    if (pre_up || pre_down || pre_left || pre_right) {
        if (myevent.key.cur_bits &  (1 << KEY_BIT_Y)) {
            if (pre_right) {
                static int cc = 0;

                if (cc == 0) {
                    nds.pen.sel+= 1;
                    if (nds.pen.sel >= nds.pen.max) {
                        nds.pen.sel = 0;
                    }
                    reload_pen();
                    cc = 30;
                }
                else {
                    cc -= 1;
                }
            }
        }
        else {
            int x = 0;
            int y = 0;
            const int xv = mycfg.joy.left.x.step;
            const int yv = mycfg.joy.left.y.step;

            if (portrait_screen_layout(nds.dis_mode) &&
                (nds.keys_rotate == 0))
            {
                if (pre_down) {
                    myevent.pen.x -= xv;
                }
                if (pre_up) {
                    myevent.pen.x += xv;
                }
                if (pre_left) {
                    myevent.pen.y -= yv;
                }
                if (pre_right) {
                    myevent.pen.y += yv;
                }
            }
            else {
                if (pre_left) {
                    myevent.pen.x -= xv;
                }
                if (pre_right) {
                    myevent.pen.x += xv;
                }
                if (pre_up) {
                    myevent.pen.y -= yv;
                }
                if (pre_down) {
                    myevent.pen.y += yv;
                }
            }
            rectify_pen_position();

            x = (myevent.pen.x * 160) / myevent.pen.max_x;
            y = (myevent.pen.y * 120) / myevent.pen.max_y;
            SDL_SendMouseMotion(
                vid.window,
                0,
                0,
                x + 80,
                y + (nds.pen.pos ? 120 : 0)
            );
        }
        mycfg.pen.show.count = DEF_CFG_PEN_SHOW_COUNT;
    }

    return 0;
}

static int update_joystick_customized_key(
    int update_x,
    int update_y,
    int pre_x,
    int pre_y)
{
    static int pre_up = 0;
    static int pre_down = 0;
    static int pre_left = 0;
    static int pre_right = 0;

    int r = 0;

    if (update_x) {
        if (pre_x < myevent.joy.threshold.left) {
            if (pre_left == 0) {
                r = 1;
                pre_left = 1;
                set_key_bit(mycfg.joy.left.remap.left, 1);
            }
        }
        else if (pre_x > myevent.joy.threshold.right) {
            if (pre_right == 0) {
                r = 1;
                pre_right = 1;
                set_key_bit(mycfg.joy.left.remap.right, 1);
            }
        }
        else {
            if (pre_left != 0) {
                r = 1;
                pre_left = 0;
                set_key_bit(mycfg.joy.left.remap.left, 0);
            }
            if (pre_right != 0) {
                r = 1;
                pre_right = 0;
                set_key_bit(mycfg.joy.left.remap.right, 0);
            }
        }
    }

    if (update_y) {
        if (pre_y < myevent.joy.threshold.up) {
            if (pre_up == 0) {
                r = 1;
                pre_up = 1;
                set_key_bit(mycfg.joy.left.remap.up, 1);
            }
        }
        else if (pre_y > myevent.joy.threshold.down) {
            if (pre_down == 0) {
                r = 1;
                pre_down = 1;
                set_key_bit(mycfg.joy.left.remap.down, 1);
            }
        }
        else {
            if (pre_up != 0) {
                r = 1;
                pre_up = 0;
                set_key_bit(mycfg.joy.left.remap.up, 0);
            }
            if (pre_down != 0) {
                r = 1;
                pre_down = 0;
                set_key_bit(mycfg.joy.left.remap.down, 0);
            }
        }
    }

    return r;
}

static int check_joystick_status(void)
{
    static int pre_x = -1;
    static int pre_y = -1;

    int need_handle_x = 0;
    int need_handle_y = 0;

    if (myjoy.last_x != pre_x) {
        pre_x = myjoy.last_x;
        need_handle_x = 1;
    }

    if (myjoy.last_y != pre_y) {
        pre_y = myjoy.last_y;
        need_handle_y = 1;
    }

    if (mycfg.joy.left.mode == _joy__lr__mode_key) {
        if (need_handle_x || need_handle_y) {
            return update_joystick_key(
                need_handle_x,
                need_handle_y,
                pre_x,
                pre_y
            );
        }
    }
    else if (mycfg.joy.left.mode == _joy__lr__mode_pen) {
        if (need_handle_x || need_handle_y) {
            return update_joystick_pen(
                need_handle_x,
                need_handle_y,
                pre_x,
                pre_y
            );
        }
    }
    else if (mycfg.joy.left.mode == _joy__lr__mode_cust) {
        if (need_handle_x || need_handle_y) {
            return update_joystick_customized_key(
                need_handle_x,
                need_handle_y,
                pre_x,
                pre_y
            );
        }
    }

    return 0;
}
#endif

static int handle_hotkey(void)
{
    int hotkey_mask = 0;

    hotkey_mask = 1;
    if (nds.menu.enable || nds.menu.drastic.enable) {
        hotkey_mask = 0;
    }

    if (hotkey_mask && hit_hotkey(KEY_BIT_UP)) {
        if (myevent.dev.mode == DEV_MODE_PEN) {
            switch (nds.dis_mode) {
            case NDS_SCREEN_LAYOUT_0:
            case NDS_SCREEN_LAYOUT_1:
            case NDS_SCREEN_LAYOUT_2:
            case NDS_SCREEN_LAYOUT_3:
                break;
            default:
                nds.pen.pos = 1;
                break;
            }
        }
#if defined(A30) || defined(UT)
        if (mycfg.joy.left.mode == _joy__lr__mode_pen) {
            nds.pen.pos = 1;
        }
#endif
        set_key_bit(KEY_BIT_UP, 0);
    }

    if (hotkey_mask && hit_hotkey(KEY_BIT_DOWN)) {
        if (myevent.dev.mode == DEV_MODE_PEN) {
            switch (nds.dis_mode) {
            case NDS_SCREEN_LAYOUT_0:
            case NDS_SCREEN_LAYOUT_1:
            case NDS_SCREEN_LAYOUT_2:
            case NDS_SCREEN_LAYOUT_3:
                break;
            default:
                nds.pen.pos = 0;
                break;
            }
        }
#if defined(A30) || defined(UT)
        if (mycfg.joy.left.mode == _joy__lr__mode_pen) {
            nds.pen.pos = 0;
        }
#endif
        set_key_bit(KEY_BIT_DOWN, 0);
    }

    if (hotkey_mask && hit_hotkey(KEY_BIT_LEFT)) {
        if (nds.hres_mode == 0) {
            if (nds.dis_mode > 0) {
                nds.dis_mode -= 1;
            }
        }
        else {
            nds.dis_mode = NDS_SCREEN_LAYOUT_17;
        }
        set_key_bit(KEY_BIT_LEFT, 0);
    }

    if (hotkey_mask && hit_hotkey(KEY_BIT_RIGHT)) {
        if (nds.hres_mode == 0) {
            if (nds.dis_mode < NDS_SCREEN_LAYOUT_LAST) {
                nds.dis_mode += 1;
            }
        }
        else {
            nds.dis_mode = NDS_SCREEN_LAYOUT_18;
        }
        set_key_bit(KEY_BIT_RIGHT, 0);
    }

    if (hotkey_mask && hit_hotkey(KEY_BIT_A)) {
        if ((myevent.dev.mode == DEV_MODE_KEY) && (nds.hres_mode == 0)) {
            uint32_t tmp = nds.alt_mode;
            nds.alt_mode = nds.dis_mode;
            nds.dis_mode = tmp;
        }
        set_key_bit(KEY_BIT_A, 0);
    }

    if (hotkey_mask && hit_hotkey(KEY_BIT_B)) {
        pixel_filter = pixel_filter ? 0 : 1;
        set_key_bit(KEY_BIT_B, 0);
    }

    if (hit_hotkey(KEY_BIT_X)) {
        set_key_bit(KEY_BIT_X, 0);
    }

    if (hit_hotkey(KEY_BIT_Y)) {
        if (hotkey_mask) {
            if (myevent.dev.mode == DEV_MODE_KEY) {
                if ((nds.overlay.sel >= nds.overlay.max) &&
                    (nds.dis_mode != NDS_SCREEN_LAYOUT_0) &&
                    (nds.dis_mode != NDS_SCREEN_LAYOUT_1) &&
                    (nds.dis_mode != NDS_SCREEN_LAYOUT_3) &&
                    (nds.dis_mode != NDS_SCREEN_LAYOUT_18))
                {
                    nds.theme.sel+= 1;
                    if (nds.theme.sel > nds.theme.max) {
                        nds.theme.sel = 0;
                    }
                }
            }
            else {
                nds.pen.sel+= 1;
                if (nds.pen.sel >= nds.pen.max) {
                    nds.pen.sel = 0;
                }
                reload_pen();
            }
        }
        else {
            nds.menu.sel+= 1;
            if (nds.menu.sel >= nds.menu.max) {
                nds.menu.sel = 0;
            }
            reload_menu();

            if (nds.menu.drastic.enable) {
                SDL_SendKeyboardKey(SDL_PRESSED, SDLK_e);
                usleep(100000);
                SDL_SendKeyboardKey(SDL_RELEASED, SDLK_e);
            }
        }
        set_key_bit(KEY_BIT_Y, 0);
    }

    if (hotkey_mask && hit_hotkey(KEY_BIT_START)) {
        if (nds.menu.enable == 0) {
            nds.menu.enable = 1;
            usleep(100000);
            handle_menu(-1);
            myevent.key.pre_bits = myevent.key.cur_bits = 0;
        }
        set_key_bit(KEY_BIT_START, 0);
    }

    if (mycfg.key.hotkey == _key__hotkey_menu) {
        if (hotkey_mask && hit_hotkey(KEY_BIT_SELECT)) {
            set_key_bit(KEY_BIT_MENU_ONION, 1);
            set_key_bit(KEY_BIT_SELECT, 0);
        }
    }

    if (hotkey_mask && hit_hotkey(KEY_BIT_R1)) {
        static int pre_ff = 0;

        if (pre_ff != nds.fast_forward) {
            pre_ff = nds.fast_forward;
            set_fast_forward(nds.fast_forward);
        }
        set_key_bit(KEY_BIT_FF, 1);
        set_key_bit(KEY_BIT_R1, 0);
    }

    if (hotkey_mask && hit_hotkey(KEY_BIT_L1)) {
        set_key_bit(KEY_BIT_EXIT, 1);
        set_key_bit(KEY_BIT_L1, 0);
    }

    if (hotkey_mask && hit_hotkey(KEY_BIT_R2)) {
        set_key_bit(KEY_BIT_QLOAD, 1);
        set_key_bit(KEY_BIT_R2, 0);
    }

    if (hotkey_mask && hit_hotkey(KEY_BIT_L2)) {
        set_key_bit(KEY_BIT_QSAVE, 1);
        set_key_bit(KEY_BIT_L2, 0);
    }
    else if (myevent.key.cur_bits & (1 << KEY_BIT_L2)) {
#if defined(A30) || defined(UT)
        if (mycfg.joy.left.mode != _joy__lr__mode_pen) {
#endif
            if ((nds.menu.enable == 0) && (nds.menu.drastic.enable == 0)) {
                myevent.dev.mode = (myevent.dev.mode == DEV_MODE_KEY) ?
                    DEV_MODE_PEN : DEV_MODE_KEY;
                set_key_bit(KEY_BIT_L2, 0);

                if (myevent.dev.mode == DEV_MODE_PEN) {
                    release_all_report_keys();
                }
                myevent.pen.lower_speed = 0;
            }
#if defined(A30) || defined(UT)
        }
#endif
    }

    if (!(myevent.key.cur_bits & 0x0f)) {
        myevent.pen.pre_ticks = clock();
    }

    return 0;
}

static int input_handler(void *data)
{
    struct input_event ev = {0};

    uint32_t l1 = DEV_KEY_CODE_L1;
    uint32_t r1 = DEV_KEY_CODE_R1;
    uint32_t l2 = DEV_KEY_CODE_L2;
    uint32_t r2 = DEV_KEY_CODE_R2;

    uint32_t a = DEV_KEY_CODE_A;
    uint32_t b = DEV_KEY_CODE_B;
    uint32_t x = DEV_KEY_CODE_X;
    uint32_t y = DEV_KEY_CODE_Y;

    uint32_t up = DEV_KEY_CODE_UP;
    uint32_t down = DEV_KEY_CODE_DOWN;
    uint32_t left = DEV_KEY_CODE_LEFT;
    uint32_t right = DEV_KEY_CODE_RIGHT;

    myevent.running = 1;
    while (myevent.running) {
        SDL_SemWait(myevent.lock);

        if ((nds.menu.enable == 0) &&
            (nds.menu.drastic.enable == 0) &&
            nds.keys_rotate)
        {
            if (nds.keys_rotate == 1) {
                up = DEV_KEY_CODE_LEFT;
                down = DEV_KEY_CODE_RIGHT;
                left = DEV_KEY_CODE_DOWN;
                right = DEV_KEY_CODE_UP;

                a = DEV_KEY_CODE_X;
                b = DEV_KEY_CODE_A;
                x = DEV_KEY_CODE_Y;
                y = DEV_KEY_CODE_B;
            }
            else {
                up = DEV_KEY_CODE_RIGHT;
                down = DEV_KEY_CODE_LEFT;
                left = DEV_KEY_CODE_UP;
                right = DEV_KEY_CODE_DOWN;

                a = DEV_KEY_CODE_B;
                b = DEV_KEY_CODE_Y;
                x = DEV_KEY_CODE_A;
                y = DEV_KEY_CODE_X;
            }
        }
        else {
            up = DEV_KEY_CODE_UP;
            down = DEV_KEY_CODE_DOWN;
            left = DEV_KEY_CODE_LEFT;
            right = DEV_KEY_CODE_RIGHT;

            a = DEV_KEY_CODE_A;
            b = DEV_KEY_CODE_B;
            x = DEV_KEY_CODE_X;
            y = DEV_KEY_CODE_Y;
        }

        if (nds.swap_l1l2) {
            l1 = DEV_KEY_CODE_L2;
            l2 = DEV_KEY_CODE_L1;
        }
        else {
            l1 = DEV_KEY_CODE_L1;
            l2 = DEV_KEY_CODE_L2;
        }

        if (nds.swap_r1r2) {
            r1 = DEV_KEY_CODE_R2;
            r2 = DEV_KEY_CODE_R1;
        }
        else {
            r1 = DEV_KEY_CODE_R1;
            r2 = DEV_KEY_CODE_R2;
        }

        if (myevent.dev.fd > 0) {
            int r = 0;

            if (read(myevent.dev.fd, &ev, sizeof(struct input_event))) {
                if ((ev.type == EV_KEY) && (ev.value != 2)) {
                    r = 1;
                    debug(SDL"%s: code:%d, value:%d in %s\n",
                        INPUT_DEV, ev.code, ev.value, __func__);
                    if (ev.code == l1) {
                        set_key_bit(KEY_BIT_L1, ev.value);
                    }
                    if (ev.code == r1) {
                        set_key_bit(KEY_BIT_R1, ev.value);
                    }
                    if (ev.code == up) {
                        set_key_bit(KEY_BIT_UP, ev.value);
                    }
                    if (ev.code == down) {
                        set_key_bit(KEY_BIT_DOWN, ev.value);
                    }
                    if (ev.code == left) {
                        set_key_bit(KEY_BIT_LEFT, ev.value);
                    }
                    if (ev.code == right) {
                        set_key_bit(KEY_BIT_RIGHT, ev.value);
                    }
                    if (ev.code == a) {
                        set_key_bit(KEY_BIT_A, ev.value);
                    }
                    if (ev.code == b) {
                        set_key_bit(KEY_BIT_B, ev.value);
                    }
                    if (ev.code == x) {
                        set_key_bit(KEY_BIT_X, ev.value);
                    }
                    if (ev.code == y) {
                        set_key_bit(KEY_BIT_Y, ev.value);
                    }
#if defined(A30) || defined(UT)
                    if (ev.code == r2) {
                        if (mycfg.joy.left.mode == _joy__lr__mode_pen) {
                            mycfg.pen.show.count = DEF_CFG_PEN_SHOW_COUNT;
                            SDL_SendMouseButton(vid.window, 0,
                                ev.value ? SDL_PRESSED : SDL_RELEASED,
                                SDL_BUTTON_LEFT);
                        }
                        set_key_bit(KEY_BIT_L2, ev.value);
                    }
                    if (ev.code == l2) {
                        set_key_bit(KEY_BIT_R2, ev.value);
                    }
#endif

#if defined(MINI) || defined(UT)
                    if (ev.code == r2) {
                        set_key_bit(KEY_BIT_L2, ev.value);
                    }
                    if (ev.code == l2) {
                        set_key_bit(KEY_BIT_R2, ev.value);
                    }
#endif

                    switch (ev.code) {
                    case DEV_KEY_CODE_START:
                        set_key_bit(KEY_BIT_START, ev.value);
                        break;
                    case DEV_KEY_CODE_SELECT:
                        set_key_bit(KEY_BIT_SELECT, ev.value);
                        break;
                    case DEV_KEY_CODE_MENU:
                        set_key_bit(KEY_BIT_MENU, ev.value);
                        break;
#if defined(MINI) || defined(UT)
                    case DEV_KEY_CODE_POWER:
                        set_key_bit(KEY_BIT_POWER, ev.value);
                        break;
                    case DEV_KEY_CODE_VOLUP:
                        set_key_bit(KEY_BIT_VOLUP, ev.value);
                        if (myevent.stock_os) {
                            if (ev.value == 0) {
                                nds.volume = volume_inc();
                            }
                        }
                        else {
                            nds.defer_update_bg = 60;
                        }
                        break;
                    case DEV_KEY_CODE_VOLDOWN:
                        set_key_bit(KEY_BIT_VOLDOWN, ev.value);
                        if (myevent.stock_os) {
                            if (ev.value == 0) {
                                nds.volume = volume_dec();
                            }
                        }
                        else {
                            nds.defer_update_bg = 60;
                        }
                        break;
#endif

#if defined(A30)
                    case DEV_KEY_CODE_VOLUP:
                        set_key_bit(KEY_BIT_VOLUP, ev.value);
                        if (ev.value == 0) {
                            nds.volume = volume_inc();
                        }
                        break;
                    case DEV_KEY_CODE_VOLDOWN:
                        set_key_bit(KEY_BIT_VOLDOWN, ev.value);
                        if (ev.value == 0) {
                            nds.volume = volume_dec();
                        }
                        break;
#endif
                    }
                }
            }

#if defined(A30) || defined(UT)
            r |= check_joystick_status();
#endif
            if (r > 0) {
                handle_hotkey();
            }
        }
        SDL_SemPost(myevent.lock);
        usleep(150000);
    }
    
    return 0;
}

void EventInit(void)
{
#if defined(MINI) || defined(UT)
    DIR *dir = NULL;

    myevent.stock_os = 1;
    dir = opendir("/mnt/SDCARD/.tmp_update");
    if (dir) {
        closedir(dir);
        myevent.stock_os = 0;
    }
#endif

    myevent.pen.max_x = NDS_W;
    myevent.pen.max_y = NDS_H;
    myevent.pen.x = myevent.pen.max_x >> 1;
    myevent.pen.y = myevent.pen.max_y >> 1;
    myevent.dev.mode = DEV_MODE_KEY;

    myevent.dev.fd = open(INPUT_DEV, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if(myevent.dev.fd < 0){
        err(SDL"failed to open \"%s\" in%s\n", INPUT_DEV, __func__);
        exit(-1);
    }

    myevent.lock = SDL_CreateSemaphore(1);
    if(myevent.lock == NULL) {
        err(SDL"failed to create input semaphore in %s\n", __func__);
        exit(-1);
    }

    myevent.key.report_key[KEY_BIT_UP] = SDLK_UP;
    myevent.key.report_key[KEY_BIT_DOWN] = SDLK_DOWN;
    myevent.key.report_key[KEY_BIT_LEFT] = SDLK_LEFT;
    myevent.key.report_key[KEY_BIT_RIGHT] = SDLK_RIGHT;
    myevent.key.report_key[KEY_BIT_A] = SDLK_SPACE;
    myevent.key.report_key[KEY_BIT_B] = SDLK_LCTRL;
    myevent.key.report_key[KEY_BIT_X] = SDLK_LSHIFT;
    myevent.key.report_key[KEY_BIT_Y] = SDLK_LALT;
    myevent.key.report_key[KEY_BIT_L1] = SDLK_e;
    myevent.key.report_key[KEY_BIT_R1] = SDLK_t;
    myevent.key.report_key[KEY_BIT_L2] = SDLK_TAB;
    myevent.key.report_key[KEY_BIT_R2] = SDLK_BACKSPACE;
    myevent.key.report_key[KEY_BIT_SELECT] = SDLK_RCTRL;
    myevent.key.report_key[KEY_BIT_START] = SDLK_RETURN;
    myevent.key.report_key[KEY_BIT_MENU] = SDLK_HOME;
    myevent.key.report_key[KEY_BIT_QSAVE] = SDLK_0;
    myevent.key.report_key[KEY_BIT_QLOAD] = SDLK_1;
    myevent.key.report_key[KEY_BIT_FF] = SDLK_2;
    myevent.key.report_key[KEY_BIT_EXIT] = SDLK_3;
    myevent.key.report_key[KEY_BIT_MENU_ONION] = SDLK_HOME;

#if defined(A30) || defined(UT)
    myevent.joy.threshold.up = DEF_THRESHOLD_UP;
    myevent.joy.threshold.down = DEF_THRESHOLD_DOWN;
    myevent.joy.threshold.left = DEF_THRESHOLD_LEFT;
    myevent.joy.threshold.right = DEF_THRESHOLD_RIGHT;
#endif

    myevent.thread = SDL_CreateThreadInternal(
        input_handler,
        "Miyoo Input Thread",
        4096,
        NULL
    );
    if(myevent.thread == NULL) {
        err(SDL"failed to create input thread in %s\n", __func__);
        exit(-1);
    }
}

void EventDeinit(void)
{
    myevent.running = 0;
    if (myevent.thread) {
        SDL_WaitThread(myevent.thread, NULL);
        myevent.thread = NULL;
    }

    if (myevent.lock) {
        SDL_DestroySemaphore(myevent.lock);
        myevent.lock = NULL;
    }

    if(myevent.dev.fd > 0) {
        close(myevent.dev.fd);
        myevent.dev.fd = -1;
    }
}

void PumpEvents(_THIS)
{
    SDL_SemWait(myevent.lock);
    if (nds.menu.enable) {
        int cc = 0;
        uint32_t bit = 0;
        uint32_t changed = myevent.key.pre_bits ^ myevent.key.cur_bits;

        for (cc = 0; cc <= KEY_BIT_LAST; cc++) {
            bit = 1 << cc;
            if (changed & bit) {
                if ((myevent.key.cur_bits & bit) == 0) {
                    handle_menu(cc);
                }
            }
        }
        myevent.key.pre_bits = myevent.key.cur_bits;
    }
    else {
        if (myevent.dev.mode == DEV_MODE_KEY) {
            if (myevent.key.pre_bits != myevent.key.cur_bits) {
                int cc = 0;
                uint32_t bit = 0;
                uint32_t changed = myevent.key.pre_bits ^ myevent.key.cur_bits;

                for (cc = 0; cc <= KEY_BIT_LAST; cc++) {
                    bit = 1 << cc;

                    if ((mycfg.key.hotkey == _key__hotkey_menu) &&
                        (cc == KEY_BIT_MENU))
                    {
                        continue;
                    }

                    if (changed & bit) {
                        SDL_SendKeyboardKey((myevent.key.cur_bits & bit) ?
                            SDL_PRESSED : SDL_RELEASED,
                            SDL_GetScancodeFromKey(myevent.key.report_key[cc]));
                    }
                }

                if (myevent.key.pre_bits & (1 << KEY_BIT_QSAVE)) {
                    nds.state |= NDS_STATE_QSAVE;
                    set_key_bit(KEY_BIT_QSAVE, 0);
                }
                if (myevent.key.pre_bits & (1 << KEY_BIT_QLOAD)) {
                    nds.state |= NDS_STATE_QLOAD;
                    set_key_bit(KEY_BIT_QLOAD, 0);
                }
                if (myevent.key.pre_bits & (1 << KEY_BIT_FF)) {
                    nds.state |= NDS_STATE_FF;
                    set_key_bit(KEY_BIT_FF, 0);
                }
                if (myevent.key.pre_bits & (1 << KEY_BIT_MENU_ONION)) {
                    set_key_bit(KEY_BIT_MENU_ONION, 0);
                }
                if (myevent.key.pre_bits & (1 << KEY_BIT_EXIT)) {
                    release_all_report_keys();
                }
                myevent.key.pre_bits = myevent.key.cur_bits;
            }
        }
        else {
            int updated = 0;
            
            if (myevent.key.pre_bits != myevent.key.cur_bits) {
                uint32_t cc = 0;
                uint32_t bit = 0;
                uint32_t changed = myevent.key.pre_bits ^ myevent.key.cur_bits;

                if (changed & (1 << KEY_BIT_A)) {
                    SDL_SendMouseButton(vid.window, 0, (myevent.key.cur_bits &
                        (1 << KEY_BIT_A)) ? SDL_PRESSED :
                        SDL_RELEASED, SDL_BUTTON_LEFT);
                }

                for (cc = 0; cc <= KEY_BIT_LAST; cc++) {
                    bit = 1 << cc;
                    if ((cc == KEY_BIT_FF) ||
                        (cc == KEY_BIT_QSAVE) ||
                        (cc == KEY_BIT_QLOAD) ||
                        (cc == KEY_BIT_EXIT) ||
                        (cc == KEY_BIT_R2))
                    {
                        if (changed & bit) {
                            SDL_SendKeyboardKey((myevent.key.cur_bits & bit) ?
                                SDL_PRESSED : SDL_RELEASED,
                                SDL_GetScancodeFromKey(
                                    myevent.key.report_key[cc]));
                        }
                    }
                    if (cc == KEY_BIT_R1) {
                        if (changed & bit) {
                            myevent.pen.lower_speed =
                                (myevent.key.cur_bits & bit);
                        }
                    }
                }
            }

            if (portrait_screen_layout(nds.dis_mode) && (nds.keys_rotate == 0)) {
                if (myevent.key.cur_bits & (1 << KEY_BIT_UP)) {
                    updated = 1;
                    myevent.pen.x+= get_movement_interval(MOVE_DIR_UP_DOWN);
                }
                if (myevent.key.cur_bits & (1 << KEY_BIT_DOWN)) {
                    updated = 1;
                    myevent.pen.x-= get_movement_interval(MOVE_DIR_UP_DOWN);
                }
                if (myevent.key.cur_bits & (1 << KEY_BIT_LEFT)) {
                    updated = 1;
                    myevent.pen.y-= get_movement_interval(MOVE_DIR_LEFT_RIGHT);
                }
                if (myevent.key.cur_bits & (1 << KEY_BIT_RIGHT)) {
                    updated = 1;
                    myevent.pen.y+= get_movement_interval(MOVE_DIR_LEFT_RIGHT);
                }
            }
            else {
                if (myevent.key.cur_bits & (1 << KEY_BIT_UP)) {
                    updated = 1;
                    myevent.pen.y-= get_movement_interval(MOVE_DIR_UP_DOWN);
                }
                if (myevent.key.cur_bits & (1 << KEY_BIT_DOWN)) {
                    updated = 1;
                    myevent.pen.y+= get_movement_interval(MOVE_DIR_UP_DOWN);
                }
                if (myevent.key.cur_bits & (1 << KEY_BIT_LEFT)) {
                    updated = 1;
                    myevent.pen.x-= get_movement_interval(MOVE_DIR_LEFT_RIGHT);
                }
                if (myevent.key.cur_bits & (1 << KEY_BIT_RIGHT)) {
                    updated = 1;
                    myevent.pen.x+= get_movement_interval(MOVE_DIR_LEFT_RIGHT);
                }
            }
            rectify_pen_position();

            if(updated){
                int x = 0;
                int y = 0;

                x = (myevent.pen.x * 160) / myevent.pen.max_x;
                y = (myevent.pen.y * 120) / myevent.pen.max_y;
                SDL_SendMouseMotion(vid.window, 0, 0,
                    x + 80, y + (nds.pen.pos ? 120 : 0));
            }

            if (myevent.key.pre_bits & (1 << KEY_BIT_QSAVE)) {
                set_key_bit(KEY_BIT_QSAVE, 0);
            }
            if (myevent.key.pre_bits & (1 << KEY_BIT_QLOAD)) {
                set_key_bit(KEY_BIT_QLOAD, 0);
            }
            if (myevent.key.pre_bits & (1 << KEY_BIT_FF)) {
                set_key_bit(KEY_BIT_FF, 0);
            }
            if (myevent.key.pre_bits & (1 << KEY_BIT_EXIT)) {
                release_all_report_keys();
            }
            myevent.key.pre_bits = myevent.key.cur_bits;
        }
    }
    SDL_SemPost(myevent.lock);
}

#if defined(UT)
TEST_GROUP_RUNNER(sdl2_event_miyoo)
{
RUN_TEST_CASE(sdl2_event_miyoo, rectify_pen_position);
RUN_TEST_CASE(sdl2_event_miyoo, portrait_screen_layout);
RUN_TEST_CASE(sdl2_event_miyoo, get_movement_interval);
RUN_TEST_CASE(sdl2_event_miyoo, release_all_report_keys);
RUN_TEST_CASE(sdl2_event_miyoo, hit_hotkey);
RUN_TEST_CASE(sdl2_event_miyoo, set_key_bit);
RUN_TEST_CASE(sdl2_event_miyoo, update_joystick_pen);
}
#endif

