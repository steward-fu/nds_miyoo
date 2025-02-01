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

#include <time.h>
#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <pb_encode.h>
#include <pb_decode.h>
#include <json-c/json.h>

#if defined(UT)
#include "unity_fixture.h"
#endif

#include "log.h"
#include "cfg.h"
#include "snd.h"
#include "cfg.pb.h"

static char cfg_path[MAX_PATH * 2] = { 0 };

miyoo_settings mycfg = miyoo_settings_init_zero;

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
        pb_decode(&stream, miyoo_settings_fields, &mycfg);
        ret = 0;
    } while (0);

    free(buf);
    return ret;
}

#if defined(UT)
TEST(common_cfg, load_config_settings)
{
    TEST_ASSERT_EQUAL_INT(0, load_config_settings());
    TEST_ASSERT_EQUAL_STRING(DEF_CFG_VERSION, mycfg.version);
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
    mycfg.has_display = true;
    mycfg.display.has_small = true;

    mycfg.has_cpu = true;
    mycfg.cpu.has_freq = true;
    mycfg.cpu.has_core = true;

    mycfg.has_pen = true;
    mycfg.pen.has_show = true;
    mycfg.pen.has_speed = true;

    mycfg.has_menu = true;
    mycfg.has_autosave = true;

    mycfg.has_key = true;
    mycfg.key.has_swap = true;

    mycfg.has_joy = true;
    mycfg.joy.has_left = true;
    mycfg.joy.has_right = true;
    mycfg.joy.left.has_x = true;
    mycfg.joy.left.has_y = true;
    mycfg.joy.left.has_remap = true;
    mycfg.joy.right.has_x = true;
    mycfg.joy.right.has_y = true;
    mycfg.joy.right.has_remap = true;
    pb_encode(&stream, miyoo_settings_fields, &mycfg);

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
    strncpy(mycfg.version, "XXX", sizeof(mycfg.version));
    mycfg.low_battery_close = true;
    mycfg.display.small.alpha = 11;
    mycfg.cpu.freq.min = 22;
    mycfg.cpu.core.min = 33;
    mycfg.pen.speed.x = 44;
    strncpy(mycfg.menu.bg, "YYY", sizeof(mycfg.menu.bg));
    mycfg.autosave.slot = 55;
    mycfg.key.swap.l1_l2 = true;
    mycfg.joy.left.remap.up = 66;
    mycfg.joy.right.remap.left = 77;

    TEST_ASSERT_EQUAL_INT(0, update_config_settings());
    TEST_ASSERT_EQUAL_INT(0, load_config_settings());

    TEST_ASSERT_EQUAL_STRING("XXX", mycfg.version);
    TEST_ASSERT_EQUAL_INT(true, mycfg.low_battery_close);
    TEST_ASSERT_EQUAL_INT(11, mycfg.display.small.alpha);
    TEST_ASSERT_EQUAL_INT(22, mycfg.cpu.freq.min);
    TEST_ASSERT_EQUAL_INT(33, mycfg.cpu.core.min);
    TEST_ASSERT_EQUAL_INT(44, mycfg.pen.speed.x);
    TEST_ASSERT_EQUAL_STRING("YYY", mycfg.menu.bg);
    TEST_ASSERT_EQUAL_INT(55, mycfg.autosave.slot);
    TEST_ASSERT_EQUAL_INT(true, mycfg.key.swap.l1_l2);
    TEST_ASSERT_EQUAL_INT(66, mycfg.joy.left.remap.up);
    TEST_ASSERT_EQUAL_INT(77, mycfg.joy.right.remap.left);
    
    TEST_ASSERT_EQUAL_INT(0, reset_config_settings());
    TEST_ASSERT_EQUAL_INT(0, update_config_settings());
}
#endif

int init_config_settings(void)
{
    getcwd(mycfg.home_folder, sizeof(mycfg.home_folder));

#if defined(UT)
    const char *path = "/../drastic";
    strncat(mycfg.home_folder, path, strlen(path));
#endif

    snprintf(cfg_path, sizeof(cfg_path), "%s/%s", mycfg.home_folder, CFG_PATH);
    info(COM"home path(\"%s\") in %s\n", mycfg.home_folder, __func__);
    info(COM"config path(\"%s\") in %s\n", cfg_path, __func__);

    if (load_config_settings() < 0) {
        warn(COM"failed to load config setting in %s\n", __func__);

        info(COM"reset all config settings back to default in %s\n", __func__);
        reset_config_settings();
        update_config_settings();
    }

    if (strcmp(DEF_CFG_VERSION, mycfg.version)) {
        warn(COM"invalid version found in \"%s\" in %s\n", cfg_path, __func__);

        info(COM"reset all config settings back to default in %s\n", __func__);
        reset_config_settings();
        update_config_settings();
    }

    set_debug_level(mycfg.debug_level);
    return 0;
}

