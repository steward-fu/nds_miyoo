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

#include <time.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <json-c/json.h>
#include <sys/stat.h>
#include <pb_encode.h>
#include <pb_decode.h>

#if defined(UT)
#include "unity_fixture.h"
#endif

#include "log.h"
#include "cfg.h"
#include "snd.h"
#include "cfg.pb.h"

char home_path[MAX_PATH] = { 0 };
static char cfg_path[MAX_PATH * 2] = { 0 };
static settings cfg = settings_init_zero;

#if defined(UT)
TEST_GROUP(common_cfg);

TEST_SETUP(common_cfg)
{
    FILE *f = NULL;

    f = fopen(JSON_SYS_FILE, "w+");
    if (f) {
        fprintf(f, "{\n");
        fprintf(f, "\"vol\": 100\n");
        fprintf(f, "}");
        fclose(f);
    }
    init_config_settings();
}

TEST_TEAR_DOWN(common_cfg)
{
    unlink(JSON_SYS_FILE);
}
#endif

int get_system_volume(void)
{
    struct json_object *jval = NULL;
    struct json_object *jfile = NULL;

    jfile = json_object_from_file(JSON_SYS_PATH);
    if (jfile ==  NULL) {
        err(COM"failed to open file(\"%s\") in %s\n", JSON_SYS_PATH, __func__);
        return -1;
    }

    if (json_object_object_get_ex(jfile, JSON_SYS_VOLUME, &jval)) {
        cfg.system_volume = json_object_get_int(jval);
        info(COM"read system volume(%d) in %s\n", cfg.system_volume, __func__);
    }
    else {
        err(COM"failed to read system volume in %s\n", __func__);
    }
    json_object_put(jfile);
    return cfg.system_volume;
}

#if defined(UT)
TEST(common_cfg, get_system_volume)
{
    set_system_volume(1);
    TEST_ASSERT_EQUAL_INT(1, get_system_volume());
    TEST_ASSERT_EQUAL_INT(1, cfg.system_volume);
}
#endif

int set_system_volume(int vol)
{
    struct json_object *jval = NULL;
    struct json_object *jfile = NULL;

    if ((vol < 0) || (vol > MAX_VOLUME)) {
        err(COM"invalid parameter(vol:%d) in %s\n", vol, __func__);
        return -1;
    }

    jfile = json_object_from_file(JSON_SYS_PATH);
    if (jfile ==  NULL) {
        err(COM"failed to open file(\"%s\") in %s\n", JSON_SYS_PATH, __func__);
        return -1;
    }

    json_object_object_add(jfile, JSON_SYS_VOLUME, json_object_new_int(vol));
    info(COM"wrote new system volume(%d) in %s\n", vol, __func__);

    json_object_to_file_ext(JSON_SYS_PATH, jfile, JSON_C_TO_STRING_PRETTY);
    json_object_put(jfile);
    return vol;
}

#if defined(UT)
TEST(common_cfg, set_system_volume)
{
    TEST_ASSERT_EQUAL_INT(-1, set_system_volume(-1));
    TEST_ASSERT_EQUAL_INT(0, set_system_volume(0));
    TEST_ASSERT_EQUAL_INT(1, set_system_volume(1));
    TEST_ASSERT_EQUAL_INT(-1, set_system_volume(MAX_VOLUME + 1));
}
#endif

int load_config_settings(void)
{
    int ret = -1;
    uint8_t *buf = malloc(MAX_MALLOC_SIZE);

    if (!buf) {
        err(COM"failed to allocate memory in %s\n", __func__);
        return -1;
    }
    memset(buf, 0, MAX_MALLOC_SIZE);

    int fd = open(cfg_path, O_RDONLY);
    do {
        if (fd < 0) {
            err(COM"failed to open file(\"%s\") in %s\n", cfg_path, __func__);
            break;
        }

        ssize_t r = read(fd, buf, MAX_MALLOC_SIZE);
        if (r < 0) {
            err(COM"failed to read file(\"%s\") in %s\n", cfg_path, __func__);
        }
        close(fd);
        info(COM"read %ld bytes from \"%s\" in %s\n", r, cfg_path, __func__);

        pb_istream_t stream = pb_istream_from_buffer(buf, r);
        pb_decode(&stream, settings_fields, &cfg);
        ret = 0;
    } while (0);

    free(buf);
    return ret;
}

