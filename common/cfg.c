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

#if defined(UT)
#include "unity_fixture.h"
#endif

#include "log.h"
#include "cfg.h"
#include "snd.h"
#include "file.h"

static struct json_config cfg = { 0 };
static struct json_config sys = { 0 };

char home_path[MAX_PATH] = { 0 };

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
    init_cfg();
}

TEST_TEAR_DOWN(common_cfg)
{
    unlink(JSON_SYS_FILE);
}
#endif

static int check_pointer_not_null_and_copy(char *dst, int dst_size, const char *src)
{
    struct stat st = { 0 };

    if (!dst || (dst_size < 0) || !src) {
        err(COM"invalid parameter in %s\n", __func__);
        return -1;
    }

    if (strlen(src) > dst_size) {
        err(COM"the length of path is too large in %s\n", __func__);
        return -1;
    }

    strncpy(dst, src, dst_size);
    return 0;
}

#if defined(UT)
TEST(common_cfg, check_pointer_not_null_and_copy)
{
    char buf[MAX_PATH] = { 0 };

    TEST_ASSERT_EQUAL_INT(-1, check_pointer_not_null_and_copy(NULL, 0, NULL));
    TEST_ASSERT_EQUAL_INT(-1, check_pointer_not_null_and_copy(buf, 0, "/tmp"));

    TEST_ASSERT_EQUAL_INT(0, check_pointer_not_null_and_copy(buf, sizeof(buf), "/tmp"));
    TEST_ASSERT_EQUAL_STRING("/tmp", buf);
}
#endif

static int check_value_max_and_set(int *dst, int val, int max_val)
{
    if (!dst) {
        err(COM"invalid parameter in %s\n", __func__);
        return -1;
    }

    if ((val < 0) || (val > max_val)) {
        err(COM"invalid value in %s(%d)\n", __func__, val);
        return -1;
    }

    *dst = val;
    return *dst;
}

static int check_value_min_and_set(int *dst, int val, int min_val)
{
    if (!dst) {
        err(COM"invalid parameter in %s\n", __func__);
        return -1;
    }

    if ((val < 0) || (val < min_val)) {
        err(COM"invalid value in %s(%d)\n", __func__, val);
        return -1;
    }

    *dst = val;
    return *dst;
}

#if defined(UT)
TEST(common_cfg, check_value_max_and_set)
{
    int v = 0;

    TEST_ASSERT_EQUAL_INT(-1, check_value_max_and_set(NULL, 0, 0));
    TEST_ASSERT_EQUAL_INT(-1, check_value_max_and_set(&v, 1, 0));
    TEST_ASSERT_EQUAL_INT(-1, check_value_max_and_set(&v, -1, 0));

    TEST_ASSERT_EQUAL_INT(1, check_value_max_and_set(&v, 1, 10));
    TEST_ASSERT_EQUAL_INT(1, v);
}
#endif

int set_home_path(const char *path)
{
    return check_absolute_folder_exist_and_copy(home_path, sizeof(home_path), path);
}

#if defined(UT)
TEST(common_cfg, set_home_path)
{
    TEST_ASSERT_EQUAL_INT(-1, set_home_path(NULL));
    TEST_ASSERT_EQUAL_INT(-1, set_home_path("/NOT_EXIST_PATH"));

    TEST_ASSERT_EQUAL_INT(0, set_home_path("/tmp"));
    TEST_ASSERT_EQUAL_STRING("/tmp", home_path);
}
#endif

int get_sys_volume(void)
{
    return sys.volume;
}

#if defined(UT)
TEST(common_cfg, get_sys_volume)
{
    set_sys_volume(1);
    TEST_ASSERT_EQUAL_INT(1, get_sys_volume());

    set_sys_volume(0);
    TEST_ASSERT_EQUAL_INT(0, get_sys_volume());
}
#endif

int set_sys_volume(int vol)
{
    return check_value_max_and_set(&sys.volume, vol, MAX_VOLUME);
}

#if defined(UT)
TEST(common_cfg, set_sys_volume)
{
    set_sys_volume(1);
    set_sys_volume(MAX_VOLUME + 1);
    TEST_ASSERT_EQUAL_INT(1, get_sys_volume());
    TEST_ASSERT_EQUAL_INT(1, sys.volume);

    set_sys_volume(0);
    set_sys_volume(-100);
    TEST_ASSERT_EQUAL_INT(0, get_sys_volume());
    TEST_ASSERT_EQUAL_INT(0, sys.volume);
}
#endif

int get_cfg_sdl_version(char *ret, int ret_size)
{
    return check_pointer_not_null_and_copy(ret, ret_size, cfg.sdl_version);
}