int get_system_volume(void)
{
    struct json_object *jval = NULL;
    struct json_object *jfile = NULL;

    jfile = json_object_from_file(JSON_SYS_PATH);
    if (jfile == NULL) {
        err(COM"failed to open file(\"%s\") in %s\n", JSON_SYS_PATH, __func__);
        return -1;
    }

    if (json_object_object_get_ex(jfile, JSON_SYS_VOLUME, &jval)) {
        mycfg.system_volume = json_object_get_int(jval);
        info(COM"read system volume(%d) in %s\n", mycfg.system_volume, __func__);
    }
    else {
        err(COM"failed to read system volume in %s\n", __func__);
    }
    json_object_put(jfile);
    return mycfg.system_volume;
}

#if defined(UT)
TEST(common_cfg, get_system_volume)
{
    set_system_volume(1);
    TEST_ASSERT_EQUAL_INT(1, get_system_volume());
    TEST_ASSERT_EQUAL_INT(1, mycfg.system_volume);
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
    if (jfile == NULL) {
        err(COM"failed to open file(\"%s\") in %s\n", JSON_SYS_PATH, __func__);
        return -1;
    }

    json_object_object_add(jfile, JSON_SYS_VOLUME, json_object_new_int(vol));
    info(COM"wrote new system volume(%d) in %s\n", vol, __func__);

    json_object_to_file_ext(JSON_SYS_PATH, jfile, JSON_C_TO_STRING_PRETTY);
    json_object_put(jfile);
    mycfg.system_volume = vol;
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

#if defined(UT)
TEST(common_cfg, init_config_settings)
{
    TEST_ASSERT_EQUAL_INT(0, init_config_settings());
}
#endif

int reset_config_settings(void)
{
    strncpy(mycfg.version, DEF_CFG_VERSION, sizeof(mycfg.version));
    strncpy(mycfg.language, DEF_CFG_LANGUAGE, sizeof(mycfg.language));
    strncpy(mycfg.font_path, DEF_CFG_FONT_PATH, sizeof(mycfg.font_path));
    strncpy(mycfg.state_folder, DEF_CFG_STATE_FOLDER, sizeof(mycfg.state_folder));
    strncpy(mycfg.border_image, DEF_CFG_BORDER_IMAGE, sizeof(mycfg.border_image));

    mycfg.debug_level = DEF_CFG_DEBUG_LEVEL;
    mycfg.system_volume = 0;
    mycfg.fast_forward = DEF_CFG_FAST_FORWARD;
    mycfg.half_volume = DEF_CFG_HALF_VOLUME;
    mycfg.low_battery_close = DEF_CFG_LOW_BATTERY_CLOSE;

    mycfg.cpu.freq.min = DEF_CFG_CPU_FREQ_MIN;
    mycfg.cpu.freq.max = DEF_CFG_CPU_FREQ_MAX;
    mycfg.cpu.core.min = DEF_CFG_CPU_CORE_MIN;
    mycfg.cpu.core.max = DEF_CFG_CPU_CORE_MAX;

    strncpy(mycfg.menu.bg, DEF_CFG_MENU_BG, sizeof(mycfg.menu.bg));
    mycfg.menu.show_cursor = DEF_CFG_MENU_SHOW_CURSOR;

    mycfg.display.layout = DEF_CFG_DISPLAY_LAYOUT;
    mycfg.display.alt_layout = DEF_CFG_DISPLAY_ALT_LAYOUT;
    mycfg.display.small.alpha = DEF_CFG_DISPLAY_SMALL_ALPHA;
    mycfg.display.small.border = DEF_CFG_DISPLAY_SMALL_BORDER;
    mycfg.display.small.position = DEF_CFG_DISPLAY_SMALL_POSITION;

    mycfg.autosave.enable = DEF_CFG_AUTOSAVE_ENABLE;
    mycfg.autosave.slot = DEF_CFG_AUTOSAVE_SLOT;

    mycfg.pen.show.mode = DEF_CFG_PEN_SHOW_MODE;
    mycfg.pen.show.count = DEF_CFG_PEN_SHOW_COUNT;
    strncpy(mycfg.pen.image, DEF_CFG_PEN_IMAGE, sizeof(mycfg.pen.image));
    mycfg.pen.screen = DEF_CFG_PEN_SCREEN;
    mycfg.pen.speed.x = DEF_CFG_PEN_SPEED_X;
    mycfg.pen.speed.y = DEF_CFG_PEN_SPEED_Y;

    mycfg.key.rotate = DEF_CFG_KEY_ROTATE;
    mycfg.key.hotkey = DEF_CFG_KEY_HOTKEY;
    mycfg.key.swap.l1_l2 = DEF_CFG_KEY_SWAP_L1_L2;
    mycfg.key.swap.r1_r2 = DEF_CFG_KEY_SWAP_R1_R2;

    mycfg.joy.left.x.min = DEF_CFG_JOY_MIN;
    mycfg.joy.left.x.max = DEF_CFG_JOY_MAX;
    mycfg.joy.left.x.zero = DEF_CFG_JOY_ZERO;
    mycfg.joy.left.x.step = DEF_CFG_JOY_STEP;
    mycfg.joy.left.x.dead = DEF_CFG_JOY_DEAD;
    mycfg.joy.left.y.min = DEF_CFG_JOY_MIN;
    mycfg.joy.left.y.max = DEF_CFG_JOY_MAX;
    mycfg.joy.left.y.zero = DEF_CFG_JOY_ZERO;
    mycfg.joy.left.y.step = DEF_CFG_JOY_STEP;
    mycfg.joy.left.y.dead = DEF_CFG_JOY_DEAD;
    mycfg.joy.left.mode = DEF_CFG_JOY_MODE;
    mycfg.joy.left.remap.up = DEF_CFG_JOY_REMAP_UP;
    mycfg.joy.left.remap.down = DEF_CFG_JOY_REMAP_DOWN;
    mycfg.joy.left.remap.left = DEF_CFG_JOY_REMAP_LEFT;
    mycfg.joy.left.remap.right = DEF_CFG_JOY_REMAP_RIGHT;

    mycfg.joy.right.x.min = DEF_CFG_JOY_MIN;
    mycfg.joy.right.x.max = DEF_CFG_JOY_MAX;
    mycfg.joy.right.x.zero = DEF_CFG_JOY_ZERO;
    mycfg.joy.right.x.step = DEF_CFG_JOY_STEP;
    mycfg.joy.right.x.dead = DEF_CFG_JOY_DEAD;
    mycfg.joy.right.y.min = DEF_CFG_JOY_MIN;
    mycfg.joy.right.y.max = DEF_CFG_JOY_MAX;
    mycfg.joy.right.y.zero = DEF_CFG_JOY_ZERO;
    mycfg.joy.right.y.step = DEF_CFG_JOY_STEP;
    mycfg.joy.right.y.dead = DEF_CFG_JOY_DEAD;
    mycfg.joy.right.mode = DEF_CFG_JOY_MODE;
    mycfg.joy.right.remap.up = DEF_CFG_JOY_REMAP_UP;
    mycfg.joy.right.remap.down = DEF_CFG_JOY_REMAP_DOWN;
    mycfg.joy.right.remap.left = DEF_CFG_JOY_REMAP_LEFT;
    mycfg.joy.right.remap.right = DEF_CFG_JOY_REMAP_RIGHT;

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

    memset(&mycfg, 0, sizeof(mycfg));
    TEST_ASSERT_EQUAL_INT(0, load_config_settings());

    TEST_ASSERT_EQUAL_STRING(DEF_CFG_VERSION, mycfg.version);
    TEST_ASSERT_EQUAL_STRING(DEF_CFG_LANGUAGE, mycfg.language);
    TEST_ASSERT_EQUAL_STRING(DEF_CFG_FONT_PATH, mycfg.font_path);
    TEST_ASSERT_EQUAL_STRING(DEF_CFG_STATE_FOLDER, mycfg.state_folder);
    TEST_ASSERT_EQUAL_STRING(DEF_CFG_BORDER_IMAGE, mycfg.border_image);

    TEST_ASSERT_EQUAL_INT(DEF_CFG_DEBUG_LEVEL, mycfg.debug_level);
    TEST_ASSERT_EQUAL_INT(100, mycfg.system_volume);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_FAST_FORWARD, mycfg.fast_forward);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_HALF_VOLUME, mycfg.half_volume);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_LOW_BATTERY_CLOSE, mycfg.low_battery_close);

    TEST_ASSERT_EQUAL_INT(DEF_CFG_CPU_FREQ_MIN, mycfg.cpu.freq.min);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_CPU_FREQ_MAX, mycfg.cpu.freq.max);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_CPU_CORE_MIN, mycfg.cpu.core.min);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_CPU_CORE_MAX, mycfg.cpu.core.max);

    TEST_ASSERT_EQUAL_STRING(DEF_CFG_MENU_BG, mycfg.menu.bg);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_MENU_SHOW_CURSOR, mycfg.menu.show_cursor);

    TEST_ASSERT_EQUAL_INT(DEF_CFG_DISPLAY_LAYOUT, mycfg.display.layout);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_DISPLAY_ALT_LAYOUT, mycfg.display.alt_layout);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_DISPLAY_SMALL_ALPHA, mycfg.display.small.alpha);

    TEST_ASSERT_EQUAL_INT(DEF_CFG_DISPLAY_SMALL_BORDER, mycfg.display.small.border);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_DISPLAY_SMALL_POSITION, mycfg.display.small.position);

    TEST_ASSERT_EQUAL_INT(DEF_CFG_AUTOSAVE_ENABLE, mycfg.autosave.enable);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_AUTOSAVE_SLOT, mycfg.autosave.slot);

    TEST_ASSERT_EQUAL_INT(DEF_CFG_PEN_SHOW_MODE, mycfg.pen.show.mode);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_PEN_SHOW_COUNT, mycfg.pen.show.count);
    TEST_ASSERT_EQUAL_STRING(DEF_CFG_PEN_IMAGE, mycfg.pen.image);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_PEN_SCREEN, mycfg.pen.screen);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_PEN_SPEED_X, mycfg.pen.speed.x);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_PEN_SPEED_Y, mycfg.pen.speed.y);

    TEST_ASSERT_EQUAL_INT(DEF_CFG_KEY_ROTATE, mycfg.key.rotate);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_KEY_HOTKEY, mycfg.key.hotkey);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_KEY_SWAP_L1_L2, mycfg.key.swap.l1_l2);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_KEY_SWAP_R1_R2, mycfg.key.swap.r1_r2);

    TEST_ASSERT_EQUAL_INT(DEF_CFG_JOY_MIN, mycfg.joy.left.x.min);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_JOY_MAX, mycfg.joy.left.x.max);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_JOY_ZERO, mycfg.joy.left.x.zero);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_JOY_STEP, mycfg.joy.left.x.step);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_JOY_DEAD, mycfg.joy.left.x.dead);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_JOY_MIN, mycfg.joy.left.y.min);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_JOY_MAX, mycfg.joy.left.y.max);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_JOY_ZERO, mycfg.joy.left.y.zero);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_JOY_STEP, mycfg.joy.left.y.step);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_JOY_DEAD, mycfg.joy.left.y.dead);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_JOY_MODE, mycfg.joy.left.mode);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_JOY_REMAP_UP, mycfg.joy.left.remap.up);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_JOY_REMAP_DOWN, mycfg.joy.left.remap.down);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_JOY_REMAP_LEFT, mycfg.joy.left.remap.left);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_JOY_REMAP_RIGHT, mycfg.joy.left.remap.right);

    TEST_ASSERT_EQUAL_INT(DEF_CFG_JOY_MIN, mycfg.joy.right.x.min);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_JOY_MAX, mycfg.joy.right.x.max);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_JOY_ZERO, mycfg.joy.right.x.zero);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_JOY_STEP, mycfg.joy.right.x.step);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_JOY_DEAD, mycfg.joy.right.x.dead);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_JOY_MIN, mycfg.joy.right.y.min);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_JOY_MAX, mycfg.joy.right.y.max);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_JOY_ZERO, mycfg.joy.right.y.zero);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_JOY_STEP, mycfg.joy.right.y.step);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_JOY_DEAD, mycfg.joy.right.y.dead);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_JOY_MODE, mycfg.joy.right.mode);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_JOY_REMAP_UP, mycfg.joy.right.remap.up);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_JOY_REMAP_DOWN, mycfg.joy.right.remap.down);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_JOY_REMAP_LEFT, mycfg.joy.right.remap.left);
    TEST_ASSERT_EQUAL_INT(DEF_CFG_JOY_REMAP_RIGHT, mycfg.joy.right.remap.right);
}
#endif

#if defined(UT)
TEST_GROUP_RUNNER(common_cfg)
{
    RUN_TEST_CASE(common_cfg, load_config_settings);
    RUN_TEST_CASE(common_cfg, update_config_settings);
    RUN_TEST_CASE(common_cfg, get_system_volume);
    RUN_TEST_CASE(common_cfg, set_system_volume);
    RUN_TEST_CASE(common_cfg, init_config_settings);
    RUN_TEST_CASE(common_cfg, reset_config_settings);
}
#endif