#if defined(UT)
TEST(common_cfg, load_config_settings)
{
    TEST_ASSERT_EQUAL_INT(0, load_config_settings());
    TEST_ASSERT_EQUAL_STRING(DEF_CFG_VERSION, cfg.version);
}
#endif

int update_config_settings(void)
{
    int fd = -1;
    int ret = -1;
    uint8_t *buf = malloc(MAX_MALLOC_SIZE);

    if (!buf) {
        err(COM"failed to allocate memory in %s\n", __func__);
        return ret;
    }

    memset(buf, 0, MAX_MALLOC_SIZE);
    pb_ostream_t stream = pb_ostream_from_buffer(buf, MAX_MALLOC_SIZE);
    cfg.has_display = true;
    cfg.display.has_small = true;
    cfg.has_cpu = true;
    cfg.cpu.has_freq = true;
    cfg.cpu.has_core = true;
    cfg.has_pen = true;
    cfg.pen.has_speed = true;
    cfg.has_menu = true;
    cfg.has_autosave = true;
    cfg.has_keypad = true;
    cfg.keypad.has_swap = true;
    cfg.has_joystick = true;
    cfg.joystick.has_remap_left = true;
    cfg.joystick.has_remap_right = true;
    pb_encode(&stream, settings_fields, &cfg);

    unlink(cfg_path);
    fd = open(cfg_path, O_CREAT | O_WRONLY, 0644);
    do {
        if (fd < 0) {
            err(COM"failed to create file(\"%s\") in %s\n", cfg_path, __func__);
            break;
        }

        ssize_t r = write(fd, buf, stream.bytes_written);
        if (r < 0) {
            err(COM"failed to write file(\"%s\") in %s\n", cfg_path, __func__);
        }

        info(COM"wrote %ld bytes to \"%s\" in %s\n", r, cfg_path, __func__);
        close(fd);
        ret = 0;
    } while (0);

    free(buf);
    return ret;
}

#if defined(UT)
TEST(common_cfg, update_config_settings)
{
    strncpy(cfg.version, "XXX", sizeof(cfg.version));
    cfg.low_battery_close = true;
    cfg.display.small.alpha = 11;
    cfg.cpu.freq.min = 22;
    cfg.cpu.core.min = 33;
    cfg.pen.speed.x = 44;
    strncpy(cfg.menu.bg, "YYY", sizeof(cfg.menu.bg));
    cfg.autosave.slot = 55;
    cfg.keypad.swap.l1_l2 = true;
    cfg.joystick.remap_left.top = 66;

    TEST_ASSERT_EQUAL_INT(0, update_config_settings());
    TEST_ASSERT_EQUAL_INT(0, load_config_settings());

    TEST_ASSERT_EQUAL_STRING("XXX", cfg.version);
    TEST_ASSERT_EQUAL_INT(true, cfg.low_battery_close);
    TEST_ASSERT_EQUAL_INT(11, cfg.display.small.alpha);
    TEST_ASSERT_EQUAL_INT(22, cfg.cpu.freq.min);
    TEST_ASSERT_EQUAL_INT(33, cfg.cpu.core.min);
    TEST_ASSERT_EQUAL_INT(44, cfg.pen.speed.x);
    TEST_ASSERT_EQUAL_STRING("YYY", cfg.menu.bg);
    TEST_ASSERT_EQUAL_INT(55, cfg.autosave.slot);
    TEST_ASSERT_EQUAL_INT(true, cfg.keypad.swap.l1_l2);
    TEST_ASSERT_EQUAL_INT(66, cfg.joystick.remap_left.top);
    
    TEST_ASSERT_EQUAL_INT(0, reset_config_settings());
    TEST_ASSERT_EQUAL_INT(0, update_config_settings());
}
#endif