#if defined(UT)
TEST(common_cfg, get_cfg_sdl_version)
{
    char buf[MAX_PATH] = { 0 };

    TEST_ASSERT_EQUAL_INT(-1, get_cfg_sdl_version(NULL, 0));
    TEST_ASSERT_EQUAL_INT(-1, get_cfg_sdl_version(buf, 0));

    TEST_ASSERT_EQUAL_INT(0, get_cfg_sdl_version(buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING(CFG_SDL_VERSION, buf);
}
#endif

int get_cfg_font_style(char *ret, int ret_size)
{
    return check_pointer_not_null_and_copy(ret, ret_size, cfg.font_style);
}

#if defined(UT)
TEST(common_cfg, get_cfg_font_style)
{
    char buf[MAX_PATH] = { 0 };
    const char *fpath = DEFAULT_FONT_STYLE;

    TEST_ASSERT_EQUAL_INT(-1, get_cfg_font_style(NULL, 0));
    TEST_ASSERT_EQUAL_INT(-1, get_cfg_font_style(buf, 0));

    TEST_ASSERT_EQUAL_INT(0, set_cfg_font_style(fpath));
    TEST_ASSERT_EQUAL_INT(0, get_cfg_font_style(buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING(fpath, buf);
}
#endif

int set_cfg_font_style(const char *path)
{
    return check_resource_file_exist_and_copy(cfg.font_style, sizeof(cfg.font_style), path);
}

#if defined(UT)
TEST(common_cfg, set_cfg_font_style)
{
    const char *fpath = DEFAULT_FONT_STYLE;

    TEST_ASSERT_EQUAL_INT(-1, set_cfg_font_style(NULL));
    TEST_ASSERT_EQUAL_INT(-1, set_cfg_font_style("font/NOT_EXIST_FONT.ttf"));

    TEST_ASSERT_EQUAL_INT(0, set_cfg_font_style(fpath));
    TEST_ASSERT_EQUAL_STRING(fpath, cfg.font_style);
}
#endif

int get_cfg_pen_image(char *ret, int ret_size)
{
    return check_resource_file_exist_and_copy(ret, ret_size, cfg.pen.image_path);
}

#if defined(UT)
TEST(common_cfg, get_cfg_pen)
{
    char buf[MAX_PATH] = { 0 };
    const char *fpath = DEFAULT_PEN_IMAGE;

    TEST_ASSERT_EQUAL_INT(-1, get_cfg_pen_image(NULL, 0));
    TEST_ASSERT_EQUAL_INT(-1, get_cfg_pen_image(buf, 1));

    TEST_ASSERT_EQUAL_INT(0, set_cfg_pen_image(fpath));
    TEST_ASSERT_EQUAL_INT(0, get_cfg_pen_image(buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING(buf, fpath);
}
#endif

int set_cfg_pen_image(const char *path)
{
    return check_resource_file_exist_and_copy(cfg.pen.image_path, sizeof(cfg.pen.image_path), path);
}

#if defined(UT)
TEST(common_cfg, set_cfg_pen)
{
    const char *fpath = DEFAULT_PEN_IMAGE;

    TEST_ASSERT_EQUAL_INT(-1, set_cfg_pen_image(NULL));
    TEST_ASSERT_EQUAL_INT(-1, set_cfg_pen_image("pen/NOT_EXIST_PEN.png"));

    TEST_ASSERT_EQUAL_INT(0, set_cfg_pen_image(fpath));
    TEST_ASSERT_EQUAL_STRING(fpath, cfg.pen.image_path);
}
#endif

int get_cfg_pen_position(void)
{
    return cfg.pen.position;
}

#if defined(UT)
TEST(common_cfg, get_cfg_pen_position)
{
    set_cfg_pen_position(1);
    TEST_ASSERT_EQUAL_INT(1, get_cfg_pen_position());
    TEST_ASSERT_EQUAL_INT(1, cfg.pen.position);

    set_cfg_pen_position(0);
    TEST_ASSERT_EQUAL_INT(0, get_cfg_pen_position());
    TEST_ASSERT_EQUAL_INT(0, cfg.pen.position);
}
#endif

int set_cfg_pen_position(int pos)
{
    return check_value_max_and_set(&cfg.pen.position, pos, MAX_PEN_POSITION);
}

#if defined(UT)
TEST(common_cfg, set_cfg_pen_position)
{
    set_cfg_pen_position(1);
    set_cfg_pen_position(MAX_PEN_POSITION + 1);
    TEST_ASSERT_EQUAL_INT(1, get_cfg_pen_position());
    TEST_ASSERT_EQUAL_INT(1, cfg.pen.position);

    set_cfg_pen_position(0);
    set_cfg_pen_position(-100);
    TEST_ASSERT_EQUAL_INT(0, get_cfg_pen_position());
    TEST_ASSERT_EQUAL_INT(0, cfg.pen.position);
}
#endif

int get_cfg_half_volume(void)
{
    return cfg.half_volume;
}

#if defined(UT)
TEST(common_cfg, get_cfg_half_volume)
{
    set_cfg_half_volume(1);
    TEST_ASSERT_EQUAL_INT(1, get_cfg_half_volume());
    TEST_ASSERT_EQUAL_INT(1, cfg.half_volume);

    set_cfg_half_volume(0);
    TEST_ASSERT_EQUAL_INT(0, get_cfg_half_volume());
    TEST_ASSERT_EQUAL_INT(0, cfg.half_volume);
}
#endif

int set_cfg_half_volume(int enable)
{
    cfg.half_volume = (enable > 0) ? 1 : 0;
    return cfg.half_volume;
}

#if defined(UT)
TEST(common_cfg, set_cfg_half_volume)
{
    set_cfg_half_volume(100);
    TEST_ASSERT_EQUAL_INT(1, get_cfg_half_volume());
    TEST_ASSERT_EQUAL_INT(1, cfg.half_volume);

    set_cfg_half_volume(-100);
    TEST_ASSERT_EQUAL_INT(0, get_cfg_half_volume());
    TEST_ASSERT_EQUAL_INT(0, cfg.half_volume);
}
#endif

int get_cfg_key_swap_l1_l2(void)
{
    return cfg.key.swap_l1l2;
}

#if defined(UT)
TEST(common_cfg, get_cfg_key_swap_l1l2)
{
    set_cfg_key_swap_l1_l2(1);
    TEST_ASSERT_EQUAL_INT(1, get_cfg_key_swap_l1_l2());
    TEST_ASSERT_EQUAL_INT(1, cfg.key_swap_l1_l2.enable);

    set_cfg_key_swap_l1_l2(0);
    TEST_ASSERT_EQUAL_INT(0, get_cfg_key_swap_l1_l2());
    TEST_ASSERT_EQUAL_INT(0, cfg.key_swap_l1_l2.enable);
}
#endif

int set_cfg_key_swap_l1_l2(int enable)
{
    cfg.key_swap_l1_l2 = (enable > 0) ? 1 : 0;
    return cfg.key_swap_l1_l2;
}

#if defined(UT)
TEST(common_cfg, set_cfg_key_swap_l1_l2)
{
    set_cfg_key_swap_l1_l2(100);
    TEST_ASSERT_EQUAL_INT(1, get_cfg_key_swap_l1_l2());
    TEST_ASSERT_EQUAL_INT(1, cfg.key_swap_l1_l2.enable);

    set_cfg_key_swap_l1_l2(-100);
    TEST_ASSERT_EQUAL_INT(0, get_cfg_key_swap_l1_l2());
    TEST_ASSERT_EQUAL_INT(0, cfg.key_swap_l1_l2.enable);
}
#endif

int get_cfg_auto_save_load(void)
{
    return cfg.auto_save_load.enable;
}

#if defined(UT)
TEST(common_cfg, get_cfg_auto_save_load)
{
    set_cfg_auto_save_load(1);
    TEST_ASSERT_EQUAL_INT(1, get_cfg_auto_save_load());
    TEST_ASSERT_EQUAL_INT(1, cfg.auto_save_load.enable);

    set_cfg_auto_save_load(0);
    TEST_ASSERT_EQUAL_INT(0, get_cfg_auto_save_load());
    TEST_ASSERT_EQUAL_INT(0, cfg.auto_save_load.enable);
}
#endif

int set_cfg_auto_save_load(int enable)
{
    cfg.auto_save_load.enable = (enable > 0) ? 1 : 0;
    return cfg.auto_save_load.enable;
}

#if defined(UT)
TEST(common_cfg, set_cfg_auto_save_load)
{
    set_cfg_auto_save_load(100);
    TEST_ASSERT_EQUAL_INT(1, get_cfg_auto_save_load());
    TEST_ASSERT_EQUAL_INT(1, cfg.auto_save_load.enable);

    set_cfg_auto_save_load(-100);
    TEST_ASSERT_EQUAL_INT(0, get_cfg_auto_save_load());
    TEST_ASSERT_EQUAL_INT(0, cfg.auto_save_load.enable);
}
#endif

int get_cfg_auto_save_load_slot(void)
{
    return cfg.auto_save_load.slot;
}

#if defined(UT)
TEST(common_cfg, get_cfg_auto_save_load_slot)
{
    set_cfg_auto_save_load_slot(1);
    TEST_ASSERT_EQUAL_INT(1, get_cfg_auto_save_load_slot());
    TEST_ASSERT_EQUAL_INT(1, cfg.auto_save_load.slot);

    set_cfg_auto_save_load_slot(0);
    TEST_ASSERT_EQUAL_INT(0, get_cfg_auto_save_load_slot());
    TEST_ASSERT_EQUAL_INT(0, cfg.auto_save_load.slot);
}
#endif

int set_cfg_auto_save_load_slot(int slot)
{
    return check_value_max_and_set(&cfg.auto_save_load.slot, slot, MAX_AUTO_SAVE_LOAD_SLOT);
}

#if defined(UT)
TEST(common_cfg, set_cfg_auto_save_load_slot)
{
    set_cfg_auto_save_load_slot(1);
    set_cfg_auto_save_load_slot(MAX_AUTO_SAVE_LOAD_SLOT + 1);
    TEST_ASSERT_EQUAL_INT(1, get_cfg_auto_save_load_slot());
    TEST_ASSERT_EQUAL_INT(1, cfg.auto_save_load.slot);

    set_cfg_auto_save_load_slot(0);
    set_cfg_auto_save_load_slot(-100);
    TEST_ASSERT_EQUAL_INT(0, get_cfg_auto_save_load_slot());
    TEST_ASSERT_EQUAL_INT(0, cfg.auto_save_load.slot);
}
#endif

int get_cfg_display_layout_alt(void)
{
    return cfg.display.alt;
}

#if defined(UT)
TEST(common_cfg, get_cfg_display_layout_alt)
{
    set_cfg_display_layout_alt(1);
    TEST_ASSERT_EQUAL_INT(1, get_cfg_display_layout_alt());
    TEST_ASSERT_EQUAL_INT(1, cfg.display.alt);

    set_cfg_display_layout_alt(0);
    TEST_ASSERT_EQUAL_INT(0, get_cfg_display_layout_alt());
    TEST_ASSERT_EQUAL_INT(0, cfg.display.alt);
}
#endif

int set_cfg_display_layout_alt(int idx)
{
    return check_value_max_and_set(&cfg.display.alt, idx, MAX_DISPLAY_LAYOUT_ALT);
}

#if defined(UT)
TEST(common_cfg, set_cfg_display_layout_alt)
{
    set_cfg_display_layout_alt(1);
    set_cfg_display_layout_alt(MAX_DISPLAY_LAYOUT_ALT + 1);
    TEST_ASSERT_EQUAL_INT(1, get_cfg_display_layout_alt());
    TEST_ASSERT_EQUAL_INT(1, cfg.display.alt);

    set_cfg_display_layout_alt(0);
    set_cfg_display_layout_alt(-100);
    TEST_ASSERT_EQUAL_INT(0, get_cfg_display_layout_alt());
    TEST_ASSERT_EQUAL_INT(0, cfg.display.alt);
}
#endif

int get_cfg_display_layout(void)
{
    return cfg.display.layout;
}

#if defined(UT)
TEST(common_cfg, get_cfg_display_layout)
{
    set_cfg_display_layout(1);
    TEST_ASSERT_EQUAL_INT(1, get_cfg_display_layout());
    TEST_ASSERT_EQUAL_INT(1, cfg.display.layout);

    set_cfg_display_layout(0);
    TEST_ASSERT_EQUAL_INT(0, get_cfg_display_layout());
    TEST_ASSERT_EQUAL_INT(0, cfg.display.layout);
}
#endif

int set_cfg_display_layout(int idx)
{
    return check_value_max_and_set(&cfg.display.layout, idx, MAX_DISPLAY_LAYOUT);
}

#if defined(UT)
TEST(common_cfg, set_cfg_display_layout)
{
    set_cfg_display_layout(1);
    set_cfg_display_layout(MAX_DISPLAY_LAYOUT + 1);
    TEST_ASSERT_EQUAL_INT(1, get_cfg_display_layout());
    TEST_ASSERT_EQUAL_INT(1, cfg.display.layout);

    set_cfg_display_layout(0);
    set_cfg_display_layout(-100);
    TEST_ASSERT_EQUAL_INT(0, get_cfg_display_layout());
    TEST_ASSERT_EQUAL_INT(0, cfg.display.layout);
}
#endif

int get_cfg_display_layout_alpha(void)
{
    return cfg.display.alpha;
}

#if defined(UT)
TEST(common_cfg, get_cfg_display_layout_alpha)
{
    set_cfg_display_layout_alpha(1);
    TEST_ASSERT_EQUAL_INT(1, get_cfg_display_layout_alpha());
    TEST_ASSERT_EQUAL_INT(1, cfg.display.alpha);

    set_cfg_display_layout_alpha(0);
    TEST_ASSERT_EQUAL_INT(0, get_cfg_display_layout_alpha());
    TEST_ASSERT_EQUAL_INT(0, cfg.display.alpha);
}
#endif

int set_cfg_display_layout_alpha(int idx)
{
    return check_value_max_and_set(&cfg.display.alpha, idx, MAX_DISPLAY_LAYOUT_ALPHA);
}

#if defined(UT)
TEST(common_cfg, set_cfg_display_layout_alpha)
{
    set_cfg_display_layout_alpha(1);
    set_cfg_display_layout_alpha(MAX_DISPLAY_LAYOUT_ALPHA + 1);
    TEST_ASSERT_EQUAL_INT(1, get_cfg_display_layout_alpha());
    TEST_ASSERT_EQUAL_INT(1, cfg.display.alpha);

    set_cfg_display_layout_alpha(0);
    set_cfg_display_layout_alpha(-100);
    TEST_ASSERT_EQUAL_INT(0, get_cfg_display_layout_alpha());
    TEST_ASSERT_EQUAL_INT(0, cfg.display.alpha);
}
#endif

int get_cfg_display_layout_border(void)
{
    return cfg.display.border;
}

#if defined(UT)
TEST(common_cfg, get_cfg_display_layout_border)
{
    set_cfg_display_layout_border(1);
    TEST_ASSERT_EQUAL_INT(1, get_cfg_display_layout_border());
    TEST_ASSERT_EQUAL_INT(1, cfg.display.border);

    set_cfg_display_layout_border(0);
    TEST_ASSERT_EQUAL_INT(0, get_cfg_display_layout_border());
    TEST_ASSERT_EQUAL_INT(0, cfg.display.border);
}
#endif

int set_cfg_display_layout_border(int idx)
{
    return check_value_max_and_set(&cfg.display.border, idx, MAX_DISPLAY_LAYOUT_BORDER);
}

#if defined(UT)
TEST(common_cfg, set_cfg_display_layout_border)
{
    set_cfg_display_layout_border(1);
    set_cfg_display_layout_border(MAX_DISPLAY_LAYOUT_BORDER + 1);
    TEST_ASSERT_EQUAL_INT(1, get_cfg_display_layout_border());
    TEST_ASSERT_EQUAL_INT(1, cfg.display.border);

    set_cfg_display_layout_border(0);
    set_cfg_display_layout_border(-100);
    TEST_ASSERT_EQUAL_INT(0, get_cfg_display_layout_border());
    TEST_ASSERT_EQUAL_INT(0, cfg.display.border);
}
#endif

int get_cfg_display_layout_position(void)
{
    return cfg.display.position;
}

#if defined(UT)
TEST(common_cfg, get_cfg_display_layout_position)
{
    set_cfg_display_layout_position(1);
    TEST_ASSERT_EQUAL_INT(1, get_cfg_display_layout_position());
    TEST_ASSERT_EQUAL_INT(1, cfg.display.position);

    set_cfg_display_layout_position(0);
    TEST_ASSERT_EQUAL_INT(0, get_cfg_display_layout_position());
    TEST_ASSERT_EQUAL_INT(0, cfg.display.position);
}
#endif

int set_cfg_display_layout_position(int idx)
{
    return check_value_max_and_set(&cfg.display.position, idx, MAX_DISPLAY_LAYOUT_POSITION);
}

#if defined(UT)
TEST(common_cfg, set_cfg_display_layout_position)
{
    set_cfg_display_layout_position(1);
    set_cfg_display_layout_position(MAX_DISPLAY_LAYOUT_POSITION + 1);
    TEST_ASSERT_EQUAL_INT(1, get_cfg_display_layout_position());
    TEST_ASSERT_EQUAL_INT(1, cfg.display.position);

    set_cfg_display_layout_position(0);
    set_cfg_display_layout_position(-100);
    TEST_ASSERT_EQUAL_INT(0, get_cfg_display_layout_position());
    TEST_ASSERT_EQUAL_INT(0, cfg.display.position);
}
#endif

int get_cfg_border_image(char *ret, int ret_size)
{
    return check_resource_file_exist_and_copy(ret, ret_size, cfg.border_image);
}

#if defined(UT)
TEST(common_cfg, get_cfg_border_image)
{
    char buf[MAX_PATH] = { 0 };
    const char *fpath = DEFAULT_BORDER_IMAGE;

    TEST_ASSERT_EQUAL_INT(-1, get_cfg_border_image(NULL, 0));
    TEST_ASSERT_EQUAL_INT(-1, get_cfg_border_image(buf, 1));

    TEST_ASSERT_EQUAL_INT(0, set_cfg_border_image(fpath));
    TEST_ASSERT_EQUAL_INT(0, get_cfg_border_image(buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING(buf, fpath);
}
#endif

int set_cfg_border_image(const char *path)
{
    return check_resource_file_exist_and_copy(cfg.border_image, sizeof(cfg.border_image), path);
}

#if defined(UT)
TEST(common_cfg, set_cfg_border_image)
{
    const char *fpath = DEFAULT_BORDER_IMAGE;

    TEST_ASSERT_EQUAL_INT(-1, set_cfg_border_image(NULL));
    TEST_ASSERT_EQUAL_INT(-1, set_cfg_border_image("border/NOT_EXIST_BORDER.png"));

    TEST_ASSERT_EQUAL_INT(0, set_cfg_border_image(fpath));
    TEST_ASSERT_EQUAL_STRING(fpath, cfg.border_image);
}
#endif

int get_cfg_pen_move_speed_x(void)
{
    return cfg.pen.move_speed.x;
}

#if defined(UT)
TEST(common_cfg, get_cfg_pen_move_speed_x)
{
    set_cfg_pen_move_speed_x(1);
    TEST_ASSERT_EQUAL_INT(1, get_cfg_pen_move_speed_x());
    TEST_ASSERT_EQUAL_INT(1, cfg.pen.move_speed.x);

    set_cfg_pen_move_speed_x(0);
    TEST_ASSERT_EQUAL_INT(0, get_cfg_pen_move_speed_x());
    TEST_ASSERT_EQUAL_INT(0, cfg.pen.move_speed.x);
}
#endif

int set_cfg_pen_move_speed_x(int speed)
{
    return check_value_max_and_set(&cfg.pen.move_speed.x, speed, MAX_PEN_MOVE_SPEED);
}

#if defined(UT)
TEST(common_cfg, set_cfg_pen_move_speed_x)
{
    set_cfg_pen_move_speed_x(1);
    set_sys_volume(MAX_PEN_MOVE_SPEED + 1);
    TEST_ASSERT_EQUAL_INT(1, get_cfg_pen_move_speed_x());
    TEST_ASSERT_EQUAL_INT(1, cfg.pen.move_speed.x);

    set_cfg_pen_move_speed_x(0);
    set_cfg_pen_move_speed_x(-100);
    TEST_ASSERT_EQUAL_INT(0, get_cfg_pen_move_speed_x());
    TEST_ASSERT_EQUAL_INT(0, cfg.pen.move_speed.x);
}
#endif

int get_cfg_pen_move_speed_y(void)
{
    return cfg.pen.move_speed.y;
}

#if defined(UT)
TEST(common_cfg, get_cfg_pen_move_speed_y)
{
    set_cfg_pen_move_speed_y(1);
    TEST_ASSERT_EQUAL_INT(1, get_cfg_pen_move_speed_y());
    TEST_ASSERT_EQUAL_INT(1, cfg.pen.move_speed.y);

    set_cfg_pen_move_speed_y(0);
    TEST_ASSERT_EQUAL_INT(0, get_cfg_pen_move_speed_y());
    TEST_ASSERT_EQUAL_INT(0, cfg.pen.move_speed.y);
}
#endif

int set_cfg_pen_move_speed_y(int speed)
{
    return check_value_max_and_set(&cfg.pen.move_speed.y, speed, MAX_PEN_MOVE_SPEED);
}

#if defined(UT)
TEST(common_cfg, set_cfg_pen_move_speed_y)
{
    set_cfg_pen_move_speed_y(1);
    set_sys_volume(MAX_PEN_MOVE_SPEED + 1);
    TEST_ASSERT_EQUAL_INT(1, get_cfg_pen_move_speed_y());
    TEST_ASSERT_EQUAL_INT(1, cfg.pen.move_speed.y);

    set_cfg_pen_move_speed_y(0);
    set_cfg_pen_move_speed_y(-100);
    TEST_ASSERT_EQUAL_INT(0, get_cfg_pen_move_speed_y());
    TEST_ASSERT_EQUAL_INT(0, cfg.pen.move_speed.y);
}
#endif

int get_cfg_cpu_min_core(void)
{
    return cfg.cpu.min_core;
}

#if defined(UT)
TEST(common_cfg, get_cfg_cpu_min_core)
{
    set_cfg_cpu_min_core(MIN_CPU_CORE);
    set_cfg_cpu_min_core(0);
    TEST_ASSERT_EQUAL_INT(MIN_CPU_CORE, get_cfg_cpu_min_core());
    TEST_ASSERT_EQUAL_INT(MIN_CPU_CORE, cfg.cpu.min_core);
}
#endif

int set_cfg_cpu_min_core(int core)
{
    return check_value_min_and_set(&cfg.cpu.min_core, core, MIN_CPU_CORE);
}

#if defined(UT)
TEST(common_cfg, set_cfg_cpu_min_core)
{
    set_cfg_cpu_min_core(MIN_CPU_CORE);
    set_cfg_cpu_min_core(MIN_CPU_CORE - 1);
    TEST_ASSERT_EQUAL_INT(MIN_CPU_CORE, get_cfg_cpu_min_core());
    TEST_ASSERT_EQUAL_INT(MIN_CPU_CORE, cfg.cpu.min_core);
}
#endif

int get_cfg_cpu_min_freq(void)
{
    return cfg.cpu.min_freq;
}

#if defined(UT)
TEST(common_cfg, get_cfg_cpu_min_freq)
{
    set_cfg_cpu_min_freq(MIN_CPU_FREQ);
    TEST_ASSERT_EQUAL_INT(MIN_CPU_FREQ, get_cfg_cpu_min_freq());
    TEST_ASSERT_EQUAL_INT(MIN_CPU_FREQ, cfg.cpu.min_freq);
}
#endif

int set_cfg_cpu_min_freq(int freq)
{
    return check_value_min_and_set(&cfg.cpu.min_freq, freq, MIN_CPU_FREQ);
}

#if defined(UT)
TEST(common_cfg, set_cfg_cpu_min_freq)
{
    set_cfg_cpu_min_freq(MIN_CPU_FREQ);
    set_cfg_cpu_min_freq(MIN_CPU_FREQ - 1);
    TEST_ASSERT_EQUAL_INT(MIN_CPU_FREQ, get_cfg_cpu_min_freq());
    TEST_ASSERT_EQUAL_INT(MIN_CPU_FREQ, cfg.cpu.min_freq);
}
#endif

int get_cfg_cpu_max_core(void)
{
    return cfg.cpu.max_core;
}

#if defined(UT)
TEST(common_cfg, get_cfg_cpu_max_core)
{
    set_cfg_cpu_max_core(1);
    TEST_ASSERT_EQUAL_INT(1, get_cfg_cpu_max_core());
    TEST_ASSERT_EQUAL_INT(1, cfg.cpu.max_core);

    set_cfg_cpu_max_core(0);
    TEST_ASSERT_EQUAL_INT(0, get_cfg_cpu_max_core());
    TEST_ASSERT_EQUAL_INT(0, cfg.cpu.max_core);
}
#endif

int set_cfg_cpu_max_core(int core)
{
    return check_value_max_and_set(&cfg.cpu.max_core, core, MAX_CPU_CORE);
}

#if defined(UT)
TEST(common_cfg, set_cfg_cpu_max_core)
{
    set_cfg_cpu_max_core(1);
    set_cfg_cpu_max_core(MAX_CPU_CORE + 1);
    TEST_ASSERT_EQUAL_INT(1, get_cfg_cpu_max_core());
    TEST_ASSERT_EQUAL_INT(1, cfg.cpu.max_core);

    set_cfg_cpu_max_core(0);
    set_cfg_cpu_max_core(-100);
    TEST_ASSERT_EQUAL_INT(0, get_cfg_cpu_max_core());
    TEST_ASSERT_EQUAL_INT(0, cfg.cpu.max_core);
}
#endif

int get_cfg_cpu_max_freq(void)
{
    return cfg.cpu.max_freq;
}

#if defined(UT)
TEST(common_cfg, get_cfg_cpu_max_freq)
{
    set_cfg_cpu_max_freq(1000);
    TEST_ASSERT_EQUAL_INT(1000, get_cfg_cpu_max_freq());
    TEST_ASSERT_EQUAL_INT(1000, cfg.cpu.max_freq);

    set_cfg_cpu_max_freq(0);
    TEST_ASSERT_EQUAL_INT(0, get_cfg_cpu_max_freq());
    TEST_ASSERT_EQUAL_INT(0, cfg.cpu.max_freq);
}
#endif

int set_cfg_cpu_max_freq(int freq)
{
    return check_value_max_and_set(&cfg.cpu.max_freq, freq, MAX_CPU_FREQ);
}

#if defined(UT)
TEST(common_cfg, set_cfg_cpu_max_freq)
{
    set_cfg_cpu_max_freq(1000);
    set_cfg_cpu_max_freq(MAX_CPU_FREQ + 1);
    TEST_ASSERT_EQUAL_INT(1000, get_cfg_cpu_max_freq());
    TEST_ASSERT_EQUAL_INT(1000, cfg.cpu.max_freq);

    set_cfg_cpu_max_freq(0);
    set_cfg_cpu_max_freq(-100);
    TEST_ASSERT_EQUAL_INT(0, get_cfg_cpu_max_freq());
    TEST_ASSERT_EQUAL_INT(0, cfg.cpu.max_freq);
}
#endif

static int read_json_obj_int(struct json_object *jfile, const char *item, int *ret)
{
    struct json_object *jval = NULL;

    if (!jfile || !item || !ret) {
        err(COM"invalid input parameters in %s(jfile:0x%x, item:0x%x, ret:0x%x)\n", __func__, jfile, item, ret);
        return -1;
    }

    if (json_object_object_get_ex(jfile, item, &jval)) {
        *ret = json_object_get_int(jval);
        info(COM"read json object(%s:%d)\n", item, *ret);
    }
    else {
        err(COM"failed to read json object(%s)\n", item);
        return -1;
    }
    return 0;
}

static int read_json_obj_str(struct json_object *jfile, const char *item, char *ret, int ret_size)
{
    struct json_object *jval = NULL;

    if (!jfile || !item || !ret) {
        err(COM"invalid input parameters in %s(jfile:0x%x, item:0x%x, ret:0x%x)\n", __func__, jfile, item, ret);
        return -1;
    }

    if (json_object_object_get_ex(jfile, item, &jval)) {
        strncpy(ret, json_object_get_string(jval), ret_size);
        info(COM"read json object(%s:\"%s\")\n", item, ret);
    }
    else {
        err(COM"failed to read json object(%s)\n", item);
        return -1;
    }
    return 0;
}

int read_cfg_json_file(void)
{
    int ret = -1;
    struct json_object *jfile = NULL;

    jfile = json_object_from_file(cfg.json_path);
    if (jfile ==  NULL) {
        err(COM"failed to open config json file(path:\"%s\")\n", cfg.json_path);
        return ret;
    }

    do {
        if (read_json_obj_int(jfile, JSON_CFG_HALF_VOLUME, &cfg.half_volume) < 0) {
            break;
        }

        if (read_json_obj_int(jfile, JSON_CFG_AUTO_SAVE_LOAD, &cfg.auto_save_load.enable) < 0) {
            break;
        }

        if (read_json_obj_int(jfile, JSON_CFG_AUTO_SAVE_LOAD_SLOT, &cfg.auto_save_load.slot) < 0) {
            break;
        }

        if (read_json_obj_str(jfile, JSON_CFG_PEN_IMAGE, cfg.pen.image_path, sizeof(cfg.pen.image_path)) < 0) {
            break;
        }

        if (read_json_obj_str(jfile, JSON_CFG_SDL_VERSION, cfg.sdl_version, sizeof(cfg.sdl_version)) < 0) {
            break;
        }

        if (read_json_obj_str(jfile, JSON_CFG_FONT_STYLE, cfg.font_style, sizeof(cfg.font_style)) < 0) {
            break;
        }

        if (read_json_obj_str(jfile, JSON_CFG_BORDER_IMAGE, cfg.border_image, sizeof(cfg.border_image)) < 0) {
            break;
        }

        if (read_json_obj_int(jfile, JSON_CFG_DISPLAY_LAYOUT, &cfg.display.layout) < 0) {
            break;
        }

        if (read_json_obj_int(jfile, JSON_CFG_DISPLAY_LAYOUT_ALPHA, &cfg.display.alpha) < 0) {
            break;
        }

        if (read_json_obj_int(jfile, JSON_CFG_CPU_MAX_CORE, &cfg.cpu.max_core) < 0) {
            break;
        }

        if (read_json_obj_int(jfile, JSON_CFG_CPU_MAX_FREQ, &cfg.cpu.max_freq) < 0) {
            break;
        }

        if (read_json_obj_int(jfile, JSON_CFG_CPU_MIN_CORE, &cfg.cpu.min_core) < 0) {
            break;
        }

        if (read_json_obj_int(jfile, JSON_CFG_CPU_MIN_FREQ, &cfg.cpu.min_freq) < 0) {
            break;
        }

        ret = 0;
    } while (0);

    json_object_put(jfile);
    return ret;
}

#if defined(UT)
TEST(common_cfg, read_cfg_json_file)
{
    TEST_ASSERT_EQUAL_INT(0, read_cfg_json_file());

    memset(&cfg, 0, sizeof(cfg));
    TEST_ASSERT_EQUAL_INT(-1, read_cfg_json_file());

    strncpy(cfg.json_path, "/NOT_EXIST_FILE", sizeof(cfg.json_path));
    TEST_ASSERT_EQUAL_INT(-1, read_cfg_json_file());
}
#endif

int read_sys_json_file(void)
{
    int ret = -1;
    struct json_object *jfile = NULL;

    jfile = json_object_from_file(sys.json_path);
    if (jfile ==  NULL) {
        err(COM"failed to open system json file(path:\"%s\")\n", sys.json_path);
        return ret;
    }

    do {
        if (read_json_obj_int(jfile, JSON_SYS_VOLUME, &sys.volume)) {
            break;
        }

        ret = 0;
    } while (0);

    json_object_put(jfile);
    return ret;
}

#if defined(UT)
TEST(common_cfg, read_sys_json_file)
{
    memset(&sys, 0, sizeof(sys));
    TEST_ASSERT_EQUAL_INT(-1, read_sys_json_file());

    strncpy(sys.json_path, "/NOT_EXIST_FILE", sizeof(sys.json_path));
    TEST_ASSERT_EQUAL_INT(-1, read_sys_json_file());

    strncpy(sys.json_path, JSON_SYS_PATH, sizeof(sys.json_path));
    TEST_ASSERT_EQUAL_INT(0, read_sys_json_file());
}
#endif

int init_cfg(void)
{
    getcwd(home_path, sizeof(home_path));

#if defined(UT)
    const char *path = "/../drastic";
    strncat(home_path, path, strlen(path));
#endif

    info(COM"home folder:\"%s\"\n", home_path);

    snprintf(cfg.json_path, sizeof(cfg.json_path), "%s/%s", home_path, JSON_CFG_PATH);
    info(COM"config json file:\"%s\"\n", cfg.json_path);

    strncpy(sys.json_path, JSON_SYS_PATH, sizeof(sys.json_path));
    info(COM"system json file:\"%s\"\n", sys.json_path);

    if (read_cfg_json_file() < 0) {
        return -1;
    }

    if (read_sys_json_file() < 0) {
        return -1;
    }

    return 0;
}

#if defined(UT)
TEST(common_cfg, init_cfg)
{
    TEST_ASSERT_EQUAL_INT(0, init_cfg());
}
#endif

#if defined(UT)
TEST_GROUP_RUNNER(common_cfg)
{
RUN_TEST_CASE(common_cfg, check_pointer_not_null_and_copy);
RUN_TEST_CASE(common_cfg, check_value_max_and_set);
RUN_TEST_CASE(common_cfg, set_home_path);
RUN_TEST_CASE(common_cfg, get_sys_volume);
RUN_TEST_CASE(common_cfg, set_sys_volume);
RUN_TEST_CASE(common_cfg, get_cfg_sdl_version);
RUN_TEST_CASE(common_cfg, get_cfg_font_style);
RUN_TEST_CASE(common_cfg, set_cfg_font_style);
RUN_TEST_CASE(common_cfg, get_cfg_pen);
RUN_TEST_CASE(common_cfg, set_cfg_pen);
RUN_TEST_CASE(common_cfg, get_cfg_pen_position);
RUN_TEST_CASE(common_cfg, set_cfg_pen_position);
RUN_TEST_CASE(common_cfg, get_cfg_half_volume);
RUN_TEST_CASE(common_cfg, set_cfg_half_volume);
RUN_TEST_CASE(common_cfg, get_cfg_auto_save_load);
RUN_TEST_CASE(common_cfg, set_cfg_auto_save_load);
RUN_TEST_CASE(common_cfg, get_cfg_auto_save_load_slot);
RUN_TEST_CASE(common_cfg, set_cfg_auto_save_load_slot);
RUN_TEST_CASE(common_cfg, get_cfg_display_layout_alt);
RUN_TEST_CASE(common_cfg, set_cfg_display_layout_alt);
RUN_TEST_CASE(common_cfg, get_cfg_display_layout);
RUN_TEST_CASE(common_cfg, set_cfg_display_layout);
RUN_TEST_CASE(common_cfg, get_cfg_display_layout_alpha);
RUN_TEST_CASE(common_cfg, set_cfg_display_layout_alpha);
RUN_TEST_CASE(common_cfg, get_cfg_display_layout_border);
RUN_TEST_CASE(common_cfg, set_cfg_display_layout_border);
RUN_TEST_CASE(common_cfg, get_cfg_display_layout_position);
RUN_TEST_CASE(common_cfg, set_cfg_display_layout_position);
RUN_TEST_CASE(common_cfg, get_cfg_border_image);
RUN_TEST_CASE(common_cfg, set_cfg_border_image);
RUN_TEST_CASE(common_cfg, get_cfg_pen_move_speed_x);
RUN_TEST_CASE(common_cfg, set_cfg_pen_move_speed_x);
RUN_TEST_CASE(common_cfg, get_cfg_pen_move_speed_y);
RUN_TEST_CASE(common_cfg, set_cfg_pen_move_speed_y);
RUN_TEST_CASE(common_cfg, get_cfg_cpu_min_core);
RUN_TEST_CASE(common_cfg, set_cfg_cpu_min_core);
RUN_TEST_CASE(common_cfg, get_cfg_cpu_min_freq);
RUN_TEST_CASE(common_cfg, set_cfg_cpu_min_freq);
RUN_TEST_CASE(common_cfg, get_cfg_cpu_max_core);
RUN_TEST_CASE(common_cfg, set_cfg_cpu_max_core);
RUN_TEST_CASE(common_cfg, get_cfg_cpu_max_freq);
RUN_TEST_CASE(common_cfg, set_cfg_cpu_max_freq);
RUN_TEST_CASE(common_cfg, read_cfg_json_file);
RUN_TEST_CASE(common_cfg, read_sys_json_file);
RUN_TEST_CASE(common_cfg, init_cfg);
}
#endif