int init_config_settings(void)
{
    getcwd(home_path, sizeof(home_path));

#if defined(UT)
    const char *path = "/../drastic";
    strncat(home_path, path, strlen(path));
#endif

    snprintf(cfg_path, sizeof(cfg_path), "%s/%s", home_path, CFG_PATH);
    info(COM"home path(\"%s\") in %s\n", home_path, __func__);
    info(COM"config path(\"%s\") in %s\n", cfg_path, __func__);

    if (load_config_settings() < 0) {
        warn(COM"failed to load config setting in %s\n", __func__);

        info(COM"reset all config settings back to default in %s\n", __func__);
        reset_config_settings();
        update_config_settings();
    }

    if (strcmp(DEF_CFG_VERSION, cfg.version)) {
        warn(COM"invalid version found in \"%s\" in %s\n", cfg_path, __func__);

        info(COM"reset all config settings back to default in %s\n", __func__);
        reset_config_settings();
        update_config_settings();
    }

    set_debug_level(cfg.debug_level);
    return 0;
}

#if defined(UT)
TEST(common_cfg, init_config_settings)
{
    TEST_ASSERT_EQUAL_INT(0, init_config_settings());
}
#endif

int reset_config_settings(void)
{
    strncpy(cfg.version, DEF_CFG_VERSION, sizeof(cfg.version));
    strncpy(cfg.language, DEF_CFG_LANGUAGE, sizeof(cfg.language));
    strncpy(cfg.menu.bg, DEF_CFG_MENU_BG, sizeof(cfg.menu.bg));
    strncpy(cfg.pen.image, DEF_CFG_PEN_IMAGE, sizeof(cfg.pen.image));
    strncpy(cfg.font_path, DEF_CFG_FONT_PATH, sizeof(cfg.font_path));
    strncpy(cfg.state_folder, DEF_CFG_STATE_FOLDER, sizeof(cfg.state_folder));
    strncpy(cfg.border_image, DEF_CFG_BORDER_IMAGE, sizeof(cfg.border_image));

    cfg.system_volume = 0;
    cfg.fast_forward = DEF_CFG_FAST_FORWARD;

    cfg.half_volume = DEF_CFG_HALF_VOLUME;
    cfg.low_battery_close = DEF_CFG_LOW_BATTERY_CLOSE;

    cfg.display.layout = DEF_CFG_DISPLAY_LAYOUT;
    cfg.display.alt_layout = DEF_CFG_DISPLAY_ALT_LAYOUT;
    cfg.display.small.alpha = DEF_CFG_DISPLAY_SMALL_ALPHA;
    cfg.display.small.border = DEF_CFG_DISPLAY_SMALL_BORDER;
    cfg.display.small.position = DEF_CFG_DISPLAY_SMALL_POSITION;

    cfg.cpu.freq.min = DEF_CFG_CPU_FREQ_MIN;
    cfg.cpu.freq.max = DEF_CFG_CPU_FREQ_MAX;
    cfg.cpu.core.min = DEF_CFG_CPU_CORE_MIN;
    cfg.cpu.core.max = DEF_CFG_CPU_CORE_MAX;

    cfg.pen.screen0 = DEF_CFG_PEN_SCREEN0;
    cfg.pen.speed.x = DEF_CFG_PEN_SPEED_X;
    cfg.pen.speed.y = DEF_CFG_PEN_SPEED_Y;

    cfg.menu.show_cursor = DEF_CFG_MENU_SHOW_CURSOR;

    cfg.autosave.enable = DEF_CFG_AUTOSAVE_ENABLE;
    cfg.autosave.slot = DEF_CFG_AUTOSAVE_SLOT;

    cfg.keypad.rotate = DEF_CFG_KEYPAD_ROTATE;
    cfg.keypad.hotkey = DEF_CFG_KEYPAD_HOTKEY;
    cfg.keypad.swap.l1_l2 = DEF_CFG_KEYPAD_SWAP_L1_L2;
    cfg.keypad.swap.r1_r2 = DEF_CFG_KEYPAD_SWAP_R1_R2;

    cfg.joystick.mode = DEF_CFG_JOYSTICK_MODE;
    cfg.joystick.dead_zone = DEF_CFG_JOYSTICK_DEAD_ZONE;
    cfg.joystick.remap_left.top = DEF_CFG_JOYSTICK_REMAP_LEFT_TOP;
    cfg.joystick.remap_left.down = DEF_CFG_JOYSTICK_REMAP_LEFT_DOWN;
    cfg.joystick.remap_left.left = DEF_CFG_JOYSTICK_REMAP_LEFT_LEFT;
    cfg.joystick.remap_left.right = DEF_CFG_JOYSTICK_REMAP_LEFT_RIGHT;
    cfg.joystick.remap_right.top = DEF_CFG_JOYSTICK_REMAP_RIGHT_TOP;
    cfg.joystick.remap_right.down = DEF_CFG_JOYSTICK_REMAP_RIGHT_DOWN;
    cfg.joystick.remap_right.left = DEF_CFG_JOYSTICK_REMAP_RIGHT_LEFT;
    cfg.joystick.remap_right.right = DEF_CFG_JOYSTICK_REMAP_RIGHT_RIGHT;

    cfg.debug_level = DEF_CFG_DEBUG_LEVEL;

    if (get_system_volume() < 0) {
        return -1;
    }
    return 0;
}

#if defined(UT)
TEST(common_cfg, reset_config_settings)
{
    TEST_ASSERT_EQUAL_INT(0, reset_config_settings());
    TEST_ASSERT_EQUAL_INT(0, update_config_settings());
    TEST_ASSERT_EQUAL_INT(0, load_config_settings());

    TEST_ASSERT_EQUAL_STRING(DEF_CFG_VERSION, cfg.version);
    TEST_ASSERT_EQUAL_STRING(DEF_CFG_LANGUAGE, cfg.language);
    TEST_ASSERT_EQUAL_STRING(DEF_CFG_MENU_BG, cfg.menu.bg);
    TEST_ASSERT_EQUAL_STRING(DEF_CFG_PEN_IMAGE, cfg.pen.image);
    TEST_ASSERT_EQUAL_STRING(DEF_CFG_FONT_PATH, cfg.font_path);
    TEST_ASSERT_EQUAL_STRING(DEF_CFG_STATE_FOLDER, cfg.state_folder);
    TEST_ASSERT_EQUAL_STRING(DEF_CFG_BORDER_IMAGE, cfg.border_image);

    TEST_ASSERT_EQUAL_INT(DEF_CFG_FAST_FORWARD, cfg.fast_forward);

    TEST_ASSERT_EQUAL_INT(DEF_CFG_HALF_VOLUME, cfg.half_volume);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_LOW_BATTERY_CLOSE, cfg.low_battery_close);

    TEST_ASSERT_EQUAL_INT(DEF_CFG_DISPLAY_LAYOUT, cfg.display.layout);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_DISPLAY_ALT_LAYOUT, cfg.display.alt_layout);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_DISPLAY_SMALL_ALPHA, cfg.display.small.alpha);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_DISPLAY_SMALL_BORDER, cfg.display.small.border);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_DISPLAY_SMALL_POSITION, cfg.display.small.position);

    TEST_ASSERT_EQUAL_INT(DEF_CFG_CPU_FREQ_MIN, cfg.cpu.freq.min);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_CPU_FREQ_MAX, cfg.cpu.freq.max);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_CPU_CORE_MIN, cfg.cpu.core.min);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_CPU_CORE_MAX, cfg.cpu.core.max);

    TEST_ASSERT_EQUAL_INT(DEF_CFG_PEN_SCREEN0, cfg.pen.screen0);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_PEN_SPEED_X, cfg.pen.speed.x);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_PEN_SPEED_Y, cfg.pen.speed.y);

    TEST_ASSERT_EQUAL_INT(DEF_CFG_MENU_SHOW_CURSOR, cfg.menu.show_cursor);

    TEST_ASSERT_EQUAL_INT(DEF_CFG_AUTOSAVE_ENABLE, cfg.autosave.enable);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_AUTOSAVE_SLOT, cfg.autosave.slot);

    TEST_ASSERT_EQUAL_INT(DEF_CFG_KEYPAD_ROTATE, cfg.keypad.rotate);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_KEYPAD_HOTKEY, cfg.keypad.hotkey);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_KEYPAD_SWAP_L1_L2, cfg.keypad.swap.l1_l2);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_KEYPAD_SWAP_R1_R2, cfg.keypad.swap.r1_r2);

    TEST_ASSERT_EQUAL_INT(DEF_CFG_JOYSTICK_MODE, cfg.joystick.mode);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_JOYSTICK_DEAD_ZONE, cfg.joystick.dead_zone);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_JOYSTICK_REMAP_LEFT_TOP, cfg.joystick.remap_left.top);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_JOYSTICK_REMAP_LEFT_DOWN, cfg.joystick.remap_left.down);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_JOYSTICK_REMAP_LEFT_LEFT, cfg.joystick.remap_left.left);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_JOYSTICK_REMAP_LEFT_RIGHT, cfg.joystick.remap_left.right);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_JOYSTICK_REMAP_RIGHT_TOP, cfg.joystick.remap_right.top);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_JOYSTICK_REMAP_RIGHT_DOWN, cfg.joystick.remap_right.down);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_JOYSTICK_REMAP_RIGHT_LEFT, cfg.joystick.remap_right.left);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_JOYSTICK_REMAP_RIGHT_RIGHT, cfg.joystick.remap_right.right);

    TEST_ASSERT_EQUAL_INT(DEF_CFG_DEBUG_LEVEL, cfg.debug_level);
}
#endif

int get_cfg_half_volume(void)
{
    return cfg.half_volume;
}

#if defined(UT)
TEST(common_cfg, get_cfg_half_volume)
{
    TEST_ASSERT_EQUAL_INT(0, reset_config_settings());
    TEST_ASSERT_EQUAL_INT(0, get_cfg_half_volume());
}
#endif

int get_cfg_autosave_enable(void)
{
    return cfg.autosave.enable;
}

#if defined(UT)
TEST(common_cfg, get_cfg_autosave_enable)
{
    cfg.autosave.enable = 0;
    TEST_ASSERT_EQUAL_INT(0, get_cfg_autosave_enable());

    cfg.autosave.enable = 1;
    TEST_ASSERT_EQUAL_INT(1, get_cfg_autosave_enable());
}
#endif

int get_cfg_autosave_slot(void)
{
    return cfg.autosave.slot;
}

#if defined(UT)
TEST(common_cfg, get_cfg_autosave_slot)
{
    cfg.autosave.slot = 0;
    TEST_ASSERT_EQUAL_INT(0, get_cfg_autosave_slot());

    cfg.autosave.slot = 10;
    TEST_ASSERT_EQUAL_INT(10, get_cfg_autosave_slot());
}
#endif

int get_cfg_pen_speed_x(void)
{
    return cfg.pen.speed.x;
}

#if defined(UT)
TEST(common_cfg, get_cfg_pen_speed_x)
{
    cfg.pen.speed.x = 100;
    TEST_ASSERT_EQUAL_INT(100, get_cfg_pen_speed_x());
}
#endif

int get_cfg_pen_speed_y(void)
{
    return cfg.pen.speed.y;
}

#if defined(UT)
TEST(common_cfg, get_cfg_pen_speed_y)
{
    cfg.pen.speed.y = 100;
    TEST_ASSERT_EQUAL_INT(100, get_cfg_pen_speed_y());
}
#endif

hotkey_t get_cfg_keypad_hotkey(void)
{
    return cfg.keypad.hotkey;
}

#if defined(UT)
TEST(common_cfg, get_cfg_keypad_hotkey)
{
    cfg.keypad.hotkey = HOTKEY_MENU;
    TEST_ASSERT_EQUAL_INT(HOTKEY_MENU, get_cfg_keypad_hotkey());

    cfg.keypad.hotkey = HOTKEY_SELECT;
    TEST_ASSERT_EQUAL_INT(HOTKEY_SELECT, get_cfg_keypad_hotkey());
}
#endif

#if defined(UT)
TEST_GROUP_RUNNER(common_cfg)
{
    RUN_TEST_CASE(common_cfg, get_system_volume);
    RUN_TEST_CASE(common_cfg, set_system_volume);
    RUN_TEST_CASE(common_cfg, load_config_settings);
    RUN_TEST_CASE(common_cfg, update_config_settings);
    RUN_TEST_CASE(common_cfg, init_config_settings);
    RUN_TEST_CASE(common_cfg, reset_config_settings);
    RUN_TEST_CASE(common_cfg, get_cfg_half_volume);
    RUN_TEST_CASE(common_cfg, get_cfg_autosave_enable);
    RUN_TEST_CASE(common_cfg, get_cfg_autosave_slot);
    RUN_TEST_CASE(common_cfg, get_cfg_pen_speed_x);
    RUN_TEST_CASE(common_cfg, get_cfg_pen_speed_y);
    RUN_TEST_CASE(common_cfg, get_cfg_keypad_hotkey);
}
#endif

