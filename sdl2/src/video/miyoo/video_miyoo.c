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
#include <dirent.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <time.h>
#include <json-c/json.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

#if defined(UT)
#include "unity_fixture.h"
#endif

#include "../../SDL_internal.h"
#include "../SDL_sysvideo.h"
#include "../SDL_sysvideo.h"
#include "../SDL_pixels_c.h"
#include "../../events/SDL_events_c.h"

#include "SDL_image.h"
#include "SDL_version.h"
#include "SDL_syswm.h"
#include "SDL_loadso.h"
#include "SDL_events.h"
#include "SDL_video.h"
#include "SDL_mouse.h"
#include "video_miyoo.h"
#include "event_miyoo.h"

#include "log.h"
#include "pen.h"
#include "drastic.h"

NDS nds = {0};
GFX gfx = {0};
MiyooVideoInfo vid = {0};

extern miyoo_event_t evt;

int FB_W = 0;
int FB_H = 0;
int FB_SIZE = 0;
int LINE_H = 0;
int TMP_SIZE = 0;
int FONT_SIZE = 0;
int show_fps = 0;
int pixel_filter = 1;
int savestate_busy = 0;
SDL_Surface *fps_info = NULL;

static volatile int is_video_thread_running = 0;

static pthread_t thread;
static int need_reload_bg = RELOAD_BG_COUNT;
static SDL_Surface *cvt = NULL;

static int MiyooVideoInit(_THIS);
static int MiyooSetDisplayMode(_THIS, SDL_VideoDisplay *display, SDL_DisplayMode *mode);
static void MiyooVideoQuit(_THIS);

static CUST_MENU drastic_menu = {0};
static char *translate[MAX_LANG_LINE] = {0};

#if defined(A30)
GLfloat bgVertices[] = {
   -1.0f,  1.0f,  0.0f,  0.0f,  0.0f,
   -1.0f, -1.0f,  0.0f,  0.0f,  1.0f,
    1.0f, -1.0f,  0.0f,  1.0f,  1.0f,
    1.0f,  1.0f,  0.0f,  1.0f,  0.0f
};

GLfloat vVertices[] = {
   -1.0f,  1.0f,  0.0f,  0.0f,  0.0f,
   -1.0f, -1.0f,  0.0f,  0.0f,  1.0f,
    1.0f, -1.0f,  0.0f,  1.0f,  1.0f,
    1.0f,  1.0f,  0.0f,  1.0f,  0.0f
};
GLushort indices[] = {0, 1, 2, 0, 2, 3};

const char *vShaderSrc =
    "attribute vec4 a_position;   \n"
    "attribute vec2 a_texCoord;   \n"
    "attribute float a_alpha;     \n"
    "varying vec2 v_texCoord;     \n"
    "void main()                  \n"
    "{                            \n"
    "    const float angle = 90.0 * (3.1415 * 2.0) / 360.0;                                                                            \n"
    "    mat4 rot = mat4(cos(angle), -sin(angle), 0.0, 0.0, sin(angle), cos(angle), 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0); \n"
    "    gl_Position = a_position * rot; \n"
    "    v_texCoord = a_texCoord;        \n"
    "}                                   \n";
    
const char *fShaderSrc =
    "precision mediump float;                                  \n"
    "varying vec2 v_texCoord;                                  \n"
    "uniform float s_alpha;                                    \n"
    "uniform sampler2D s_texture;                              \n"
    "void main()                                               \n"
    "{                                                         \n"
    "    if (s_alpha >= 2.0) {                                 \n"
    "        gl_FragColor = texture2D(s_texture, v_texCoord);  \n"
    "    }                                                     \n"
    "    else if (s_alpha > 0.0) {                             \n"
    "        vec3 tex = texture2D(s_texture, v_texCoord).bgr;  \n"
    "        gl_FragColor = vec4(tex, s_alpha);                \n"
    "    }                                                     \n"
    "    else {                                                \n"
    "        vec3 tex = texture2D(s_texture, v_texCoord).bgr;  \n"
    "        gl_FragColor = vec4(tex, 1.0);                    \n"
    "    }                                                     \n"
    "}                                                         \n";

static struct _cpu_clock cpu_clock[] = {
    {96, 0x80000110},
    {144, 0x80000120},
    {192, 0x80000130},
    {216, 0x80000220},
    {240, 0x80000410},
    {288, 0x80000230},
    {336, 0x80000610},
    {360, 0x80000420},
    {384, 0x80000330},
    {432, 0x80000520},
    {480, 0x80000430},
    {504, 0x80000620},
    {528, 0x80000a10},
    {576, 0x80000530},
    {624, 0x80000c10},
    {648, 0x80000820},
    {672, 0x80000630},
    {720, 0x80000920},
    {768, 0x80000730},
    {792, 0x80000a20},
    {816, 0x80001010},
    {864, 0x80000830},
    {864, 0x80001110},
    {912, 0x80001210},
    {936, 0x80000c20},
    {960, 0x80000930},
    {1008, 0x80000d20},
    {1056, 0x80000a30},
    {1080, 0x80000e20},
    {1104, 0x80001610},
    {1152, 0x80000b30},
    {1200, 0x80001810},
    {1224, 0x80001020},
    {1248, 0x80000c30},
    {1296, 0x80001120},
    {1344, 0x80000d30},
    {1368, 0x80001220},
    {1392, 0x80001c10},
    {1440, 0x80000e30},
    {1488, 0x80001e10},
    {1512, 0x80001420},
    {1536, 0x80000f30},
    {1584, 0x80001520},
    {1632, 0x80001030},
    {1656, 0x80001620},
    {1728, 0x80001130},
    {1800, 0x80001820},
    {1824, 0x80001230},
    {1872, 0x80001920},
    {1920, 0x80001330},
    {1944, 0x80001a20},
    {2016, 0x80001430},
    {2088, 0x80001c20},
    {2112, 0x80001530},
    {2160, 0x80001d20},
    {2208, 0x80001630},
    {2232, 0x80001e20},
    {2304, 0x80001730},
    {2400, 0x80001830},
    {2496, 0x80001930},
    {2592, 0x80001a30},
    {2688, 0x80001b30},
    {2784, 0x80001c30},
    {2880, 0x80001d30},
    {2976, 0x80001e30},
    {3072, 0x80001f30},
};

static int max_cpu_item = sizeof(cpu_clock) / sizeof(struct _cpu_clock);
#endif

#if defined(UT)
TEST_GROUP(sdl2_video_miyoo);

TEST_SETUP(sdl2_video_miyoo)
{
}

TEST_TEAR_DOWN(sdl2_video_miyoo)
{
}
#endif

static void* sdl_malloc(size_t size)
{
    static int idx = 0;

    void *r = NULL;
    uint32_t bpp = 0;//*((uint32_t *)VAR_SDL_SCREEN_BPP);

    if ((size == (NDS_W * NDS_H * bpp)) ||
        (size == (NDS_Wx2 * NDS_Hx2 * bpp)))
    {
        r = gfx.lcd.virAddr[0][idx];
        idx += 1;
        idx %= 2;
    }
    else {
        r = malloc(size);
    }
    return r;
}

static void sdl_free(void *ptr)
{
    int c0 = 0;
    int c1 = 0;
    int found = 0;

    for (c0 = 0; c0 < 2; c0++) {
        for (c1 = 0; c1 < 2; c1++) {
            if (ptr == gfx.lcd.virAddr[c0][c1]) {
                found = 1;
                break;
            }
        }
    }

    if (found == 0) {
        free(ptr);
    }
}

static void* sdl_realloc(void *ptr, size_t size)
{
    void *r = NULL;
    uint32_t bpp = 0;//*((uint32_t *)VAR_SDL_SCREEN_BPP);

    if ((size == (NDS_W * NDS_H * bpp)) ||
        (size == (NDS_Wx2 * NDS_Hx2 * bpp)))
    {
        r = sdl_malloc(size);
    }
    else {
        r = realloc(ptr, size);
    }
    return r;
}

#ifdef UT
int get_bat_val(void)
{
    return 0;
}
#endif

#ifdef MINI
static int get_bat_val(void)
{
    int r = 0;
    char buf[255] = {0};
    uint32_t v[2] = {0};
    struct stat st = {0};
    const char *AXP = "/customer/app/axp_test";

    if (stat (AXP, &st) == 0) {
        // {"battery":99, "voltage":4254, "charging":3}
        FILE *fd = popen(AXP, "r");

        if (fd) {
            fgets(buf, sizeof(buf), fd);
            pclose(fd);
            return (atoi(&buf[11]));
        }
        return 0;
    }

    if (vid.sar_fd < 0) {
        return 0;
    }

    ioctl(vid.sar_fd, 0x6100, 0);
    ioctl(vid.sar_fd, 0x6101, v);
    r = 100 - (((BAT_MAX_VAL - v[1]) * 100) / (BAT_MAX_VAL - BAT_MIN_VAL));
    if (r > 100) {
        r = 100;
    }
    if (r < 0) {
        r = 0;
    }
    return r;
}
#endif

#ifdef A30
static int get_bat_val(void)
{
    static int maxv = 0;

    FILE *fd = NULL;
    char buf[32] = {0};
    int r = 0;
    int curv = 0;

    if (maxv == 0) {
        fd = popen(BAT_MAX_CMD, "r");
        if (fd) {
            fgets(buf, sizeof(buf), fd);
            pclose(fd);
            maxv = atoi(buf);
        }
    }

    fd = popen(BAT_CUR_CMD, "r");
    if (fd) {
        fgets(buf, sizeof(buf), fd);
        pclose(fd);
        curv = atoi(buf);
    }

    r = 100 - (((maxv - curv) * 100) / 1200000);
    if (r > 100) {
        r = 100;
    }
    if (r < 0) {
        r = 0;
    }
    return r;
}
#endif

static void write_file(const char *fname, const void *buf, int len)
{
    struct stat st = {0};
    int fd = -1;

    if (stat(buf, &st) == -1) {
        fd = open(fname, O_WRONLY | O_CREAT, 0755);
    }
    else {
        fd = open(fname, O_WRONLY);
    }

    if (fd >= 0) {
        if (write(fd, buf, len) != len) {
            printf("Failed to write file (%s)\n", fname);
        }
        close(fd);
    }
}

static int get_current_menu_layer(void)
{
    int cc = 0;
    const char *P0 = "Change Options";
    const char *P1 = "Frame skip type";
    const char *P2 = "D-Pad Up";
    const char *P3 = "Enter Menu";
    const char *P4 = "Username";
    const char *P5 = "KB Space: toggle cheat/folder    KB Left Ctrl: return to main menu";
    const char *P6 = "KB Space: select";

    for (cc=0; cc<drastic_menu.cnt; cc++) {
        if (!memcmp(drastic_menu.item[cc].msg, P0, strlen(P0))) {
            return NDS_DRASTIC_MENU_MAIN;
        }
        else if (!memcmp(drastic_menu.item[cc].msg, P1, strlen(P1))) {
            return NDS_DRASTIC_MENU_OPTION;
        }
        else if (!memcmp(drastic_menu.item[cc].msg, P2, strlen(P2))) {
            return NDS_DRASTIC_MENU_CONTROLLER;
        }
        else if (!memcmp(drastic_menu.item[cc].msg, P3, strlen(P3))) {
            return NDS_DRASTIC_MENU_CONTROLLER2;
        }
        else if (!memcmp(drastic_menu.item[cc].msg, P4, strlen(P4))) {
            return NDS_DRASTIC_MENU_FIRMWARE;
        }
        else if (!memcmp(drastic_menu.item[cc].msg, P5, strlen(P5))) {
            return NDS_DRASTIC_MENU_CHEAT;
        }
        else if (!memcmp(drastic_menu.item[cc].msg, P6, strlen(P6))) {
            return NDS_DRASTIC_MENU_ROM;
        }
    }
    return -1;
}

#ifdef UT
TEST(sdl2_video_miyoo, get_current_menu_layer)
{
    TEST_ASSERT_EQUAL(get_current_menu_layer(), -1);
}
#endif

static int draw_drastic_menu_main(void)
{
    int cc = 0;
    int div = 1;
    int w = 30;
    int h = 100;
    int draw = 0;
    int draw_shot = 0;
    int x = 0, y = 0;
    SDL_Rect rt = {0};
    CUST_MENU_SUB *p = NULL;
    char buf[MAX_PATH << 1] = {0};

    for (cc=0; cc<drastic_menu.cnt; cc++) {
        draw = 0;
        x = 90 / div;
        w = LINE_H / div;
        h = nds.enable_752x560 ? (115 / div) : (100 / div);

        memset(buf, 0, sizeof(buf));
        p = &drastic_menu.item[cc];
        if (p->y == 201) {
            draw = 1;
            snprintf(buf, sizeof(buf), "NDS %s", &p->msg[8]);
            x = FB_W - get_font_width(buf) - 10;
            y = 10 / div;
        }
        else if (p->y == 280) {
            draw = 1;
            y = h + (0 * w);
            strcpy(buf, to_lang("Change Options"));
        }
        else if (p->y == 288) {
            draw = 1;
            y = h + (1 * w);
            strcpy(buf, to_lang("Configure Controls"));
        }
        else if (p->y == 296) {
            draw = 1;
            y = h + (2 * w);
            strcpy(buf, to_lang("Configure Firmware"));
        }
        else if (p->y == 304) {
            draw = 1;
            y = h + (3 * w);
            strcpy(buf, to_lang("Configure Cheats"));
        }
        else if (p->y == 320) {
            draw = 1;
            y = h + (4 * w);
            snprintf(buf, sizeof(buf), "%s %s", to_lang("Load state"), &p->msg[13]);
        }
        else if (p->y == 328) {
            draw = 1;
            y = h + (5 * w);
            snprintf(buf, sizeof(buf), "%s %s", to_lang("Save state"), &p->msg[13]);
        }
        else if (p->y == 344) {
            draw = 1;
            y = h + (6 * w);
            strcpy(buf, to_lang("Load new game"));
        }
        else if (p->y == 352) {
            draw = 1;
            y = h + (7 * w);
            strcpy(buf, to_lang("Restart game"));
        }
        else if (p->y == 368) {
            draw = 1;
            y = h + (8 * w);
            strcpy(buf, to_lang("Return to game"));
        }
        else if (p->y == NDS_Hx2) {
            draw = 1;
            y = h + (9 * w);
            strcpy(buf, to_lang("Exit DraStic"));
        }

        if (draw) {
            if (p->bg) {
                rt.x = 5 / div;
                rt.y = y - (3 / div);
                rt.w = FB_W - (10 / div);
                rt.h = w;
                SDL_FillRect(nds.menu.drastic.main, &rt, SDL_MapRGB(nds.menu.drastic.main->format, 
                    (nds.menu.c2 >> 16) & 0xff, (nds.menu.c2 >> 8) & 0xff, nds.menu.c2 & 0xff));
                if ((p->y == 320) || (p->y == 328)) {
                    draw_shot = 1;
                }

                if (nds.menu.show_cursor && nds.menu.drastic.cursor) {
                    rt.x = (5 / div) + (x - nds.menu.drastic.cursor->w) / 2;
                    rt.y -= ((nds.menu.drastic.cursor->h - LINE_H) / 2);
                    rt.w = 0;
                    rt.h = 0;
                    SDL_BlitSurface(nds.menu.drastic.cursor, NULL, nds.menu.drastic.main, &rt);
                }
            }
            draw_info(nds.menu.drastic.main, buf, x, y, p->bg ? nds.menu.c0 : nds.menu.c1, 0);
        }
    }

    y = 10;

#ifdef A30
    if (nds.chk_bat) {
        snprintf(buf, sizeof(buf), "Rel "NDS_VER", Res %s, BAT %d%%", "640*480", get_bat_val());
    }
    else {
        snprintf(buf, sizeof(buf), "Rel "NDS_VER", Res %s", "640*480");
    }
#endif

#ifdef MINI
    if (nds.chk_bat) {
        snprintf(buf, sizeof(buf), "Rel "NDS_VER", Res %s, BAT %d%%", nds.enable_752x560 ? "752*560" : "640*480", get_bat_val());
    }
    else {
        snprintf(buf, sizeof(buf), "Rel "NDS_VER", Res %s", nds.enable_752x560 ? "752*560" : "640*480");
    }
#endif

    draw_info(nds.menu.drastic.main, buf, 10, y / div, nds.menu.c1, 0);

    if (draw_shot) {
        const uint32_t len = NDS_W * NDS_H * 2;
        uint16_t *top = malloc(len);
        uint16_t *bottom = malloc(len);

        if (top && bottom) {
            SDL_Surface *t = NULL;
            uint32_t slot = 0;//*((uint32_t *)VAR_SYSTEM_SAVESTATE_NUM);
            //nds_load_state_index _func = (nds_load_state_index)FUN_LOAD_STATE_INDEX;

            memset(top, 0, len);
            memset(bottom, 0, len);
            //_func((void*)VAR_SYSTEM, slot, top, bottom, 1);
            t = SDL_CreateRGBSurfaceFrom(top, NDS_W, NDS_H, 16, NDS_W * 2, 0, 0, 0, 0);
            if (t) {
                rt.x = FB_W - (NDS_W + (nds.enable_752x560 ? 30 : 10));
                rt.y = nds.enable_752x560 ? h - 20 : 50;
                rt.w = NDS_W;
                rt.h = NDS_H;
                SDL_BlitSurface(t, NULL, nds.menu.drastic.main, &rt);
                SDL_FreeSurface(t);
            }

            t = SDL_CreateRGBSurfaceFrom(bottom, NDS_W, NDS_H, 16, NDS_W * 2, 0, 0, 0, 0);
            if (t) {
                rt.x = FB_W - (NDS_W + (nds.enable_752x560 ? 30 : 10));
                rt.y = nds.enable_752x560 ? (h + NDS_H) - 20 : 50 + NDS_H;
                rt.w = NDS_W;
                rt.h = NDS_H;
                SDL_BlitSurface(t, NULL, nds.menu.drastic.main, &rt);
                SDL_FreeSurface(t);
            }
        }

        if (top) {
            free(top);
        }

        if (bottom) {
            free(bottom);
        }
    }
    return 0;
}

static int mark_double_spaces(char *p)
{
    int cc = 0;
    int len = strlen(p);

    for (cc=0; cc<len - 1; cc++) {
        if ((p[cc] == ' ') && (p[cc + 1] == ' ')) {
            p[cc] = 0;
            return 0;
        }
    }
    return -1;
}

static char* find_menu_string_tail(char *p)
{
    int cc = 0;

    for (cc=strlen(p) - 1; cc>=0; cc--) {
        if (p[cc] == ' ') {
            return &p[cc + 1];
        }
    }
    return NULL;
}

static int draw_drastic_menu_option(void)
{
    int w = 0;
    int y = 0;
    int ww = 0;
    int s0 = 0;
    int s1 = 0;
    int cc = 0;
    int div = 1;
    int cnt = 0;
    int cursor = 0;
    SDL_Rect rt = {0};
    CUST_MENU_SUB *p = NULL;
    char buf[MAX_PATH] = {0};

    cursor = 0;
    for (cc=0; cc<drastic_menu.cnt; cc++) {
        if (drastic_menu.item[cc].bg > 0) {
            cursor = cc;
        }
    }

    if (cursor <= 6) {
        s0 = 1;
        s1 = 14;
    }
    else if (cursor >= (drastic_menu.cnt - 7)) {
        s0 = drastic_menu.cnt - 14;
        s1 = drastic_menu.cnt - 1;
    }
    else {
        s0 = cursor - 6;
        s1 = cursor + 7;
    }

    for (cc=0; cc<drastic_menu.cnt; cc++) {
        ww = LINE_H / div;

        if ((cc >= s0) && (cc < s1)) {
            y = (25 / div) + (cnt * ww);
            memset(buf, 0, sizeof(buf));
            p = &drastic_menu.item[cc];
        
            cnt+= 1;            
            if (p->bg) {
                rt.x = 5 / div;
                rt.y = y - (3 / div);
                rt.w = FB_W - (10 / div);
                rt.h = ww;
                SDL_FillRect(nds.menu.drastic.main, &rt, SDL_MapRGB(nds.menu.drastic.main->format, 
                    (nds.menu.c2 >> 16) & 0xff, (nds.menu.c2 >> 8) & 0xff, nds.menu.c2 & 0xff));
            }

            if (p->y <= NDS_Hx2) {
                strcpy(buf, to_lang(find_menu_string_tail(p->msg)));
                w = get_font_width(buf);
                draw_info(nds.menu.drastic.main, buf, FB_W - w - (ww / div), y, p->bg ? nds.menu.c0 : nds.menu.c1, 0);

                mark_double_spaces(p->msg);
                strcpy(buf, to_lang(p->msg));
            }
            else {
                strcpy(buf, to_lang(p->msg));
            }
            draw_info(nds.menu.drastic.main, buf, ww / div, y, p->bg ? nds.menu.c0 : nds.menu.c1, 0);
        }
    }
    return 0;
}

static int draw_drastic_menu_controller(void)
{
    int y = 0;
    int w = 0;
    int cc = 0;
    int div = 1;
    int cnt = 0;
    int cursor = 0;
    SDL_Rect rt = {0};
    int s0 = 0, s1 = 0;
    CUST_MENU_SUB *p = NULL;
    char buf[MAX_PATH] = {0};

    cursor = 0;
    for (cc=0; cc<drastic_menu.cnt;) {
        if ((drastic_menu.item[cc].y >= 240) && (drastic_menu.item[cc].y <= 376)) {
            if ((drastic_menu.item[cc + 1].bg > 0) || (drastic_menu.item[cc + 2].bg > 0)) {
                break;
            }
            cc+= 3;
        }
        else {
            if (drastic_menu.item[cc].bg > 0) {
                break;
            }
            cc+= 1;
        }
        cursor+= 1;
    }
    
    if (cursor <= 6) {
        s0 = 0;
        s1 = 13;
    }
    else if (cursor >= (24 - 7)) {
        s0 = 24 - 13;
        s1 = 24;
    }
    else {
        s0 = cursor - 6;
        s1 = cursor + 7;
    }

    cnt = 0;
    for (cc=0; cc<drastic_menu.cnt; cc++) {
        w = LINE_H / div;
        p = &drastic_menu.item[cc];

        if ((p->y == 224) || (p->y == 232) || (p->y == 201)) {
            continue;
        }

        memset(buf, 0, sizeof(buf));
        if ((cnt >= s0) && (cnt < s1)) {
            y = (25 / div) + ((cnt - s0) * w);

            if ((p->y >= 240) && (p->y <= 376)) {
                if (drastic_menu.item[cc + 1].bg || drastic_menu.item[cc + 2].bg) {
                    int sum = drastic_menu.item[cc + 1].bg + drastic_menu.item[cc + 2].bg;
                    uint32_t c = sum > 500 ? 0xff0000 : nds.menu.c2;

                    rt.x = 5 / div;
                    rt.y = y - (3 / div);
                    rt.w = FB_W - (10 / div);
                    rt.h = w;
                    SDL_FillRect(nds.menu.drastic.main, &rt, SDL_MapRGB(nds.menu.drastic.main->format, (c >> 16) & 0xff, (c >> 8) & 0xff, c & 0xff));
                }
                draw_info(nds.menu.drastic.main, p->msg, 20 / div, y, p->bg ? nds.menu.c0 : nds.menu.c1, 0);
                if ((p->y >= 240) && (p->y <= 376)) {
                    if (nds.enable_752x560) {
                        draw_info(nds.menu.drastic.main, to_lang(drastic_menu.item[cc + 1].msg), 320 / div, y, drastic_menu.item[cc + 1].bg ? nds.menu.c0 : nds.menu.c1, 0);
                        draw_info(nds.menu.drastic.main, to_lang(drastic_menu.item[cc + 2].msg), 550 / div, y, drastic_menu.item[cc + 2].bg ? nds.menu.c0 : nds.menu.c1, 0);
                    }
                    else {
                        draw_info(nds.menu.drastic.main, to_lang(drastic_menu.item[cc + 1].msg), 300 / div, y, drastic_menu.item[cc + 1].bg ? nds.menu.c0 : nds.menu.c1, 0);
                        draw_info(nds.menu.drastic.main, to_lang(drastic_menu.item[cc + 2].msg), 480 / div, y, drastic_menu.item[cc + 2].bg ? nds.menu.c0 : nds.menu.c1, 0);
                    }
                }
            }
            else {
                if (p->bg) {
                    rt.x = 5 / div;
                    rt.y = y - (3 / div);
                    rt.w = FB_W - (10 / div);
                    rt.h = w;
                    SDL_FillRect(nds.menu.drastic.main, &rt, SDL_MapRGB(nds.menu.drastic.main->format, 
                        (nds.menu.c2 >> 16) & 0xff, (nds.menu.c2 >> 8) & 0xff, nds.menu.c2 & 0xff));
                }
                draw_info(nds.menu.drastic.main, to_lang(p->msg), 20 / div, y, p->bg ? nds.menu.c0 : nds.menu.c1, 0);
            }
        }

        cnt+= 1;
        if ((p->y >= 240) && (p->y <= 376)) {
            cc+= 2;
        }
    }
    return 0;
}

static int draw_drastic_menu_controller2(void)
{
    int y = 0;
    int w = 0;
    int cc = 0;
    int cnt = 0;
    int div = 1;
    int cursor = 0;
    SDL_Rect rt = {0};
    int s0 = 0, s1 = 0;
    CUST_MENU_SUB *p = NULL;
    char buf[MAX_PATH] = {0};

    cursor = 0;
    for (cc=0; cc<drastic_menu.cnt;) {
        if ((drastic_menu.item[cc].y >= 240) && (drastic_menu.item[cc].y <= NDS_Hx2)) {
            if ((drastic_menu.item[cc + 1].bg > 0) || (drastic_menu.item[cc + 2].bg > 0)) {
                break;
            }
            cc+= 3;
        }
        else {
            if (drastic_menu.item[cc].bg > 0) {
                break;
            }
            cc+= 1;
        }
        cursor+= 1;
    }
    
    if (cursor <= 6) {
        s0 = 0;
        s1 = 13;
    }
    else if (cursor >= (23 - 7)) {
        s0 = 23 - 13;
        s1 = 23;
    }
    else {
        s0 = cursor - 6;
        s1 = cursor + 7;
    }

    cnt = 0;
    for (cc=0; cc<drastic_menu.cnt; cc++) {
        w = LINE_H / div;
        p = &drastic_menu.item[cc];

        if ((p->y == 224) || (p->y == 232) || (p->y == 201)) {
            continue;
        }

        memset(buf, 0, sizeof(buf));
        if ((cnt >= s0) && (cnt < s1)) {
            y = (25 / div) + ((cnt - s0) * w);

            if ((p->y >= 240) && (p->y <= NDS_Hx2)) {
                if (drastic_menu.item[cc + 1].bg || drastic_menu.item[cc + 2].bg) {
                    int sum = drastic_menu.item[cc + 1].bg + drastic_menu.item[cc + 2].bg;
                    uint32_t c = sum > 500 ? 0xff0000 : nds.menu.c2;

                    rt.x = 5 / div;
                    rt.y = y - (3 / div);
                    rt.w = FB_W - (10 / div);
                    rt.h = w;
                    SDL_FillRect(nds.menu.drastic.main, &rt, SDL_MapRGB(nds.menu.drastic.main->format, (c >> 16) & 0xff, (c >> 8) & 0xff, c & 0xff));
                }
                draw_info(nds.menu.drastic.main, p->msg, 20 / div, y, p->bg ? nds.menu.c0 : nds.menu.c1, 0);
                if ((p->y >= 240) && (p->y <= NDS_Hx2)) {
                    if (nds.enable_752x560) {
                        draw_info(nds.menu.drastic.main, to_lang(drastic_menu.item[cc + 1].msg), 320 / div, y, drastic_menu.item[cc + 1].bg ? nds.menu.c0 : nds.menu.c1, 0);
                        draw_info(nds.menu.drastic.main, to_lang(drastic_menu.item[cc + 2].msg), 550 / div, y, drastic_menu.item[cc + 2].bg ? nds.menu.c0 : nds.menu.c1, 0);
                    }
                    else {
                        draw_info(nds.menu.drastic.main, to_lang(drastic_menu.item[cc + 1].msg), 300 / div, y, drastic_menu.item[cc + 1].bg ? nds.menu.c0 : nds.menu.c1, 0);
                        draw_info(nds.menu.drastic.main, to_lang(drastic_menu.item[cc + 2].msg), 480 / div, y, drastic_menu.item[cc + 2].bg ? nds.menu.c0 : nds.menu.c1, 0);
                    }
                }
            }
            else {
                if (p->bg) {
                    rt.x = 5 / div;
                    rt.y = y - (3 / div);
                    rt.w = FB_W - (10 / div);
                    rt.h = w;
                    SDL_FillRect(nds.menu.drastic.main, &rt, SDL_MapRGB(nds.menu.drastic.main->format, 
                        (nds.menu.c2 >> 16) & 0xff, (nds.menu.c2 >> 8) & 0xff, nds.menu.c2 & 0xff));
                }
                draw_info(nds.menu.drastic.main, to_lang(p->msg), 20 / div, y, p->bg ? nds.menu.c0 : nds.menu.c1, 0);
            }
        }

        cnt+= 1;
        if ((p->y >= 240) && (p->y <= NDS_Hx2)) {
            cc+= 2;
        }
    }
    return 0;
}

static int draw_drastic_menu_firmware(void)
{
    int t = 0;
    int w = 0;
    int y = 0;
    int ww = 30;
    int cc = 0;
    int div = 1;
    int cnt = 0;
    SDL_Rect rt = {0};
    CUST_MENU_SUB *p = NULL;
    char buf[MAX_PATH] = {0};
    char name[MAX_PATH] = {0};

    for (cc=0; cc<drastic_menu.cnt; cc++) {
        ww = LINE_H / div;
        p = &drastic_menu.item[cc];
        if ((p->x == 352) || (p->x == 108)) {
            continue;
        }
    
        memset(buf, 0, sizeof(buf));
        if ((p->x != 92) && (p->x != 256)) {
            strcat(name, p->msg);
        }
        else {
            y = (25 / div) + (cnt * ww);
            if (((p->x == 92) || (p->x == 256)) && (p->bg)) {
                rt.x = 5 / div;
                rt.y = y - (3 / div);
                rt.w = FB_W - (10 / div);
                rt.h = ww;
                SDL_FillRect(nds.menu.drastic.main, &rt, SDL_MapRGB(nds.menu.drastic.main->format, 
                    (nds.menu.c2 >> 16) & 0xff, (nds.menu.c2 >> 8) & 0xff, nds.menu.c2 & 0xff));
            }

            cnt+= 1;
            if (p->y == 280) {
                mark_double_spaces(p->msg);
                strcpy(buf, to_lang(p->msg));
            }
            else if (p->y == 296) {
                w = get_font_width(name);
                draw_info(nds.menu.drastic.main, name, FB_W - w - (ww / div), 25 / div, nds.menu.c1, 0);

                w = strlen(p->msg);
                p->msg[w - 3] = 0;
                for (t=14; t<w; t++) {
                    if (p->msg[t] != ' ') {
                        strcpy(buf, &p->msg[t]);
                        break;
                    }
                }
                w = get_font_width(buf);
                draw_info(nds.menu.drastic.main, buf, FB_W - w - (ww / div), y, p->bg ? nds.menu.c0 : nds.menu.c1, 0);

                strcpy(buf, to_lang("Favorite Color"));
            }
            else if (p->y <= 312) {
                strcpy(buf, to_lang(find_menu_string_tail(p->msg)));
                w = get_font_width(buf);
                draw_info(nds.menu.drastic.main, buf, FB_W - w - (ww / div), y, p->bg ? nds.menu.c0 : nds.menu.c1, 0);

                mark_double_spaces(p->msg);
                strcpy(buf, to_lang(p->msg));
            }
            else {
                strcpy(buf, to_lang(p->msg));
            }
            draw_info(nds.menu.drastic.main, buf, ww / div, y, p->bg ? nds.menu.c0 : nds.menu.c1, 0);
        }
    }
    return 0;
}

static int draw_drastic_menu_cheat(void)
{
    int y = 0;
    int w = 30;
    int cc = 0;
    int div = 1;
    int cnt = 0;
    int cursor = 0;
    SDL_Rect rt = {0};
    int s0 = 0, s1 = 0;
    CUST_MENU_SUB *p = NULL;
    char buf[MAX_PATH] = {0};

    for (cc=0; cc<drastic_menu.cnt; cc++) {
        p = &drastic_menu.item[cc];
        if (p->x == 650) {
            for (s0=0; s0<drastic_menu.cnt; s0++) {
                if ((drastic_menu.item[s0].x == 10) && (drastic_menu.item[s0].y == p->y)) {
                    drastic_menu.item[s0].cheat = 1;
                    drastic_menu.item[s0].enable = strcmp(p->msg, "enabled") == 0 ? 1 : 0;
                    break;
                }
            }
        }
    }

    s0 = 0;
    for (cc=0; cc<drastic_menu.cnt; cc++) {
        if (drastic_menu.item[cc].x == 10) {
            memcpy(&drastic_menu.item[s0], &drastic_menu.item[cc], sizeof(drastic_menu.item[cc]));
            s0+= 1;
        }
        memset(&drastic_menu.item[cc], 0, sizeof(drastic_menu.item[cc]));
    }
    drastic_menu.cnt = s0;

    cursor = 0;
    for (cc=0; cc<drastic_menu.cnt; cc++) {
        if (drastic_menu.item[cc].bg > 0) {
            cursor = cc;
        }
    }

    if (drastic_menu.cnt == 0) {
        return 0;
    }

    if (drastic_menu.cnt < 13) {
        s0 = 0;
        s1 = drastic_menu.cnt;
    }
    else if (cursor <= 6) {
        s0 = 0;
        s1 = 13;
    }
    else if (cursor >= (drastic_menu.cnt - 7)) {
        s0 = drastic_menu.cnt - 13;
        s1 = drastic_menu.cnt;
    }
    else {
        s0 = cursor - 6;
        s1 = cursor + 7;
    }

    cnt = 0;
    for (cc=0; cc<drastic_menu.cnt; cc++) {
        w = LINE_H / div;
        memset(buf, 0, sizeof(buf));
        p = &drastic_menu.item[cc];

        if (p->x != 10) {
            continue;
        }

        if ((cc >= s0) && (cc < s1)) {
            y = (25 / div) + (cnt * w);

            if (p->bg) {
                rt.x = 5 / div;
                rt.y = y - (3 / div);
                rt.w = FB_W - (10 / div);
                rt.h = w;
                SDL_FillRect(nds.menu.drastic.main, &rt, SDL_MapRGB(nds.menu.drastic.main->format,
                    (nds.menu.c2 >> 16) & 0xff, (nds.menu.c2 >> 8) & 0xff, nds.menu.c2 & 0xff));
            }

            cnt+= 1;
            draw_info(nds.menu.drastic.main, p->msg, w / div, y, p->bg ? nds.menu.c0 : nds.menu.c1, 0);
            if (p->cheat && nds.menu.drastic.yes && nds.menu.drastic.no) {
                rt.x = FB_W - nds.menu.drastic.yes->w - (w / div);
                rt.y = y - 1;
                rt.w = 0;
                rt.h = 0;
                SDL_BlitSurface((p->enable > 0 ) ? nds.menu.drastic.yes : nds.menu.drastic.no, NULL, nds.menu.drastic.main, &rt);
            }
        }
    }
    return 0;
}

static int draw_drastic_menu_rom(void)
{
    int y = 0;
    int w = 0;
    int cc = 0;
    int div = 1;
    int chk = 0;
    int all = 0;
    int cnt = 0;
    int cursor = 0;
    SDL_Rect rt = {0};
    int s0 = 0, s1 = 0;
    CUST_MENU_SUB *p = NULL;

    for (cc=0; cc<drastic_menu.cnt; cc++) {
        if (drastic_menu.item[cc].x == 10) {
            if (drastic_menu.item[cc].bg > 0) {
                chk = 10;
                break;
            }
        }
        if (drastic_menu.item[cc].x == 587) {
            if (drastic_menu.item[cc].bg > 0) {
                chk = 587;
                break;
            }
        }
    }

    cursor = 0;
    for (cc=0; cc<drastic_menu.cnt; cc++) {
        if (drastic_menu.item[cc].x == chk) {
            if (drastic_menu.item[cc].bg > 0) {
                break;
            }
            cursor+= 1;
        }
    }

    all = 0;
    for (cc=0; cc<drastic_menu.cnt; cc++) {
        if (drastic_menu.item[cc].x == chk) {
            all+= 1;
        }
    }

    if (all < 12) {
        s0 = 0;
        s1 = all;
    }
    else if (cursor <= 6) {
        s0 = 0;
        s1 = 12;
    }
    else if (cursor >= (all - 6)) {
        s0 = all - 12;
        s1 = all;
    }
    else {
        s0 = cursor - 6;
        s1 = cursor + 6;
    }

    {
        uint32_t c = 0x335445;

        w = LINE_H / div;
        p = &drastic_menu.item[0];
        rt.x = 5 / div;
        rt.y = (25 / div) - (4 / div);
        rt.w = FB_W - (10 / div);
        rt.h = w;
        SDL_FillRect(nds.menu.drastic.main, &rt, SDL_MapRGB(nds.menu.drastic.main->format, (c >> 16) & 0xff, (c >> 8) & 0xff, c & 0xff));
        draw_info(nds.menu.drastic.main, p->msg, 20 / div, 25 / div, 0xa0cb93, 0);
    }

    cnt = 0;
    for (cc=0; cc<drastic_menu.cnt; cc++) {
        w = LINE_H / div;
        p = &drastic_menu.item[cc];
        if (p->x == chk) {
            y = (25 / div) + (((cnt - s0) + 1) * w);
            if ((cnt >= s0) && (cnt < s1)) {
                if (p->bg) {
                    rt.x = 5 / div;
                    rt.y = y - (4 / div);
                    rt.w = FB_W - (10 / div);
                    rt.h = w;
                    SDL_FillRect(nds.menu.drastic.main, &rt, SDL_MapRGB(nds.menu.drastic.main->format,
                        (nds.menu.c2 >> 16) & 0xff, (nds.menu.c2 >> 8) & 0xff, nds.menu.c2 & 0xff));
                }
                draw_info(nds.menu.drastic.main, p->msg, 20 / div, y, p->bg ? nds.menu.c0 : nds.menu.c1, 0);
            }
            cnt+= 1;
        }
    }
    return 0;
}

int process_drastic_menu(void)
{
    int layer = get_current_menu_layer();

#if defined(UT)
    return 0;
#endif

    if (layer == NDS_DRASTIC_MENU_MAIN) {
        SDL_SoftStretch(nds.menu.drastic.bg0, NULL, nds.menu.drastic.main, NULL);
    }
    else {
        SDL_SoftStretch(nds.menu.drastic.bg1, NULL, nds.menu.drastic.main, NULL);
    }

    switch (layer) {
    case NDS_DRASTIC_MENU_MAIN:
        draw_drastic_menu_main();
        break;
    case NDS_DRASTIC_MENU_OPTION:
        draw_drastic_menu_option();
        break;
    case NDS_DRASTIC_MENU_CONTROLLER:
        draw_drastic_menu_controller();
        break;
    case NDS_DRASTIC_MENU_CONTROLLER2:
        draw_drastic_menu_controller2();
        break;
    case NDS_DRASTIC_MENU_FIRMWARE:
        draw_drastic_menu_firmware();
        break;
    case NDS_DRASTIC_MENU_CHEAT:
        draw_drastic_menu_cheat();
        break;
    case NDS_DRASTIC_MENU_ROM:
        draw_drastic_menu_rom();
        break;
    default:
        SDL_SendKeyboardKey(SDL_PRESSED, SDL_GetScancodeFromKey(SDLK_RIGHT));
        usleep(100000);
        SDL_SendKeyboardKey(SDL_RELEASED, SDL_GetScancodeFromKey(SDLK_RIGHT));
        memset(&drastic_menu, 0, sizeof(drastic_menu));
        return 0;
    }
#if defined(A30)
    nds.update_menu = 1;
#endif

#if defined(MINI)
    GFX_Copy(-1, nds.menu.drastic.main->pixels, nds.menu.drastic.main->clip_rect, nds.menu.drastic.main->clip_rect, nds.menu.drastic.main->pitch, 0, E_MI_GFX_ROTATE_180);
    GFX_Flip();
    GFX_Copy(-1, nds.menu.drastic.main->pixels, nds.menu.drastic.main->clip_rect, nds.menu.drastic.main->clip_rect, nds.menu.drastic.main->pitch, 0, E_MI_GFX_ROTATE_180);
    GFX_Flip();
#endif
    memset(&drastic_menu, 0, sizeof(drastic_menu));
    return 0;
}

static int process_screen(void)
{
    static int need_loadstate = 15;
    static int show_info_cnt = 0;
    static int cur_fb_w = 0;
    static int cur_volume = 0;
    static int cur_dis_mode = 0;
    static int cur_touchpad = 0;
    static int cur_theme_sel = 0;
    static int cur_pixel_filter = 0;
    static int pre_dis_mode = NDS_LAYOUT_8;
    static int pre_hres_mode = NDS_LAYOUT_17;
    static char show_info_buf[MAX_PATH << 1] = {0};
    static int bat_chk_cnt = BAT_CHK_CNT;
    static int col_fg = 0xe0e000;
    static int col_bg = 0x000000;

    int idx = 0;
    int screen_cnt = 0;
    char buf[MAX_PATH] = {0};
    int cur_sel = gfx.lcd.cur_sel ^ 1;

    screen_cnt = 2;
    if (nds.auto_state > 0) {
        if (need_loadstate > 0) {
            need_loadstate-= 1;
            if (need_loadstate == 0) {
                invoke_drastic_load_state(nds.auto_slot);
            }
        }
    }

    if (nds.menu.enable) {
        need_reload_bg = RELOAD_BG_COUNT;
        return 0;
    }

    if (nds.menu.drastic.enable) {
        nds.menu.drastic.enable = 0;
        need_reload_bg = RELOAD_BG_COUNT;
    }

    if ((cur_fb_w != FB_W) ||
        (nds.shot.take) ||
        (cur_touchpad != nds.pen.pos) ||
        (cur_dis_mode != nds.dis_mode) ||
        (cur_theme_sel != nds.theme.sel) ||
        (cur_pixel_filter != pixel_filter) ||
        (cur_volume != nds.volume))
    {
        if (cur_fb_w != FB_W) {
            show_info_cnt = 150;
            snprintf(show_info_buf, sizeof(show_info_buf), " %d x %d ", FB_W, FB_H);
        }
        else if (cur_volume != nds.volume) {
            show_info_cnt = 50;
            snprintf(show_info_buf, sizeof(show_info_buf), " %s %d ", to_lang("Volume"), nds.volume);
        }
        else if (cur_touchpad != nds.pen.pos) {
            show_info_cnt = 50;
            snprintf(show_info_buf, sizeof(show_info_buf), " %s %d ", to_lang("Touchpad"), nds.pen.pos);
        }
        else if (cur_theme_sel != nds.theme.sel) {
            show_info_cnt = 50;
            if ((nds.theme.max > 0) && (nds.theme.sel < nds.theme.max)) {
                if (get_dir_path(nds.theme.path, nds.theme.sel, buf) == 0) {
                    int off = strlen(nds.theme.path);
                    snprintf(show_info_buf, sizeof(show_info_buf), " %s %s ", to_lang("Wallpaper"), &buf[off + 1]);
                }
                else {
                    snprintf(show_info_buf, sizeof(show_info_buf), " %s %d ", to_lang("Wallpaper"), nds.theme.sel);
                }
            }
            else {
                snprintf(show_info_buf, sizeof(show_info_buf), " %s %s ", to_lang("Wallpaper"), to_lang("None"));
            }
        }
        else if (cur_pixel_filter != pixel_filter) {
            show_info_cnt = 50;
            snprintf(show_info_buf, sizeof(show_info_buf), " %s ", to_lang(pixel_filter ? "Pixel" : "Blur"));
        }
        else if (nds.shot.take) {
            show_info_cnt = 50;
            nds.shot.take = 0;
            snprintf(show_info_buf, sizeof(show_info_buf), " %s ", to_lang("Take Screenshot"));
        }

        cur_fb_w = FB_W;
        cur_theme_sel = nds.theme.sel;
        cur_volume = nds.volume;
        cur_dis_mode = nds.dis_mode;
        cur_touchpad = nds.pen.pos;
        cur_pixel_filter = pixel_filter;
        need_reload_bg = RELOAD_BG_COUNT;
    }

    if (show_info_cnt == 0) {
        need_reload_bg = RELOAD_BG_COUNT;
        show_info_cnt = -1;
        col_fg = 0xe0e000;
        col_bg = 0x000000;
    }
        
    if (nds.defer_update_bg > 0) {
        nds.defer_update_bg-= 1;
    }
    else if (nds.defer_update_bg == 0) {
        nds.defer_update_bg = -1;
        need_reload_bg = RELOAD_BG_COUNT;
    }

    if (nds.state) {
        if (nds.state & NDS_STATE_QSAVE) {
            show_info_cnt = 50;
            snprintf(show_info_buf, sizeof(show_info_buf), " %s ", to_lang("Quick Save"));
            nds.state&= ~NDS_STATE_QSAVE;
        }
        else if (nds.state & NDS_STATE_QLOAD) {
            show_info_cnt = 50;
            snprintf(show_info_buf, sizeof(show_info_buf), " %s ", to_lang("Quick Load"));
            nds.state&= ~NDS_STATE_QLOAD;
        }
        else if (nds.state & NDS_STATE_FF) {
            show_info_cnt = 50;
            snprintf(show_info_buf, sizeof(show_info_buf), " %s ", to_lang("Fast Forward"));
            nds.state&= ~NDS_STATE_FF;
        }
    }

    if (nds.chk_bat) {
        bat_chk_cnt -= 1;
        if (bat_chk_cnt <= 0) {
            int v = get_bat_val();

            if (v <= 10) {
                show_info_cnt = 50;
                col_fg = 0xffffff;
                col_bg = 0xff0000;
                snprintf(show_info_buf, sizeof(show_info_buf), " %s %d%% ", to_lang("Battery"), v);
            }

            bat_chk_cnt = BAT_CHK_CNT;
            if (v <= 5) {
                bat_chk_cnt /= 2;
            }
        }
    }

//    nds.screen.bpp = *((uint32_t *)VAR_SDL_SCREEN_BPP);
//    nds.screen.init = *((uint32_t *)VAR_SDL_SCREEN_NEED_INIT);

    if (need_reload_bg) {
        reload_bg();
        need_reload_bg -= 1;
    }

    for (idx = 0; idx < screen_cnt; idx++) {
        int screen0 = 0;
        int screen1 = 0;
        int show_pen = 1;
        int need_update = 1;
        int rotate = E_MI_GFX_ROTATE_180;
        SDL_Rect srt = {0, 0, NDS_W, NDS_H};
        SDL_Rect drt = {0, 0, 160, 120};

//        nds.screen.hres_mode[idx] = idx ?
//            *((uint8_t *)VAR_SDL_SCREEN1_HRES_MODE):
//            *((uint8_t *)VAR_SDL_SCREEN0_HRES_MODE);

        nds.screen.pixels[idx] = gfx.lcd.virAddr[cur_sel][idx];
        if (nds.screen.hres_mode[idx]) {
            srt.w = NDS_Wx2;
            srt.h = NDS_Hx2;
            nds.screen.pitch[idx] = nds.screen.bpp * srt.w;
            if (nds.hres_mode == 0) {
                nds.pen.pos = 0;
                nds.hres_mode = 1;
                pre_dis_mode = nds.dis_mode;
                nds.dis_mode = pre_hres_mode;
            }
        }
        else {
            srt.w = NDS_W;
            srt.h = NDS_H;
            nds.screen.pitch[idx] = nds.screen.bpp * srt.w;

            drt.y = idx * 120;
            screen0 = (idx == 0);
            screen1 = (idx != 0);
            show_pen = nds.pen.pos ? screen1 : screen0;
            if (nds.hres_mode == 1) {
                nds.hres_mode = 0;
                pre_hres_mode = nds.dis_mode;
                nds.dis_mode = pre_dis_mode;
            }
        }

        switch (nds.dis_mode) {
        case NDS_LAYOUT_0:
            if (screen1) {
                drt.x = 0;
                drt.y = 0;
                drt.w = FB_W;
                drt.h = FB_H;
                show_pen = 1;
                nds.pen.pos = 1;
            }
            else {
                show_pen = 0;
                need_update = 0;
            }
            break;
        case NDS_LAYOUT_1:
            if (screen1) {
                drt.x = 0;
                drt.y = 0;
                drt.w = FB_W;
                drt.h = FB_H;
                show_pen = 1;
                nds.pen.pos = 1;
            }
            else {
                show_pen = 0;
                need_update = 0;
            }
            break;
        case NDS_LAYOUT_2:
            if (screen1) {
                drt.w = NDS_W * 2;
                drt.h = NDS_H * 2;
                drt.x = (FB_W - drt.w) / 2;
                drt.y = (FB_H - drt.h) / 2;
                show_pen = 1;
                nds.pen.pos = 1;
            }
            else {
                show_pen = 0;
                need_update = 0;
            }
            break;
        case NDS_LAYOUT_3:
            if (screen1) {
                drt.x = 0;
                drt.y = 0;
                drt.w = FB_W;
                drt.h = FB_H;
                show_pen = 1;
                nds.pen.pos = 1;
            }
            else {
                show_pen = 0;
                need_update = 0;
            }
            break;
        case NDS_LAYOUT_4:
            drt.w = NDS_W;
            drt.h = NDS_H;
            drt.x = (FB_W - drt.w) / 2;
            if (nds.enable_752x560 == 0) {
                drt.y = screen0 ? 48 : 48 + drt.h;
            }
            else {
                drt.y = screen0 ? 88 : 88 + drt.h;
            }
            break;
        case NDS_LAYOUT_5:
            if (nds.enable_752x560 == 0) {
                drt.w = 320;
                drt.h = 240;
            }
            else {
                drt.w = 373;
                drt.h = 280;
            }
            drt.x = (FB_W - drt.w) / 2;
            drt.y = screen0 ? 0 : drt.h;
            break;
        case NDS_LAYOUT_6:
            drt.w = NDS_W;
            drt.h = NDS_H;
            if (nds.enable_752x560 == 0) {
                drt.x = screen0 ? 64 : 64 + drt.w;
            }
            else {
                drt.x = screen0 ? 120 : 120 + drt.w;
            }
            drt.y = (FB_H - drt.h) / 2;
            break;
        case NDS_LAYOUT_7:
            if (nds.enable_752x560 == 0) {
                drt.w = 320;
                drt.h = 240;
                drt.x = screen0 ? 0 : drt.w;
            }
            else {
                drt.w = 373;
                drt.h = 280;
                drt.x = screen0 ? 3 : (drt.w + 3);
            }
            drt.y = (FB_H - drt.h) / 2;
            break;
        case NDS_LAYOUT_8:
            drt.x = screen1 ? 160 : 0;
            drt.y = screen1 ? 120 : 0;
            drt.w = screen1 ? (FB_W - 160) : 160;
            drt.h = screen1 ? (FB_H - 120) : 120;
            break;
        case NDS_LAYOUT_9:
            drt.x = screen1 ? NDS_W : 0;
            drt.y = screen1 ? NDS_H : 0;
            drt.w = screen1 ? (FB_W - NDS_W) : NDS_W;
            drt.h = screen1 ? (FB_H - NDS_H) : NDS_H;
            break;
        case NDS_LAYOUT_16:
            drt.w = screen1 ? (FB_W - 160) : 160;
            drt.h = screen1 ? (FB_H - 120) : 120;
            drt.x = screen1 ? ((FB_W - drt.w) / 2) : ((FB_W - drt.w) / 2);
            drt.y = screen1 ? 120 : 0;
            break;
        case NDS_LAYOUT_10:
            drt.w = screen0 ? NDS_W : (FB_W - NDS_W);
            drt.h = screen0 ? NDS_H : (FB_H - NDS_H);
            drt.x = screen0 ? ((FB_W - drt.w) / 2) : ((FB_W - drt.w) / 2);
            drt.y = screen0 ? 0 : NDS_H;
            break;
        case NDS_LAYOUT_11:
            drt.w = screen0 ? NDS_W : (FB_W - NDS_W);
            drt.h = screen0 ? NDS_H : (FB_H - NDS_H);
            drt.x = screen0 ? 0 : NDS_W;
            drt.y = screen0 ? ((FB_H - drt.h) / 2) : ((FB_H - drt.h) / 2);
            break;
        case NDS_LAYOUT_12:
        case NDS_LAYOUT_13:
            if (nds.enable_752x560 == 0) {
                drt.x = screen0 ? 0 : 320;
                drt.y = 26;
                drt.w = 427;
                drt.h = 320;
            }
            else {
                drt.x = screen0 ? 0 : 376;
                drt.y = 29;
                drt.w = 501;
                drt.h = 376;
            }
            rotate = (nds.dis_mode == NDS_LAYOUT_12) ? E_MI_GFX_ROTATE_90 : E_MI_GFX_ROTATE_270;
            break;
        case NDS_LAYOUT_14:
        case NDS_LAYOUT_15:
            if (nds.enable_752x560 == 0) {
                drt.x = screen0 ? 0 : 320;
                drt.y = 0;
                drt.w = 480;
                drt.h = 320;
            }
            else {
                drt.x = screen0 ? 0 : 376;
                drt.y = 0;
                drt.w = 560;
                drt.h = 376;
            }
            rotate = (nds.dis_mode == NDS_LAYOUT_14) ? E_MI_GFX_ROTATE_90 : E_MI_GFX_ROTATE_270;
            break;
        case NDS_LAYOUT_17:
            drt.w = NDS_Wx2;
            drt.h = NDS_Hx2;
            drt.x = (FB_W - drt.w) / 2;
            drt.y = (FB_H - drt.h) / 2;
            break;
        case NDS_LAYOUT_18:
            drt.x = 0;
            drt.y = 0;
            drt.w = FB_W;
            drt.h = FB_H;
            break;
        }

#if defined(A30)
        if (rotate == E_MI_GFX_ROTATE_180) {
            drt.y = (DEF_FB_H - drt.y) - drt.h;
            drt.x = (DEF_FB_W - drt.x) - drt.w;
        }
        else if (rotate == E_MI_GFX_ROTATE_90) {
            drt.x = (drt.x == 0) ? 320 : 0;
        }

#ifdef A30
        if (show_pen && ((evt.dev_mode == MOUSE_MODE) || (nds.joy.show_cnt && (nds.joy.mode == MYJOY_MODE_STYLUS)))) {
#else
        if (show_pen && (evt.dev_mode == MOUSE_MODE)) {
#endif
#else
        if (show_pen && (evt.dev_mode == MOUSE_MODE)) {
#endif
            draw_pen(nds.screen.pixels[idx], srt.w, nds.screen.pitch[idx]);

#ifdef A30
            if (nds.joy.show_cnt && (nds.joy.mode == MYJOY_MODE_STYLUS)) {
                nds.joy.show_cnt -= 1;
            }
#endif
        }

#if defined(A30)
        if ((idx == 0) && (nds.alpha.border > 0) && ((nds.dis_mode == NDS_LAYOUT_0) || (nds.dis_mode == NDS_LAYOUT_1))) {
            int c0 = 0;
            uint32_t *p0 = NULL;
            uint32_t *p1 = NULL;
            uint32_t col[] = { 0, 0xffffff, 0xff0000, 0x00ff00, 0x0000ff, 0x000000, 0xffff00, 0x00ffff };

            p0 = (uint32_t *)nds.screen.pixels[idx];
            p1 = (uint32_t *)nds.screen.pixels[idx] + ((srt.h - 1) * srt.w);
            for (c0 = 0; c0 < srt.w; c0++) {
                *p0++ = col[nds.alpha.border];
                *p1++ = col[nds.alpha.border];
            }

            p0 = (uint32_t *)nds.screen.pixels[idx];
            p1 = (uint32_t *)nds.screen.pixels[idx] + (srt.w - 1);
            for (c0 = 0; c0 < srt.h; c0++) {
                *p0 = col[nds.alpha.border];
                *p1 = col[nds.alpha.border];
                p0 += srt.w;
                p1 += srt.w;
            }
        }
        glBindTexture(GL_TEXTURE_2D, vid.texID[idx]);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        if (pixel_filter) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }
        else {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, srt.w, srt.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nds.screen.pixels[idx]);
#endif

        if (need_update) {
#ifdef MINI
            MI_SYS_FlushInvCache(nds.screen.pixels[idx], nds.screen.pitch[idx] * srt.h);
#endif

#if defined(A30)
            GFX_Copy(idx, nds.screen.pixels[idx], srt, drt, nds.screen.pitch[idx], 0, rotate);
#else
            GFX_Copy(-1, nds.screen.pixels[idx], srt, drt, nds.screen.pitch[idx], 0, rotate);
#endif

#if defined(MINI) || defined(A30)
            switch (nds.dis_mode) {
            case NDS_LAYOUT_0:
                drt.x = 0;
                drt.y = 0;
                drt.w = 160;
                drt.h = 120;
#if defined(A30)
                switch (nds.alpha.pos) {
                case 0:
                    drt.x = DEF_FB_W - drt.w;
                    drt.y = 0;
                    break;
                case 1:
                    drt.x = 0;
                    drt.y = 0;
                    break;
                case 2:
                    drt.x = 0;
                    drt.y = DEF_FB_H - drt.h;
                    break;
                case 3:
                    drt.x = DEF_FB_W - drt.w;
                    drt.y = DEF_FB_H - drt.h;
                    break;
                }
                GFX_Copy(TEX_SCR0, nds.screen.pixels[0], srt, drt, nds.screen.pitch[0], 1, rotate);
#else
                GFX_Copy(-1, nds.screen.pixels[0], srt, drt, nds.screen.pitch[0], 1, rotate);
#endif
                break;
            case NDS_LAYOUT_1:
                drt.x = 0;
                drt.y = 0;
                drt.w = NDS_W;
                drt.h = NDS_H;
#if defined(A30)
                switch (nds.alpha.pos) {
                case 0:
                    drt.x = DEF_FB_W - drt.w;
                    drt.y = 0;
                    break;
                case 1:
                    drt.x = 0;
                    drt.y = 0;
                    break;
                case 2:
                    drt.x = 0;
                    drt.y = DEF_FB_H - drt.h;
                    break;
                case 3:
                    drt.x = DEF_FB_W - drt.w;
                    drt.y = DEF_FB_H - drt.h;
                    break;
                }
                GFX_Copy(TEX_SCR0, nds.screen.pixels[0], srt, drt, nds.screen.pitch[0], 1, rotate);
#else 
                GFX_Copy(-1, nds.screen.pixels[0], srt, drt, nds.screen.pitch[0], 1, rotate);
#endif
                break;
            }
#endif
        }

        if (nds.screen.hres_mode[idx]) {
            break;
        }
    }

    if (show_info_cnt > 0) {
        draw_info(NULL, show_info_buf, FB_W - get_font_width(show_info_buf), 0, col_fg, col_bg);
        show_info_cnt-= 1;
    }
    else if (show_fps && fps_info) {
        SDL_Rect rt = {0};

        rt.x = FB_W - fps_info->w;
        rt.y = 0;
        rt.w = fps_info->w;
        rt.h = fps_info->h;
        GFX_Copy(-1, fps_info->pixels, fps_info->clip_rect, rt, fps_info->pitch, 0, E_MI_GFX_ROTATE_180);
    }

    if (nds.screen.init) {
        //nds_set_screen_menu_off _func = (nds_set_screen_menu_off)FUN_SET_SCREEN_MENU_OFF;
        //_func();
    }
    GFX_Flip();
    return 0;
}

void sdl_blit_screen_menu(uint16_t *src, uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
}

void sdl_update_screen(void)
{
    static int prepare_time = 30;

    if (prepare_time) {
        process_screen();
        prepare_time -= 1;
    }
    else if (nds.update_screen == 0) {
        gfx.lcd.cur_sel ^= 1;
//        *((uint32_t *)VAR_SDL_SCREEN0_PIXELS) = (uintptr_t)(gfx.lcd.virAddr[gfx.lcd.cur_sel][0]);
//        *((uint32_t *)VAR_SDL_SCREEN1_PIXELS) = (uintptr_t)(gfx.lcd.virAddr[gfx.lcd.cur_sel][1]);
#if defined(A30)
        nds.menu.drastic.enable = 0;
#endif
        nds.update_screen = 1;
    }
}

void sdl_print_string(char *p, uint32_t fg, uint32_t bg, uint32_t x, uint32_t y)
{
    int w = 0, h = 0;
    SDL_Color col = {0};
    SDL_Surface *t0 = NULL;
    SDL_Surface *t1 = NULL;
    static int fps_cnt = 0;

    if (p && (strlen(p) > 0)) {
        if (drastic_menu.cnt < MAX_MENU_LINE) {
            drastic_menu.item[drastic_menu.cnt].x = x;
            drastic_menu.item[drastic_menu.cnt].y = y;
            drastic_menu.item[drastic_menu.cnt].fg = fg;
            drastic_menu.item[drastic_menu.cnt].bg = bg;
            strcpy(drastic_menu.item[drastic_menu.cnt].msg, p);
            drastic_menu.cnt+= 1;
        }
        //printf(PREFIX"x:%d, y:%d, fg:0x%x, bg:0x%x, \'%s\'\n", x, y, fg, bg, p);
    }

    if ((x == 0) && (y == 0) && (fg == 0xffff) && (bg == 0x0000)) {
        if (fps_cnt++ > 60) {
            fps_cnt = 0;

            w = strlen(p);
            for (h=w-1; h>=0; h--) {
                if (p[h] == ' ') {
                    p[h] = 0;
                    break;
                }
            }

            col.r = 0xcc;
            col.g = 0xcc;
            col.b = 0x00;
            TTF_SizeUTF8(nds.font, p, &w, &h);
            t0 = TTF_RenderUTF8_Solid(nds.font, p, col);
            if (t0) {
                t1 = SDL_CreateRGBSurface(SDL_SWSURFACE, t0->w, t0->h, 32, 0, 0, 0, 0);
                if (t1) {
                    SDL_FillRect(t1, &t1->clip_rect, 0x000000);
                    SDL_BlitSurface(t0, NULL, t1, NULL);

                    if (fps_info) {
                        SDL_FreeSurface(fps_info);
                    }
                    fps_info = SDL_ConvertSurface(t1, cvt->format, 0);
                    SDL_FreeSurface(t1);
                }
                SDL_FreeSurface(t0);
            }
            show_fps = 1;
        }
    }
}

void sdl_savestate_pre(void)
{
#ifndef UT
    asm volatile (
        "mov r1, %0                 \n"
        "mov r2, #1                 \n"
        "str r2, [r1]               \n"
        "add sp, sp, #(0x20 + 0)    \n"
        "ldr pc, [sp], #4           \n"
        ::"r"(&savestate_busy):
    );
#endif
}

void sdl_savestate_post(void)
{
#ifndef UT
    asm volatile (
        "mov r1, %0                 \n"
        "mov r2, #0                 \n"
        "str r2, [r1]               \n"
        "add sp, sp, #(0x18 + 0)    \n"
        "ldr pc, [sp], #4           \n"
        ::"r"(&savestate_busy):
    );
#endif
}

void sigterm_handler(int sig)
{
    static int ran = 0;

    if (ran == 0) {
        ran = 1;
        printf(PREFIX"Oops sigterm !\n");
        invoke_drastic_quit();
    }
}

static void strip_newline(char *p)
{
    int cc = 0, len = strlen(p);

    for(cc=0; cc<len; cc++) {
        if ((p[cc] == '\r') || (p[cc] == '\n')) {
            p[cc] = 0;
            break;
        }
    }
}

static void *video_handler(void *threadid)
{
#if defined(A30)
    EGLint egl_major = 0;
    EGLint egl_minor = 0;
    EGLint num_configs = 0;
    EGLint config_attribs[] = {
        EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE,   8,  
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE,  8,  
        EGL_ALPHA_SIZE, 8,
        EGL_NONE
    };
    EGLint window_attributes[] = { 
        EGL_RENDER_BUFFER, EGL_BACK_BUFFER,
        EGL_NONE
    };
    EGLint const context_attributes[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE,
    };
  
    vid.eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(vid.eglDisplay, &egl_major, &egl_minor);
    eglChooseConfig(vid.eglDisplay, config_attribs, &vid.eglConfig, 1, &num_configs);
    vid.eglSurface = eglCreateWindowSurface(vid.eglDisplay, vid.eglConfig, 0, window_attributes);
    vid.eglContext = eglCreateContext(vid.eglDisplay, vid.eglConfig, EGL_NO_CONTEXT, context_attributes);
    eglMakeCurrent(vid.eglDisplay, vid.eglSurface, vid.eglSurface, vid.eglContext);
  
    vid.vShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vid.vShader, 1, &vShaderSrc, NULL);
    glCompileShader(vid.vShader);
  
    vid.fShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(vid.fShader, 1, &fShaderSrc, NULL);
    glCompileShader(vid.fShader);
   
    vid.pObject = glCreateProgram();
    glAttachShader(vid.pObject, vid.vShader);
    glAttachShader(vid.pObject, vid.fShader);
    glLinkProgram(vid.pObject);
    glUseProgram(vid.pObject);

    eglSwapInterval(vid.eglDisplay, 1);
    vid.posLoc = glGetAttribLocation(vid.pObject, "a_position");
    vid.texLoc = glGetAttribLocation(vid.pObject, "a_texCoord");
    vid.samLoc = glGetUniformLocation(vid.pObject, "s_texture");
    vid.alphaLoc = glGetUniformLocation(vid.pObject, "s_alpha");

    glGenTextures(TEX_MAX, vid.texID);

    glViewport(0, 0, DEF_FB_H, DEF_FB_W);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnableVertexAttribArray(vid.posLoc);
    glEnableVertexAttribArray(vid.texLoc);
    glUniform1i(vid.samLoc, 0);
    glUniform1f(vid.alphaLoc, 0.0);

    pixel_filter = 0;
    gfx.lcd.virAddr[0][0] = malloc(SCREEN_DMA_SIZE);
    gfx.lcd.virAddr[0][1] = malloc(SCREEN_DMA_SIZE);
    gfx.lcd.virAddr[1][0] = malloc(SCREEN_DMA_SIZE);
    gfx.lcd.virAddr[1][1] = malloc(SCREEN_DMA_SIZE);
    printf(PREFIX"Ping-pong Buffer %p\n", gfx.lcd.virAddr[0][0]);
    printf(PREFIX"Ping-pong Buffer %p\n", gfx.lcd.virAddr[0][1]);
    printf(PREFIX"Ping-pong Buffer %p\n", gfx.lcd.virAddr[1][0]);
    printf(PREFIX"Ping-pong Buffer %p\n", gfx.lcd.virAddr[1][1]);
#endif

    while (is_video_thread_running) {
#if defined(A30)
        if (nds.menu.enable) {
            if (nds.update_menu) {
                nds.update_menu = 0;
                GFX_Copy(-1, cvt->pixels, cvt->clip_rect, cvt->clip_rect, cvt->pitch, 0, E_MI_GFX_ROTATE_180);
                GFX_Flip();
            }
        }
        else if (nds.menu.drastic.enable) {
            if (nds.update_menu) {
                nds.update_menu = 0;
                GFX_Copy(-1, nds.menu.drastic.main->pixels, nds.menu.drastic.main->clip_rect, nds.menu.drastic.main->clip_rect, nds.menu.drastic.main->pitch, 0, 0);
                GFX_Flip();
            }
        }
        else if (nds.update_screen) {
#else
        if (nds.update_screen) {
#endif

            process_screen();
            nds.update_screen = 0;
        }
        else {
            usleep(0);
        }
    }

#if defined(A30)
    glDeleteTextures(TEX_MAX, vid.texID);
    eglMakeCurrent(vid.eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(vid.eglDisplay, vid.eglContext);
    eglDestroySurface(vid.eglDisplay, vid.eglSurface);
    eglTerminate(vid.eglDisplay);

    free(gfx.lcd.virAddr[0][0]);
    free(gfx.lcd.virAddr[0][1]);
    free(gfx.lcd.virAddr[1][0]);
    free(gfx.lcd.virAddr[1][1]);
#endif
    pthread_exit(NULL);
}

static int lang_unload(void)
{
    int cc = 0;

    for (cc=0; translate[cc]; cc++) {
        if (translate[cc]) {
            free(translate[cc]);
        }
        translate[cc] = NULL;
    }
    memset(translate, 0, sizeof(translate));
    return 0;
}

static int lang_load(const char *lang)
{
    FILE *f = NULL;
    char buf[MAX_PATH << 1] = {0};

    if (strcasecmp(nds.lang.trans[DEF_LANG_SLOT], DEF_LANG_LANG)) {
        snprintf(buf, sizeof(buf), "%s/%s", nds.lang.path, lang);
        f = fopen(buf, "r");

        if (f != NULL) {
            int cc = 0, len = 0;

            memset(buf, 0, sizeof(buf));
            while (fgets(buf, sizeof(buf), f)) {
                strip_newline(buf);
                len = strlen(buf) + 2;
                if (len == 0) {
                    continue;
                }

                if (translate[cc] != NULL) {
                    free(translate[cc]);
                }
                translate[cc] = malloc(len);
                if (translate[cc] != NULL) {
                    memcpy(translate[cc], buf, len);
                    //printf(PREFIX"Translate: \'%s\'(len=%d)\n", translate[cc], len);
                }
                cc+= 1;
                if (cc >= MAX_LANG_LINE) {
                    break;
                }
                memset(buf, 0, sizeof(buf));
            }
            fclose(f);
        }
        else {
            printf(PREFIX"Failed to open lang folder \'%s\'\n", nds.lang.path);
        }
    }
    return 0;
}

static void lang_enum(void)
{
    int idx = 2;
    DIR *d = NULL;
    struct dirent *dir = NULL;

    strcpy(nds.lang.trans[DEF_LANG_SLOT], DEF_LANG_LANG);
    strcpy(nds.lang.trans[DEF_LANG_SLOT + 1], DEF_LANG_LANG);
    d = opendir(nds.lang.path);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_type == DT_DIR) {
                continue;
            }
            if (strcmp(dir->d_name, ".") == 0) {
                continue;
            }
            if (strcmp(dir->d_name, "..") == 0) {
                continue;
            }

            //printf(PREFIX"found lang \'lang[%d]=%s\'\n", idx, dir->d_name);
            strcpy(nds.lang.trans[idx], dir->d_name);
            idx+= 1;
            if (idx >= MAX_LANG_FILE) {
                break;
            }
        }
        closedir(d);
    }
}

static int read_config(void)
{
    struct json_object *jval = NULL;
    struct json_object *jfile = NULL;

    jfile = json_object_from_file(nds.cfg.path);
    if (jfile == NULL) {
        printf(PREFIX"Failed to read settings from json file (%s)\n", nds.cfg.path);
        return -1;
    }

    json_object_object_get_ex(jfile, JSON_NDS_PEN_SEL, &jval);
    if (jval) {
        nds.pen.sel = json_object_get_int(jval);
    }

    json_object_object_get_ex(jfile, JSON_NDS_PEN_POS, &jval);
    if (jval) {
        nds.pen.pos = json_object_get_int(jval) == 0 ? 0 : 1;
    }

    json_object_object_get_ex(jfile, JSON_NDS_THEME_SEL, &jval);
    if (jval) {
        nds.theme.sel = json_object_get_int(jval);
    }

    json_object_object_get_ex(jfile, JSON_NDS_DIS_MODE, &jval);
    if (jval) {
        nds.dis_mode = json_object_get_int(jval);
    }
    
    json_object_object_get_ex(jfile, JSON_NDS_ALT_MODE, &jval);
    if (jval) {
        nds.alt_mode = json_object_get_int(jval);
    }
    
    json_object_object_get_ex(jfile, JSON_NDS_PEN_XV, &jval);
    if (jval) {
        nds.pen.xv = json_object_get_int(jval);
    }
    
    json_object_object_get_ex(jfile, JSON_NDS_PEN_YV, &jval);
    if (jval) {
        nds.pen.yv = json_object_get_int(jval);
    }

    json_object_object_get_ex(jfile, JSON_NDS_ALPHA_VALUE, &jval);
    if (jval) {
        nds.alpha.val = json_object_get_int(jval);
    }

    json_object_object_get_ex(jfile, JSON_NDS_ALPHA_POSITION, &jval);
    if (jval) {
        nds.alpha.pos = json_object_get_int(jval);
    }

    json_object_object_get_ex(jfile, JSON_NDS_ALPHA_BORDER, &jval);
    if (jval) {
        nds.alpha.border = json_object_get_int(jval);
        nds.alpha.border%= NDS_BORDER_MAX;
    }

    json_object_object_get_ex(jfile, JSON_NDS_MAX_CPU, &jval);
    if (jval) {
        nds.maxcpu = json_object_get_int(jval);
    }

    json_object_object_get_ex(jfile, JSON_NDS_MIN_CPU, &jval);
    if (jval) {
        nds.mincpu = json_object_get_int(jval);
    }

    json_object_object_get_ex(jfile, JSON_NDS_MAX_CORE, &jval);
    if (jval) {
        nds.maxcore = json_object_get_int(jval);
    }

    json_object_object_get_ex(jfile, JSON_NDS_MIN_CORE, &jval);
    if (jval) {
        nds.mincore = json_object_get_int(jval);
    }

    json_object_object_get_ex(jfile, JSON_NDS_OVERLAY, &jval);
    if (jval) {
        nds.overlay.sel = json_object_get_int(jval);
    }

    json_object_object_get_ex(jfile, JSON_NDS_SWAP_L1L2, &jval);
    if (jval) {
        nds.swap_l1l2 = json_object_get_int(jval) ? 1 : 0;
    }

    json_object_object_get_ex(jfile, JSON_NDS_SWAP_R1R2, &jval);
    if (jval) {
        nds.swap_r1r2 = json_object_get_int(jval) ? 1 : 0;
    }

    json_object_object_get_ex(jfile, JSON_NDS_KEYS_ROTATE, &jval);
    if (jval) {
        nds.keys_rotate = json_object_get_int(jval) % 3;
    }

    json_object_object_get_ex(jfile, JSON_NDS_CHK_BAT, &jval);
    if (jval) {
        nds.chk_bat = json_object_get_int(jval) ? 1: 0;
    }

#ifdef A30
    json_object_object_get_ex(jfile, JSON_NDS_JOY_MODE, &jval);
    if (jval) {
        nds.joy.mode = json_object_get_int(jval);
    }

    json_object_object_get_ex(jfile, JSON_NDS_JOY_DZONE, &jval);
    if (jval) {
        nds.joy.dzone = json_object_get_int(jval);
    }

    json_object_object_get_ex(jfile, JSON_NDS_JOY_CUSKEY0, &jval);
    if (jval) {
        nds.joy.cuskey[0] = json_object_get_int(jval);
    }

    json_object_object_get_ex(jfile, JSON_NDS_JOY_CUSKEY1, &jval);
    if (jval) {
        nds.joy.cuskey[1] = json_object_get_int(jval);
    }

    json_object_object_get_ex(jfile, JSON_NDS_JOY_CUSKEY2, &jval);
    if (jval) {
        nds.joy.cuskey[2] = json_object_get_int(jval);
    }

    json_object_object_get_ex(jfile, JSON_NDS_JOY_CUSKEY3, &jval);
    if (jval) {
        nds.joy.cuskey[3] = json_object_get_int(jval);
    }
#endif

    nds.menu.c0 = 0xffffff;
    json_object_object_get_ex(jfile, JSON_NDS_MENU_C0, &jval);
    if (jval) {
        const char *p = json_object_get_string(jval);
        nds.menu.c0 = strtol(p, NULL, 16);
    }

    nds.menu.c1 = 0x000000;
    json_object_object_get_ex(jfile, JSON_NDS_MENU_C1, &jval);
    if (jval) {
        const char *p = json_object_get_string(jval);
        nds.menu.c1 = strtol(p, NULL, 16);
    }

    nds.menu.c2 = 0x289a35;
    json_object_object_get_ex(jfile, JSON_NDS_MENU_C2, &jval);
    if (jval) {
        const char *p = json_object_get_string(jval);
        nds.menu.c2 = strtol(p, NULL, 16);
    }

    nds.auto_state = 1;
    json_object_object_get_ex(jfile, JSON_NDS_AUTO_STATE, &jval);
    if (jval) {
        nds.auto_state = json_object_get_int(jval) ? 1 : 0;
    }

    nds.auto_slot = 1;
    json_object_object_get_ex(jfile, JSON_NDS_AUTO_SLOT, &jval);
    if (jval) {
        nds.auto_slot = json_object_get_int(jval);
    }

    lang_enum();
    json_object_object_get_ex(jfile, JSON_NDS_LANG, &jval);
    if (jval) {
        const char *lang = json_object_get_string(jval);

        strcpy(nds.lang.trans[DEF_LANG_SLOT], lang);
        lang_load(lang);
    }

    json_object_object_get_ex(jfile, JSON_NDS_HOTKEY, &jval);
    if (jval) {
        nds.hotkey = json_object_get_int(jval);
    }

    nds.menu.show_cursor = 0;
    json_object_object_get_ex(jfile, JSON_NDS_MENU_CURSOR, &jval);
    if (jval) {
        nds.menu.show_cursor = json_object_get_int(jval);
    }

    nds.fast_forward = 6;
    json_object_object_get_ex(jfile, JSON_NDS_FAST_FORWARD, &jval);
    if (jval) {
        nds.fast_forward = json_object_get_int(jval);
    }

#if defined(MINI) || defined(A30)
    json_object_object_get_ex(jfile, JSON_NDS_STATES, &jval);
    if (jval) {
        struct stat st = {0};
        const char *path = json_object_get_string(jval);

        if ((path != NULL) && (path[0] != 0)) {
            strcpy(nds.states.path, path);

            if (stat(nds.states.path, &st) == -1) {
                mkdir(nds.states.path, 0755);
                printf(PREFIX"Create states folder in \'%s\'\n", nds.states.path);
            }
        }
    }
#endif

    if (nds.dis_mode > NDS_LAYOUT_LAST) {
        nds.dis_mode = NDS_LAYOUT_8;
        nds.alt_mode = NDS_LAYOUT_2;
    }

    nds.menu.sel = 0;
    json_object_object_get_ex(jfile, JSON_NDS_MENU_BG, &jval);
    if (jval) {
        nds.menu.sel = json_object_get_int(jval);
        if (nds.menu.sel >= nds.menu.max) {
            nds.menu.sel = 0;
        }
    }
    reload_menu();

    reload_pen();
#if defined(MINI) || defined(A30)
    reload_overlay();
#endif
    json_object_put(jfile);

#ifdef A30
    nds.joy.max_x = 200;
    nds.joy.zero_x = 135;
    nds.joy.min_x = 75;
    nds.joy.max_y = 200;
    nds.joy.zero_y = 135;
    nds.joy.min_y = 75;
#endif
    return 0;
}

static int write_config(void)
{
    struct json_object *jfile = NULL;

    jfile = json_object_from_file(nds.cfg.path);
    if (jfile == NULL) {
        printf(PREFIX"Failed to write settings to json file (%s)\n", nds.cfg.path);
        return -1;
    }

    if (nds.dis_mode > NDS_LAYOUT_LAST) {
        nds.dis_mode = NDS_LAYOUT_8;
        nds.alt_mode = NDS_LAYOUT_2;
    }

    json_object_object_add(jfile, JSON_NDS_PEN_SEL, json_object_new_int(nds.pen.sel));
    json_object_object_add(jfile, JSON_NDS_THEME_SEL, json_object_new_int(nds.theme.sel));
    json_object_object_add(jfile, JSON_NDS_DIS_MODE, json_object_new_int(nds.dis_mode));
    json_object_object_add(jfile, JSON_NDS_ALPHA_VALUE, json_object_new_int(nds.alpha.val));
    json_object_object_add(jfile, JSON_NDS_ALPHA_POSITION, json_object_new_int(nds.alpha.pos));
    json_object_object_add(jfile, JSON_NDS_ALPHA_BORDER, json_object_new_int(nds.alpha.border));
    json_object_object_add(jfile, JSON_NDS_OVERLAY, json_object_new_int(nds.overlay.sel));
    json_object_object_add(jfile, JSON_NDS_ALT_MODE, json_object_new_int(nds.alt_mode));
    json_object_object_add(jfile, JSON_NDS_KEYS_ROTATE, json_object_new_int(nds.keys_rotate));
    json_object_object_add(jfile, JSON_NDS_LANG, json_object_new_string(nds.lang.trans[DEF_LANG_SLOT]));
    json_object_object_add(jfile, JSON_NDS_HOTKEY, json_object_new_int(nds.hotkey));
    json_object_object_add(jfile, JSON_NDS_SWAP_L1L2, json_object_new_int(nds.swap_l1l2));
    json_object_object_add(jfile, JSON_NDS_SWAP_R1R2, json_object_new_int(nds.swap_r1r2));
    json_object_object_add(jfile, JSON_NDS_PEN_XV, json_object_new_int(nds.pen.xv));
    json_object_object_add(jfile, JSON_NDS_PEN_YV, json_object_new_int(nds.pen.yv));
    json_object_object_add(jfile, JSON_NDS_MENU_BG, json_object_new_int(nds.menu.sel));
    json_object_object_add(jfile, JSON_NDS_MENU_CURSOR, json_object_new_int(nds.menu.show_cursor));
    json_object_object_add(jfile, JSON_NDS_FAST_FORWARD, json_object_new_int(nds.fast_forward));
    json_object_object_add(jfile, JSON_NDS_CHK_BAT, json_object_new_int(nds.chk_bat));

#ifdef A30
    json_object_object_add(jfile, JSON_NDS_JOY_MODE, json_object_new_int(nds.joy.mode));
    json_object_object_add(jfile, JSON_NDS_JOY_DZONE, json_object_new_int(nds.joy.dzone));
    json_object_object_add(jfile, JSON_NDS_JOY_CUSKEY0, json_object_new_int(nds.joy.cuskey[0]));
    json_object_object_add(jfile, JSON_NDS_JOY_CUSKEY1, json_object_new_int(nds.joy.cuskey[1]));
    json_object_object_add(jfile, JSON_NDS_JOY_CUSKEY2, json_object_new_int(nds.joy.cuskey[2]));
    json_object_object_add(jfile, JSON_NDS_JOY_CUSKEY3, json_object_new_int(nds.joy.cuskey[3]));
#endif
    json_object_object_add(jfile, JSON_NDS_CHK_BAT, json_object_new_int(nds.chk_bat));

    json_object_to_file_ext(nds.cfg.path, jfile, JSON_C_TO_STRING_PRETTY);
    json_object_put(jfile);
    printf(PREFIX"Wrote changed settings back !\n");
    return 0;
}

#ifdef MINI
static int get_cpuclock(void)
{
    static const uint64_t divsrc = 432000000llu * 524288;

    int fd_mem = -1;
    void *pll_map = NULL;
    uint32_t rate = 0;
    uint32_t lpf_value = 0;
    uint32_t post_div = 0;

    fd_mem = open("/dev/mem", O_RDWR);
    if (fd_mem < 0) {
        return 0;
    }

    pll_map = mmap(0, PLL_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd_mem, BASE_REG_MPLL_PA);
    if (pll_map) {
        volatile uint8_t* p8 = (uint8_t*)pll_map;
        volatile uint16_t* p16 = (uint16_t*)pll_map;

        lpf_value = p16[0x2a4] + (p16[0x2a6] << 16);
        post_div = p16[0x232] + 1;
        if (lpf_value == 0) {
            lpf_value = (p8[0x2c2 << 1] << 16) + (p8[0x2c1 << 1] << 8) + p8[0x2c0 << 1];
        }

        if (lpf_value && post_div) {
            rate = (divsrc / lpf_value * 2 / post_div * 16);
        }
        printf(PREFIX"Current cpuclock=%u (lpf=%u, post_div=%u)\n", rate, lpf_value, post_div);
        munmap(pll_map, PLL_SIZE);
    }
    close(fd_mem);
    return rate / 1000000;
}

static int set_cpuclock(uint32_t newclock)
{
    int fd_mem = -1;
    void *pll_map = NULL;
    uint32_t post_div = 0;
    char clockstr[16] = {0};
    const char fn_governor[] = "/sys/devices/system/cpu/cpufreq/policy0/scaling_governor";
    const char fn_setspeed[] = "/sys/devices/system/cpu/cpufreq/policy0/scaling_setspeed";

    if ((newclock < nds.mincpu) || (newclock > nds.maxcpu)) {
        return -1;
    }

    fd_mem = open("/dev/mem", O_RDWR);
    if (fd_mem < 0) {
        return -1;
    }

    pll_map = mmap(0, PLL_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd_mem, BASE_REG_MPLL_PA);
    if (pll_map) {
        printf(PREFIX"Set CPU %dMHz\n", newclock);

        newclock*= 1000;
        snprintf(clockstr, sizeof(clockstr), "%d", newclock);
        write_file(fn_governor, "userspace", strlen("userspace"));
        write_file(fn_setspeed, clockstr, strlen(clockstr));

        if (newclock >= 800000) {
            post_div = 2;
        }
        else if (newclock >= 400000) {
            post_div = 4;
        }
        else if (newclock >= 200000) {
            post_div = 8;
        }
        else {
            post_div = 16;
        }

        if (1) {
            static const uint64_t divsrc = 432000000llu * 524288;
            uint32_t rate = (newclock * 1000) / 16 * post_div / 2;
            uint32_t lpf = (uint32_t)(divsrc / rate);
            volatile uint16_t* p16 = (uint16_t*)pll_map;
            uint32_t cur_post_div = (p16[0x232] & 0x0f) + 1;
            uint32_t tmp_post_div = cur_post_div;

            if (post_div > cur_post_div) {
                while (tmp_post_div != post_div) {
                    tmp_post_div <<= 1;
                    p16[0x232] = (p16[0x232] & 0xf0) | ((tmp_post_div - 1) & 0x0f);
                }
            }

            p16[0x2A8] = 0x0000;        // reg_lpf_enable = 0
            p16[0x2AE] = 0x000f;        // reg_lpf_update_cnt = 32
            p16[0x2A4] = lpf & 0xffff;  // set target freq to LPF high
            p16[0x2A6] = lpf >> 16;     // set target freq to LPF high
            p16[0x2B0] = 0x0001;        // switch to LPF control
            p16[0x2B2]|= 0x1000;        // from low to high
            p16[0x2A8] = 0x0001;        // reg_lpf_enable = 1
            while(!(p16[0x2ba] & 1));   // polling done
            p16[0x2A0] = lpf & 0xffff;  // store freq to LPF low
            p16[0x2A2] = lpf >> 16;     // store freq to LPF low

            if (post_div < cur_post_div) {
                while (tmp_post_div != post_div) {
                    tmp_post_div >>= 1;
                    p16[0x232] = (p16[0x232] & 0xf0) | ((tmp_post_div - 1) & 0x0f);
                }
            }
        }
        munmap(pll_map, PLL_SIZE);
    }
    close(fd_mem);
    return 0;
}
#endif

int get_dir_path(const char *path, int desire, char *buf)
{
    DIR *d = NULL;
    int count = 0, r = -1;
    struct dirent *dir = NULL;

    d = opendir(path);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (strcmp(dir->d_name, ".") == 0) {
                continue;
            }

            if (strcmp(dir->d_name, "..") == 0) {
                continue;
            }

            if (dir->d_type != DT_DIR) {
                continue;
            }

            if (count == desire) {
                r = snprintf(buf, MAX_PATH, "%s/%s", path, dir->d_name) ? 0 : 1;
                break;
            }
            count+= 1;
        }
        closedir(d);
    }
    return r;
}

static int get_file_path(const char *path, int desire, char *buf, int add_path)
{
    DIR *d = NULL;
    int count = 0, r = -1;
    struct dirent *dir = NULL;

    d = opendir(path);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (strcmp(dir->d_name, ".") == 0) {
                continue;
            }

            if (strcmp(dir->d_name, "..") == 0) {
                continue;
            }

            if (dir->d_type == DT_DIR) {
                continue;
            }

            if (count == desire) {
                if (add_path) {
                    r = snprintf(buf, MAX_PATH, "%s/%s", path, dir->d_name) ? 0 : 1;
                }
                else {
                    r = snprintf(buf, MAX_PATH, "%s", dir->d_name) ? 0 : 1;
                }
                break;
            }
            count+= 1;
        }
        closedir(d);
    }
    return r;
}

static int get_dir_count(const char *path)
{
    DIR *d = NULL;
    int count = 0;
    struct dirent *dir = NULL;

    d = opendir(path);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_type != DT_DIR) {
                continue;
            }
            if (strcmp(dir->d_name, ".") == 0) {
                continue;
            }
            if (strcmp(dir->d_name, "..") == 0) {
                continue;
            }
            count+= 1;
        }
        closedir(d);
    }
    return count;
}

static int get_file_count(const char *path)
{
    DIR *d = NULL;
    int count = 0;
    struct dirent *dir = NULL;

    d = opendir(path);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_type == DT_DIR) {
                continue;
            }
            if (strcmp(dir->d_name, ".") == 0) {
                continue;
            }
            if (strcmp(dir->d_name, "..") == 0) {
                continue;
            }
            count+= 1;
        }
        closedir(d);
    }
    return count;
}

static int get_theme_count(void)
{
    return get_dir_count(nds.theme.path);
}

static int get_menu_count(void)
{
    return get_dir_count(MENU_PATH);
}

static int get_pen_count(void)
{
    return get_file_count(nds.pen.path);
}

static int get_overlay_count(void)
{
    return get_file_count(nds.overlay.path);
}

#ifdef UT
int fb_init(void)
{
    return 0;
}

int fb_quit(void)
{
    return 0;
}
#endif

#ifdef A30
static int get_core(int index)
{
    FILE *fd = NULL;
    char buf[255] = {0};

    snprintf(buf, sizeof(buf), "cat /sys/devices/system/cpu/cpu%d/online", index % 4); 
    fd = popen(buf, "r");
    if (fd == NULL) {
        return -1;
    }       
    fgets(buf, sizeof(buf), fd);
    pclose(fd);
    return atoi(buf);
}

static void check_before_set(int num, int v)
{
    char buf[255] = {0};

    if (get_core(num) != v) {
        snprintf(buf, sizeof(buf), "echo %d > /sys/devices/system/cpu/cpu%d/online", v ? 1 : 0, num);
        system(buf);
    }       
}   

static void set_core(int n)
{           
    if (n <= 1) {
        printf(PREFIX"New CPU Core: 1\n");
        check_before_set(0, 1);
        check_before_set(1, 0);
        check_before_set(2, 0);
        check_before_set(3, 0);
    }       
    else if (n == 2) {
        printf(PREFIX"New CPU Core: 2\n");
        check_before_set(0, 1);
        check_before_set(1, 1);
        check_before_set(2, 0);
        check_before_set(3, 0);
    }       
    else if (n == 3) {
        printf(PREFIX"New CPU Core: 3\n");
        check_before_set(0, 1);
        check_before_set(1, 1);
        check_before_set(2, 1);
        check_before_set(3, 0);
    }
    else {
        printf(PREFIX"New CPU Core: 4\n");
        check_before_set(0, 1);
        check_before_set(1, 1);
        check_before_set(2, 1);
        check_before_set(3, 1);
    }
}

static int set_best_match_cpu_clock(int clk)
{
    int cc = 0;

    system("echo performance > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
    for (cc = 0; cc < max_cpu_item; cc++) {
        if (cpu_clock[cc].clk >= clk) {
            printf(PREFIX"Set Best Match CPU %dMHz (0x%08x)\n", cpu_clock[cc].clk, cpu_clock[cc].reg);
            *vid.cpu_ptr = cpu_clock[cc].reg;
            return cc;
        }
    }
    return -1;
}

int fb_init(void)
{
    gfx.fb_dev = open("/dev/fb0", O_RDWR, 0);
    if (gfx.fb_dev < 0) {
        printf(PREFIX"Failed to open /dev/fb0\n");
        return -1;
    }

    if (ioctl(gfx.fb_dev, FBIOGET_VSCREENINFO, &gfx.vinfo) < 0) {
        printf(PREFIX"Failed to get fb info\n");
        return -1;
    }

    gfx.fb.virAddr = mmap(NULL, FB_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, gfx.fb_dev, 0);
    if (gfx.fb.virAddr == (void *)-1) {
        close(gfx.fb_dev);
        gfx.fb_dev = -1;
        printf(PREFIX"Failed to mmap fb\n");
        return -1;
    }
    printf(PREFIX"FB virAddr %p (size:%d)\n", gfx.fb.virAddr, FB_SIZE);
    memset(gfx.fb.virAddr, 0 , FB_SIZE);

    gfx.vinfo.yres_virtual = gfx.vinfo.yres * 2;
    ioctl(gfx.fb_dev, FBIOPUT_VSCREENINFO, &gfx.vinfo);

    vid.mem_fd = open("/dev/mem", O_RDWR);
    if (vid.mem_fd < 0) { 
        printf("Failed to open /dev/mem\n");
        return -1;
    }    
    vid.ccu_mem = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, vid.mem_fd, CCU_BASE);
    if (vid.ccu_mem == MAP_FAILED) {
        printf("Failed to map memory\n");
        return -1;
    }    
    printf(PREFIX"CCU MMap %p\n", vid.ccu_mem);
    vid.cpu_ptr = (uint32_t *)&vid.ccu_mem[0x00];

    vid.dac_mem = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, vid.mem_fd, DAC_BASE);
    if (vid.dac_mem == MAP_FAILED) {
        printf("Failed to map memory\n");
        return -1;
    }    
    printf(PREFIX"DAC MMap %p\n", vid.dac_mem);
    vid.vol_ptr = (uint32_t *)(&vid.dac_mem[0xc00 + 0x258]);

    set_best_match_cpu_clock(INIT_CPU_CLOCK);
    set_core(INIT_CPU_CORE);
    return 0;
}

int fb_quit(void)
{
    set_best_match_cpu_clock(DEINIT_CPU_CLOCK);
    set_core(DEINIT_CPU_CORE);
    system("echo ondemand > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");

    if (gfx.fb.virAddr) {
        munmap(gfx.fb.virAddr, FB_SIZE);
        gfx.fb.virAddr = NULL;
    }

    if (gfx.fb_dev > 0) {
        close(gfx.fb_dev);
        gfx.fb_dev = -1;
    }

    if (vid.ccu_mem != MAP_FAILED) {
        munmap(vid.ccu_mem, 4096);
        vid.ccu_mem = NULL;
    }

    if (vid.dac_mem != MAP_FAILED) {
        munmap(vid.dac_mem, 4096);
        vid.dac_mem = NULL;
    }

    if (vid.mem_fd > 0) {
        close(vid.mem_fd);
        vid.mem_fd = -1;
    }
    return 0;
}
#endif

#ifdef MINI
int fb_init(void)
{
#if USE_MASK
    int c0 = 0;
    int c1 = 0;
    uint32_t *p = NULL;
#endif

    MI_SYS_Init();
    MI_GFX_Open();

    gfx.fb_dev = open("/dev/fb0", O_RDWR);
    if (gfx.fb_dev < 0) {
        printf(PREFIX"Failed to open /dev/fb0\n");
        return -1;
    }
    ioctl(gfx.fb_dev, FBIOGET_FSCREENINFO, &gfx.finfo);
    ioctl(gfx.fb_dev, FBIOGET_VSCREENINFO, &gfx.vinfo);
    gfx.vinfo.yoffset = 0;
    gfx.vinfo.yres_virtual = gfx.vinfo.yres * 2;
    ioctl(gfx.fb_dev, FBIOPUT_VSCREENINFO, &gfx.vinfo);

    gfx.fb.phyAddr = gfx.finfo.smem_start;
    MI_SYS_MemsetPa(gfx.fb.phyAddr, 0, FB_SIZE);
    MI_SYS_Mmap(gfx.fb.phyAddr, gfx.finfo.smem_len, &gfx.fb.virAddr, TRUE);
    memset(&gfx.hw.opt, 0, sizeof(gfx.hw.opt));

    MI_SYS_MMA_Alloc(NULL, TMP_SIZE, &gfx.tmp.phyAddr);
    MI_SYS_Mmap(gfx.tmp.phyAddr, TMP_SIZE, &gfx.tmp.virAddr, TRUE);

    MI_SYS_MMA_Alloc(NULL, TMP_SIZE, &gfx.overlay.phyAddr);
    MI_SYS_Mmap(gfx.overlay.phyAddr, TMP_SIZE, &gfx.overlay.virAddr, TRUE);

    MI_SYS_MMA_Alloc(NULL, SCREEN_DMA_SIZE, &gfx.lcd.phyAddr[0][0]);
    MI_SYS_MMA_Alloc(NULL, SCREEN_DMA_SIZE, &gfx.lcd.phyAddr[0][1]);
    MI_SYS_MMA_Alloc(NULL, SCREEN_DMA_SIZE, &gfx.lcd.phyAddr[1][0]);
    MI_SYS_MMA_Alloc(NULL, SCREEN_DMA_SIZE, &gfx.lcd.phyAddr[1][1]);
    MI_SYS_Mmap(gfx.lcd.phyAddr[0][0], SCREEN_DMA_SIZE, &gfx.lcd.virAddr[0][0], TRUE);
    MI_SYS_Mmap(gfx.lcd.phyAddr[0][1], SCREEN_DMA_SIZE, &gfx.lcd.virAddr[0][1], TRUE);
    MI_SYS_Mmap(gfx.lcd.phyAddr[1][0], SCREEN_DMA_SIZE, &gfx.lcd.virAddr[1][0], TRUE);
    MI_SYS_Mmap(gfx.lcd.phyAddr[1][1], SCREEN_DMA_SIZE, &gfx.lcd.virAddr[1][1], TRUE);
    printf(PREFIX"Ping-pong Buffer %p\n", gfx.lcd.virAddr[0][0]);
    printf(PREFIX"Ping-pong Buffer %p\n", gfx.lcd.virAddr[0][1]);
    printf(PREFIX"Ping-pong Buffer %p\n", gfx.lcd.virAddr[1][0]);
    printf(PREFIX"Ping-pong Buffer %p\n", gfx.lcd.virAddr[1][1]);

#if USE_MASK
    MI_SYS_MMA_Alloc(NULL, MASK_SIZE, &gfx.mask.phyAddr[0]);
    MI_SYS_MMA_Alloc(NULL, MASK_SIZE, &gfx.mask.phyAddr[1]);
    MI_SYS_Mmap(gfx.mask.phyAddr[0], MASK_SIZE, &gfx.mask.virAddr[0], TRUE);
    MI_SYS_Mmap(gfx.mask.phyAddr[1], MASK_SIZE, &gfx.mask.virAddr[1], TRUE);

    p = (uint32_t *)gfx.mask.virAddr[0];
    for (c0 = 0; c0 < NDS_Hx3; c0++) {
        for (c1 = 0; c1 < NDS_Wx3; c1++) {
            if (((c0 % 3) == 0) || (c1 % 3) == 0) {
                *p++ = 0xff000000;
            }
            else {
                *p++ = 0x00000000;
            }
        }
    }
    MI_SYS_FlushInvCache(gfx.mask.virAddr[0], MASK_SIZE);
#endif

    vid.sar_fd = open("/dev/sar", O_RDWR);
    return 0;
}

int fb_quit(void)
{
    MI_SYS_Munmap(gfx.fb.virAddr, TMP_SIZE);

    MI_SYS_Munmap(gfx.tmp.virAddr, TMP_SIZE);
    MI_SYS_MMA_Free(gfx.tmp.phyAddr);

    MI_SYS_Munmap(gfx.overlay.virAddr, TMP_SIZE);
    MI_SYS_MMA_Free(gfx.overlay.phyAddr);

#if USE_MASK
    MI_SYS_Munmap(gfx.mask.virAddr[0], MASK_SIZE);
    MI_SYS_Munmap(gfx.mask.virAddr[1], MASK_SIZE);
    MI_SYS_MMA_Free(gfx.mask.phyAddr[0]);
    MI_SYS_MMA_Free(gfx.mask.phyAddr[1]);
#endif

    MI_SYS_Munmap(gfx.lcd.virAddr[0][0], SCREEN_DMA_SIZE);
    MI_SYS_Munmap(gfx.lcd.virAddr[0][1], SCREEN_DMA_SIZE);
    MI_SYS_Munmap(gfx.lcd.virAddr[1][0], SCREEN_DMA_SIZE);
    MI_SYS_Munmap(gfx.lcd.virAddr[1][1], SCREEN_DMA_SIZE);
    MI_SYS_MMA_Free(gfx.lcd.phyAddr[0][0]);
    MI_SYS_MMA_Free(gfx.lcd.phyAddr[0][1]);
    MI_SYS_MMA_Free(gfx.lcd.phyAddr[1][0]);
    MI_SYS_MMA_Free(gfx.lcd.phyAddr[1][1]);

    MI_GFX_Close();
    MI_SYS_Exit();

    gfx.vinfo.yoffset = 0;
    ioctl(gfx.fb_dev, FBIOPUT_VSCREENINFO, &gfx.vinfo);
    close(gfx.fb_dev);
    gfx.fb_dev = -1;

    close(vid.sar_fd);
    vid.sar_fd = -1;
    return 0;
}
#endif

void GFX_Init(void)
{
    struct stat st = {0};
    char buf[MAX_PATH << 1] = {0};

    fb_init();
    memset(nds.pen.path, 0, sizeof(nds.pen.path));
    if (getcwd(nds.pen.path, sizeof(nds.pen.path))) {
        strcat(nds.pen.path, "/");
        strcat(nds.pen.path, PEN_PATH);
    }

    strcpy(nds.shot.path, SHOT_PATH);
    if (stat(nds.shot.path, &st) == -1) {
        mkdir(nds.shot.path, 0755);
    }

    memset(nds.theme.path, 0, sizeof(nds.theme.path));
    if (getcwd(nds.theme.path, sizeof(nds.theme.path))) {
        strcat(nds.theme.path, "/");
        strcat(nds.theme.path, THEME_PATH);
        strcat(nds.theme.path, nds.enable_752x560 ? "_752" : "_640");
    }

    memset(nds.overlay.path, 0, sizeof(nds.overlay.path));
    if (getcwd(nds.overlay.path, sizeof(nds.overlay.path))) {
        strcat(nds.overlay.path, "/");
        strcat(nds.overlay.path, OVERLAY_PATH);
    }

    memset(nds.lang.path, 0, sizeof(nds.lang.path));
    if (getcwd(nds.lang.path, sizeof(nds.lang.path))) {
        strcat(nds.lang.path, "/");
        strcat(nds.lang.path, LANG_PATH);
    }

    memset(nds.menu.path, 0, sizeof(nds.menu.path));
    if (getcwd(nds.menu.path, sizeof(nds.menu.path))) {
        strcat(nds.menu.path, "/");
        strcat(nds.menu.path, MENU_PATH);
    }

    memset(nds.bios.path, 0, sizeof(nds.bios.path));
    if (getcwd(nds.bios.path, sizeof(nds.bios.path))) {
        strcat(nds.bios.path, "/");
        strcat(nds.bios.path, BIOS_PATH);
//
//        snprintf(buf, sizeof(buf), "%s/drastic_bios_arm7.bin", nds.bios.path);
//        write_file(buf, drastic_bios_arm7, sizeof(drastic_bios_arm7));
//
//        snprintf(buf, sizeof(buf), "%s/drastic_bios_arm9.bin", nds.bios.path);
//        write_file(buf, drastic_bios_arm9, sizeof(drastic_bios_arm9));
//
//        snprintf(buf, sizeof(buf), "%s/nds_bios_arm7.bin", nds.bios.path);
//        write_file(buf, nds_bios_arm7, sizeof(nds_bios_arm7));
//
//        snprintf(buf, sizeof(buf), "%s/nds_bios_arm9.bin", nds.bios.path);
//        write_file(buf, nds_bios_arm9, sizeof(nds_bios_arm9));
//
//        snprintf(buf, sizeof(buf), "%s/nds_firmware.bin", nds.bios.path);
//        write_file(buf, nds_firmware, sizeof(nds_firmware));
    }

    cvt = SDL_CreateRGBSurface(SDL_SWSURFACE, FB_W, FB_H, 32, 0, 0, 0, 0);

    nds.pen.sel = 0;
    nds.pen.max = get_pen_count();

    nds.theme.sel = 0;
    nds.theme.max = get_theme_count();

    nds.overlay.sel = nds.overlay.max = get_overlay_count();

    nds.menu.sel = 0;
    nds.menu.max = get_menu_count();

    nds.menu.drastic.main = SDL_CreateRGBSurface(SDL_SWSURFACE, FB_W, FB_H, 32, 0, 0, 0, 0);
    if (nds.menu.drastic.main) {
        if (nds.menu.drastic.bg0) {
            SDL_SoftStretch(nds.menu.drastic.bg0, NULL, nds.menu.drastic.main, NULL);
        }
    }

    if (NULL == getcwd(nds.cfg.path, sizeof(nds.cfg.path))) {
        printf("Failed to get current directory\n");
    }
    strcat(nds.cfg.path, "/");
    strcat(nds.cfg.path, CFG_PATH);

    TTF_Init();
    nds.font = TTF_OpenFont(FONT_PATH, FONT_SIZE);
    if (nds.enable_752x560) {
        //TTF_SetFontStyle(nds.font, TTF_STYLE_BOLD);
    }

    is_video_thread_running = 1;
    pthread_create(&thread, NULL, video_handler, (void *)NULL);
}

void GFX_Quit(void)
{
    void *ret = NULL;

    printf(PREFIX"Wait for video_handler exit\n");
    is_video_thread_running = 0;
    pthread_join(thread, &ret);

    GFX_Clear();
    printf(PREFIX"Free FB resources\n");
    fb_quit();

    if (cvt) {
        SDL_FreeSurface(cvt);
        cvt = NULL;
    }
}

void GFX_Clear(void)
{
#ifdef MINI
    MI_SYS_MemsetPa(gfx.fb.phyAddr, 0, FB_SIZE);
    MI_SYS_MemsetPa(gfx.tmp.phyAddr, 0, TMP_SIZE);
    MI_SYS_MemsetPa(gfx.lcd.phyAddr[0][0], 0, SCREEN_DMA_SIZE);
    MI_SYS_MemsetPa(gfx.lcd.phyAddr[0][1], 0, SCREEN_DMA_SIZE);
    MI_SYS_MemsetPa(gfx.lcd.phyAddr[1][0], 0, SCREEN_DMA_SIZE);
    MI_SYS_MemsetPa(gfx.lcd.phyAddr[1][1], 0, SCREEN_DMA_SIZE);
#endif
}

int draw_pen(void *pixels, int width, int pitch)
{
#ifndef UT
    int c0 = 0;
    int c1 = 0;
    int w = 28;
    int h = 28;
    int sub = 0;
    int sw = NDS_W;
    int sh = NDS_H;
    int x0 = 0, y0 = 0;
    int x1 = 0, y1 = 0;
    int x = 0, y = 0, is_565 = 0, scale = 1;
    uint32_t r = 0, g = 0, b = 0;
    uint32_t *s = hex_pen;
    uint16_t *d_565 = (uint16_t*)pixels;
    uint32_t *d_888 = (uint32_t*)pixels;

    if ((pitch / width) == 2) {
        is_565 = 1;
    }

    if (width == NDS_Wx2) {
        sw = NDS_Wx2;
        sh = NDS_Hx2;
        scale = 2;
    }

    x = (evt.mouse.x * sw) / evt.mouse.maxx;
    y = (evt.mouse.y * sh) / evt.mouse.maxy;

    if (nds.pen.img) {
        w = nds.pen.img->w;
        h = nds.pen.img->h;
        s = nds.pen.img->pixels;
    }

    switch(nds.pen.type) {
    case PEN_LT:
        break;
    case PEN_LB:
        y -= (h * scale);
        break;
    case PEN_RT:
        x -= (w * scale);
        break;
    case PEN_RB:
        x -= (w * scale);
        y -= (h * scale);
        break;
    case PEN_CP:
        x -= ((w * scale) >> 1);
        y -= ((h * scale) >> 1);
        break;
    }

#if 0
    vVertices[0] = ((((float)x) / NDS_W) - 0.5) * 2.0;
    vVertices[1] = ((((float)y) / NDS_H) - 0.5) * -2.0;

    vVertices[5] = vVertices[0];
    vVertices[6] = ((((float)(y + nds.pen.img->h)) / NDS_H) - 0.5) * -2.0;

    vVertices[10] = ((((float)(x + nds.pen.img->w)) / NDS_W) - 0.5) * 2.0;
    vVertices[11] = vVertices[6];

    vVertices[15] = vVertices[10];
    vVertices[16] = vVertices[1];

    glUniform1f(vid.alphaLoc, 1.0);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, vid.texID[TEX_PEN]);
    glVertexAttribPointer(vid.posLoc, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), vVertices);
    glVertexAttribPointer(vid.texLoc, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), &vVertices[3]);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);
    glDisable(GL_BLEND);
    glUniform1f(vid.alphaLoc, 0.0);
#endif

    asm ("PLD [%0, #128]"::"r" (s));
    for (c1=0; c1<h; c1++) {
        asm ("PLD [%0, #128]"::"r" (d_565));
        asm ("PLD [%0, #128]"::"r" (d_888));
        for (c0=0; c0<w; c0++) {
            x0 = x1 = (c0 * scale) + x;
            y0 = y1 = (c1 * scale) + (y - sub);

            if (scale == 2) {
                x0 += 1;
                y0 += 1;
            }
            if ((y0 >= 0) && (y0 < sh) && (x0 < sw) && (x0 >= 0)) {
                if (*s) {
                    if (is_565) {
                        r = (*s & 0xf80000) >> 8;
                        g = (*s & 0x00f800) >> 5;
                        b = (*s & 0x0000f8) >> 3;
                        d_565[(y1 * width) + x1] = r | g | b;
                        if (scale == 2) {
                            d_565[((y1 + 0) * width) + (x1 + 1)] = r | g | b;
                            d_565[((y1 + 1) * width) + (x1 + 0)] = r | g | b;
                            d_565[((y1 + 1) * width) + (x1 + 1)] = r | g | b;
                        }
                    }
                    else {
                        d_888[(y1 * width) + x1] = *s;
                        if (scale == 2) {
                            d_888[((y1 + 0) * width) + (x1 + 1)] = *s;
                            d_888[((y1 + 1) * width) + (x1 + 0)] = *s;
                            d_888[((y1 + 1) * width) + (x1 + 1)] = *s;
                        }
                    }
                }
            }
            s+= 1;
        }
    }
#endif
    return 0;
}

int GFX_Copy(int id, const void *pixels, SDL_Rect srcrect, SDL_Rect dstrect, int pitch, int alpha, int rotate)
{
#if defined(A30)
    int tex = (id >= 0) ? id : TEX_TMP;

    if ((id != -1) && ((nds.dis_mode == NDS_LAYOUT_13) || (nds.dis_mode == NDS_LAYOUT_15))) {
        vVertices[5] = ((((float)dstrect.x) / 640.0) - 0.5) * 2.0;
        vVertices[6] = ((((float)dstrect.y) / 480.0) - 0.5) * -2.0;

        vVertices[10] = vVertices[5];
        vVertices[11] = ((((float)(dstrect.y + dstrect.w)) / 480.0) - 0.5) * -2.0;

        vVertices[15] = ((((float)(dstrect.x + dstrect.h)) / 640.0) - 0.5) * 2.0;
        vVertices[16] = vVertices[11];

        vVertices[0] = vVertices[15];
        vVertices[1] = vVertices[6];
    }
    else if ((id != -1) && ((nds.dis_mode == NDS_LAYOUT_12) || (nds.dis_mode == NDS_LAYOUT_14))) {
        vVertices[15] = ((((float)dstrect.x) / 640.0) - 0.5) * 2.0;
        vVertices[16] = ((((float)dstrect.y) / 480.0) - 0.5) * -2.0;

        vVertices[0] = vVertices[15];
        vVertices[1] = ((((float)(dstrect.y + dstrect.w)) / 480.0) - 0.5) * -2.0;

        vVertices[5] = ((((float)(dstrect.x + dstrect.h)) / 640.0) - 0.5) * 2.0;
        vVertices[6] = vVertices[1];

        vVertices[10] = vVertices[5];
        vVertices[11] = vVertices[16];
    }
    else {
        vVertices[0] = ((((float)dstrect.x) / 640.0) - 0.5) * 2.0;
        vVertices[1] = ((((float)dstrect.y) / 480.0) - 0.5) * -2.0;

        vVertices[5] = vVertices[0];
        vVertices[6] = ((((float)(dstrect.y + dstrect.h)) / 480.0) - 0.5) * -2.0;

        vVertices[10] = ((((float)(dstrect.x + dstrect.w)) / 640.0) - 0.5) * 2.0;
        vVertices[11] = vVertices[6];

        vVertices[15] = vVertices[10];
        vVertices[16] = vVertices[1];
    }

    if (tex == TEX_TMP) {
        glBindTexture(GL_TEXTURE_2D, vid.texID[tex]);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        if (pixel_filter) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }
        else {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, srcrect.w, srcrect.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    }

    if (((nds.dis_mode == NDS_LAYOUT_0) || (nds.dis_mode == NDS_LAYOUT_1)) && (tex == TEX_SCR0)) {
        glUniform1f(vid.alphaLoc, 1.0 - ((float)nds.alpha.val / 10.0));
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_BLEND);
    }
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, vid.texID[tex]);
    glVertexAttribPointer(vid.posLoc, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), vVertices);
    glVertexAttribPointer(vid.texLoc, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), &vVertices[3]);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);

    if (((nds.dis_mode == NDS_LAYOUT_0) || (nds.dis_mode == NDS_LAYOUT_1)) && (tex == TEX_SCR0)) {
        glUniform1f(vid.alphaLoc, 0.0);
        glDisable(GL_BLEND);
    }
#endif

#ifdef MINI
    int cc = 0;
    int copy_it = 1;
    int dma_found = 0;
    MI_U16 u16Fence = 0;
    int is_rgb565 = (pitch / srcrect.w) == 2 ? 1 : 0;

    if (pixels == NULL) {
        return -1;
    }

    for (cc = 0; cc < 2; cc++) {
        if (pixels == gfx.lcd.virAddr[cc][0]) {
            dma_found = 1;
            gfx.hw.src.surf.phyAddr = gfx.lcd.phyAddr[cc][0];
            break;
        }
        if (pixels == gfx.lcd.virAddr[cc][1]) {
            dma_found = 1;
            gfx.hw.src.surf.phyAddr = gfx.lcd.phyAddr[cc][1];
            break;
        }
    }

    if (alpha != 0) {
        if (nds.alpha.val > NDS_ALPHA_MAX) {
            nds.alpha.val = 0;
        }

        if (nds.alpha.val > 0) {
            float m0 = (float)nds.alpha.val / 10;
            float m1 = 1.0 - m0;
            uint32_t *d = gfx.tmp.virAddr;
            uint32_t r0 = 0, g0 = 0, b0 = 0;
            uint32_t r1 = 0, g1 = 0, b1 = 0;
            int x = 0, y = 0, ax = 0, ay = 0, sw = 0, sh = 0;
            const uint32_t *s0 = gfx.fb.virAddr + (FB_W * gfx.vinfo.yoffset * FB_BPP);
            const uint16_t *s1_565 = pixels;
            const uint32_t *s1_888 = pixels;
            uint32_t col[] = {
                0x000000, 0xa0a0a0, 0x400000, 0x004000, 0x000040, 0x000000, 0xa0a000, 0x00a0a0
            };

            switch (nds.dis_mode) {
            case NDS_LAYOUT_0:
                sw = 170;
                sh = 128;
                break;
            case NDS_LAYOUT_1:
                sw = srcrect.w;
                sh = srcrect.h;
                break;
            }

            ay = 0;
            for (y=0; y<sh; y++) {
                switch (nds.dis_mode) {
                case NDS_LAYOUT_0:
                    if (y && ((y % 2) == 0)) {
                        ay+= 1;
                    }
                    break;
                }

                ax = 0;
                for (x=0; x<sw; x++) {
                    asm ("PLD [%0, #128]"::"r" (s0));
                    if ((nds.alpha.border > 0) && ((y == 0) || (y == (sh - 1)) || (x == 0) || (x == (sw - 1)))) {
                        *d++ = col[nds.alpha.border];
                    }
                    else {
                        switch (nds.dis_mode) {
                        case NDS_LAYOUT_0:
                            if (x && ((x % 2) == 0)) {
                                ax+= 1;
                            }
                            break;
                        }

                        if (is_rgb565) {
                            asm ("PLD [%0, #128]"::"r" (s1_565));
                            r1 = (s1_565[((y + ay) * srcrect.w) + x + ax] & 0xf800) >> 8;
                            g1 = (s1_565[((y + ay) * srcrect.w) + x + ax] & 0x07e0) >> 3;
                            b1 = (s1_565[((y + ay) * srcrect.w) + x + ax] & 0x001f) << 3;
                        }
                        else {
                            asm ("PLD [%0, #128]"::"r" (s1_888));
                            r1 = (s1_888[((y + ay) * srcrect.w) + x + ax] & 0xff0000) >> 16;
                            g1 = (s1_888[((y + ay) * srcrect.w) + x + ax] & 0x00ff00) >> 8;
                            b1 = (s1_888[((y + ay) * srcrect.w) + x + ax] & 0x0000ff) >> 0;
                        }

                        switch (nds.alpha.pos % 4) {
                        case 0:
                            r0 = (s0[((sh - y + (FB_H - sh) - 1) * FB_W) + (sw - x - 1)] & 0xff0000) >> 16;
                            g0 = (s0[((sh - y + (FB_H - sh) - 1) * FB_W) + (sw - x - 1)] & 0x00ff00) >> 8;
                            b0 = (s0[((sh - y + (FB_H - sh) - 1) * FB_W) + (sw - x - 1)] & 0x0000ff) >> 0;
                            break;
                        case 1:
                            r0 = (s0[((sh - y + (FB_H - sh) - 1) * FB_W) + (sw - x + (FB_W - sw) - 1)] & 0xff0000) >> 16;
                            g0 = (s0[((sh - y + (FB_H - sh) - 1) * FB_W) + (sw - x + (FB_W - sw) - 1)] & 0x00ff00) >> 8;
                            b0 = (s0[((sh - y + (FB_H - sh) - 1) * FB_W) + (sw - x + (FB_W - sw) - 1)] & 0x0000ff) >> 0;
                            break;
                        case 2:
                            r0 = (s0[((sh - y - 1) * FB_W) + (sw - x + (FB_W - sw) - 1)] & 0xff0000) >> 16;
                            g0 = (s0[((sh - y - 1) * FB_W) + (sw - x + (FB_W - sw) - 1)] & 0x00ff00) >> 8;
                            b0 = (s0[((sh - y - 1) * FB_W) + (sw - x + (FB_W - sw) - 1)] & 0x0000ff) >> 0;
                            break;
                        case 3:
                            r0 = (s0[((sh - y - 1) * FB_W) + (sw - x - 1)] & 0xff0000) >> 16;
                            g0 = (s0[((sh - y - 1) * FB_W) + (sw - x - 1)] & 0x00ff00) >> 8;
                            b0 = (s0[((sh - y - 1) * FB_W) + (sw - x - 1)] & 0x0000ff) >> 0;
                            break;
                        }

                        r0 = (uint8_t)((r0 * m0) + (r1 * m1));
                        g0 = (uint8_t)((g0 * m0) + (g1 * m1));
                        b0 = (uint8_t)((b0 * m0) + (b1 * m1));
                        *d++ = ((r0 << 16) | (g0 << 8) | b0);
                    }
                }
            }
            copy_it = 0;
        }

        switch (nds.dis_mode) {
        case NDS_LAYOUT_0:
            dstrect.w = 170;
            dstrect.h = 128;
            if (nds.alpha.val > 0) {
                srcrect.w = dstrect.w;
                srcrect.h = dstrect.h;
                pitch = srcrect.w * 4;
            }
            break;
        case NDS_LAYOUT_1:
            dstrect.w = NDS_W;
            dstrect.h = NDS_H;
            if (nds.alpha.val > 0) {
                srcrect.w = dstrect.w;
                srcrect.h = dstrect.h;
                pitch = srcrect.w * 4;
            }
            break;
        }

        switch (nds.alpha.pos % 4) {
        case 0:
            dstrect.x = 0;
            dstrect.y = FB_H - dstrect.h;
            break;
        case 1:
            dstrect.x = FB_W - dstrect.w;
            dstrect.y = FB_H - dstrect.h;
            break;
        case 2:
            dstrect.x = FB_W - dstrect.w;
            dstrect.y = 0;
            break;
        case 3:
            dstrect.x = 0;
            dstrect.y = 0;
            break;
        }
    }

    if (copy_it && pixel_filter) {
        do {
            if (nds.hres_mode != 0) {
                break;
            }

            if ((srcrect.w != NDS_W) || (srcrect.h != NDS_H)) {
                break;
            }

            if (pitch == 1024) {
#if USE_MASK
                if (dma_found) {
                    gfx.hw.opt.u32GlobalSrcConstColor = 0;
                    gfx.hw.opt.eRotate = 0;
                    gfx.hw.opt.eSrcDfbBldOp = E_MI_GFX_DFB_BLD_ONE;
                    gfx.hw.opt.eDstDfbBldOp = 0;
                    gfx.hw.opt.eDFBBlendFlag = 0;

                    gfx.hw.src.rt.s32Xpos = 0;
                    gfx.hw.src.rt.s32Ypos = 0;
                    gfx.hw.src.rt.u32Width = NDS_W;
                    gfx.hw.src.rt.u32Height = NDS_H;
                    gfx.hw.src.surf.u32Width = NDS_W;
                    gfx.hw.src.surf.u32Height = NDS_H;
                    gfx.hw.src.surf.u32Stride = pitch;
                    gfx.hw.src.surf.eColorFmt = is_rgb565 ? E_MI_GFX_FMT_RGB565 : E_MI_GFX_FMT_ARGB8888;

                    gfx.hw.dst.rt.s32Xpos = 0;
                    gfx.hw.dst.rt.s32Ypos = 0;
                    gfx.hw.dst.rt.u32Width = NDS_Wx3;
                    gfx.hw.dst.rt.u32Height = NDS_Hx3;
                    gfx.hw.dst.surf.u32Width = NDS_Wx3;
                    gfx.hw.dst.surf.u32Height = NDS_Hx3;
                    gfx.hw.dst.surf.u32Stride = NDS_Wx3 * FB_BPP;
                    gfx.hw.dst.surf.eColorFmt = E_MI_GFX_FMT_ARGB8888;
                    gfx.hw.dst.surf.phyAddr = gfx.mask.phyAddr[1];

                    MI_GFX_BitBlit(&gfx.hw.src.surf, &gfx.hw.src.rt, &gfx.hw.dst.surf, &gfx.hw.dst.rt, &gfx.hw.opt, &u16Fence);
                    MI_GFX_WaitAllDone(FALSE, u16Fence);

                    gfx.hw.overlay.surf.phyAddr = gfx.mask.phyAddr[0];
                    gfx.hw.overlay.surf.eColorFmt = E_MI_GFX_FMT_ARGB8888;
                    gfx.hw.overlay.surf.u32Width = NDS_Wx3;
                    gfx.hw.overlay.surf.u32Height = NDS_Hx3;
                    gfx.hw.overlay.surf.u32Stride = NDS_Wx3 * FB_BPP;
                    gfx.hw.overlay.rt.s32Xpos = 0;
                    gfx.hw.overlay.rt.s32Ypos = 0;
                    gfx.hw.overlay.rt.u32Width = NDS_Wx3;
                    gfx.hw.overlay.rt.u32Height = NDS_Hx3;

                    gfx.hw.dst.rt.s32Xpos = 0;
                    gfx.hw.dst.rt.s32Ypos = 0;
                    gfx.hw.dst.rt.u32Width = NDS_Wx3;
                    gfx.hw.dst.rt.u32Height = NDS_Hx3;
                    gfx.hw.dst.surf.phyAddr = gfx.mask.phyAddr[1];

                    gfx.hw.opt.u32GlobalSrcConstColor = 0xff000000;
                    gfx.hw.opt.eRotate = 0;
                    gfx.hw.opt.eSrcDfbBldOp = E_MI_GFX_DFB_BLD_ONE;
                    gfx.hw.opt.eDstDfbBldOp = E_MI_GFX_DFB_BLD_INVSRCALPHA;
                    gfx.hw.opt.eDFBBlendFlag = E_MI_GFX_DFB_BLEND_SRC_PREMULTIPLY | E_MI_GFX_DFB_BLEND_COLORALPHA | E_MI_GFX_DFB_BLEND_ALPHACHANNEL;
                    MI_GFX_BitBlit(&gfx.hw.overlay.surf, &gfx.hw.overlay.rt, &gfx.hw.dst.surf, &gfx.hw.dst.rt, &gfx.hw.opt, &u16Fence);
                    MI_GFX_WaitAllDone(FALSE, u16Fence);

                    copy_it = 0;
                    srcrect.x = 0;
                    srcrect.y = 0;
                    srcrect.w = NDS_Wx3;
                    srcrect.h = NDS_Hx3;
                    pitch = srcrect.w * 4;
                }
                else
#endif
                {
                    asm volatile (
                        "0:  add r8, %1, %2         ;"
                        "1:  vldmia %0!, {q0-q3}    ;"
                        "    vldmia %0!, {q8-q11}   ;"
                        "    vdup.32 d15, d7[1]     ;"
                        "    vdup.32 d14, d7[0]     ;"
                        "    vdup.32 d13, d6[1]     ;"
                        "    vdup.32 d12, d6[0]     ;"
                        "    vdup.32 d11, d5[1]     ;"
                        "    vdup.32 d10, d5[0]     ;"
                        "    vdup.32 d9, d4[1]      ;"
                        "    vdup.32 d8, d4[0]      ;"
                        "    vdup.32 d7, d3[1]      ;"
                        "    vdup.32 d6, d3[0]      ;"
                        "    vdup.32 d5, d2[1]      ;"
                        "    vdup.32 d4, d2[0]      ;"
                        "    vdup.32 d3, d1[1]      ;"
                        "    vdup.32 d2, d1[0]      ;"
                        "    vdup.32 d1, d0[1]      ;"
                        "    vdup.32 d0, d0[0]      ;"
                        "    vdup.32 d31, d23[1]    ;"
                        "    vdup.32 d30, d23[0]    ;"
                        "    vdup.32 d29, d22[1]    ;"
                        "    vdup.32 d28, d22[0]    ;"
                        "    vdup.32 d27, d21[1]    ;"
                        "    vdup.32 d26, d21[0]    ;"
                        "    vdup.32 d25, d20[1]    ;"
                        "    vdup.32 d24, d20[0]    ;"
                        "    vdup.32 d23, d19[1]    ;"
                        "    vdup.32 d22, d19[0]    ;"
                        "    vdup.32 d21, d18[1]    ;"
                        "    vdup.32 d20, d18[0]    ;"
                        "    vdup.32 d19, d17[1]    ;"
                        "    vdup.32 d18, d17[0]    ;"
                        "    vdup.32 d17, d16[1]    ;"
                        "    vdup.32 d16, d16[0]    ;"
                        "    vstmia %1!, {q0-q7}    ;"
                        "    vstmia %1!, {q8-q15}   ;"
                        "    vstmia r8!, {q0-q7}    ;"
                        "    vstmia r8!, {q8-q15}   ;"
                        "2:  vldmia %0!, {q0-q3}    ;"
                        "    vldmia %0!, {q8-q11}   ;"
                        "    vdup.32 d15, d7[1]     ;"
                        "    vdup.32 d14, d7[0]     ;"
                        "    vdup.32 d13, d6[1]     ;"
                        "    vdup.32 d12, d6[0]     ;"
                        "    vdup.32 d11, d5[1]     ;"
                        "    vdup.32 d10, d5[0]     ;"
                        "    vdup.32 d9, d4[1]      ;"
                        "    vdup.32 d8, d4[0]      ;"
                        "    vdup.32 d7, d3[1]      ;"
                        "    vdup.32 d6, d3[0]      ;"
                        "    vdup.32 d5, d2[1]      ;"
                        "    vdup.32 d4, d2[0]      ;"
                        "    vdup.32 d3, d1[1]      ;"
                        "    vdup.32 d2, d1[0]      ;"
                        "    vdup.32 d1, d0[1]      ;"
                        "    vdup.32 d0, d0[0]      ;"
                        "    vdup.32 d31, d23[1]    ;"
                        "    vdup.32 d30, d23[0]    ;"
                        "    vdup.32 d29, d22[1]    ;"
                        "    vdup.32 d28, d22[0]    ;"
                        "    vdup.32 d27, d21[1]    ;"
                        "    vdup.32 d26, d21[0]    ;"
                        "    vdup.32 d25, d20[1]    ;"
                        "    vdup.32 d24, d20[0]    ;"
                        "    vdup.32 d23, d19[1]    ;"
                        "    vdup.32 d22, d19[0]    ;"
                        "    vdup.32 d21, d18[1]    ;"
                        "    vdup.32 d20, d18[0]    ;"
                        "    vdup.32 d19, d17[1]    ;"
                        "    vdup.32 d18, d17[0]    ;"
                        "    vdup.32 d17, d16[1]    ;"
                        "    vdup.32 d16, d16[0]    ;"
                        "    vstmia %1!, {q0-q7}    ;"
                        "    vstmia %1!, {q8-q15}   ;"
                        "    vstmia r8!, {q0-q7}    ;"
                        "    vstmia r8!, {q8-q15}   ;"
                        "3:  vldmia %0!, {q0-q3}    ;"
                        "    vldmia %0!, {q8-q11}   ;"
                        "    vdup.32 d15, d7[1]     ;"
                        "    vdup.32 d14, d7[0]     ;"
                        "    vdup.32 d13, d6[1]     ;"
                        "    vdup.32 d12, d6[0]     ;"
                        "    vdup.32 d11, d5[1]     ;"
                        "    vdup.32 d10, d5[0]     ;"
                        "    vdup.32 d9, d4[1]      ;"
                        "    vdup.32 d8, d4[0]      ;"
                        "    vdup.32 d7, d3[1]      ;"
                        "    vdup.32 d6, d3[0]      ;"
                        "    vdup.32 d5, d2[1]      ;"
                        "    vdup.32 d4, d2[0]      ;"
                        "    vdup.32 d3, d1[1]      ;"
                        "    vdup.32 d2, d1[0]      ;"
                        "    vdup.32 d1, d0[1]      ;"
                        "    vdup.32 d0, d0[0]      ;"
                        "    vdup.32 d31, d23[1]    ;"
                        "    vdup.32 d30, d23[0]    ;"
                        "    vdup.32 d29, d22[1]    ;"
                        "    vdup.32 d28, d22[0]    ;"
                        "    vdup.32 d27, d21[1]    ;"
                        "    vdup.32 d26, d21[0]    ;"
                        "    vdup.32 d25, d20[1]    ;"
                        "    vdup.32 d24, d20[0]    ;"
                        "    vdup.32 d23, d19[1]    ;"
                        "    vdup.32 d22, d19[0]    ;"
                        "    vdup.32 d21, d18[1]    ;"
                        "    vdup.32 d20, d18[0]    ;"
                        "    vdup.32 d19, d17[1]    ;"
                        "    vdup.32 d18, d17[0]    ;"
                        "    vdup.32 d17, d16[1]    ;"
                        "    vdup.32 d16, d16[0]    ;"
                        "    vstmia %1!, {q0-q7}    ;"
                        "    vstmia %1!, {q8-q15}   ;"
                        "    vstmia r8!, {q0-q7}    ;"
                        "    vstmia r8!, {q8-q15}   ;"
                        "4:  vldmia %0!, {q0-q3}    ;"
                        "    vldmia %0!, {q8-q11}   ;"
                        "    vdup.32 d15, d7[1]     ;"
                        "    vdup.32 d14, d7[0]     ;"
                        "    vdup.32 d13, d6[1]     ;"
                        "    vdup.32 d12, d6[0]     ;"
                        "    vdup.32 d11, d5[1]     ;"
                        "    vdup.32 d10, d5[0]     ;"
                        "    vdup.32 d9, d4[1]      ;"
                        "    vdup.32 d8, d4[0]      ;"
                        "    vdup.32 d7, d3[1]      ;"
                        "    vdup.32 d6, d3[0]      ;"
                        "    vdup.32 d5, d2[1]      ;"
                        "    vdup.32 d4, d2[0]      ;"
                        "    vdup.32 d3, d1[1]      ;"
                        "    vdup.32 d2, d1[0]      ;"
                        "    vdup.32 d1, d0[1]      ;"
                        "    vdup.32 d0, d0[0]      ;"
                        "    vdup.32 d31, d23[1]    ;"
                        "    vdup.32 d30, d23[0]    ;"
                        "    vdup.32 d29, d22[1]    ;"
                        "    vdup.32 d28, d22[0]    ;"
                        "    vdup.32 d27, d21[1]    ;"
                        "    vdup.32 d26, d21[0]    ;"
                        "    vdup.32 d25, d20[1]    ;"
                        "    vdup.32 d24, d20[0]    ;"
                        "    vdup.32 d23, d19[1]    ;"
                        "    vdup.32 d22, d19[0]    ;"
                        "    vdup.32 d21, d18[1]    ;"
                        "    vdup.32 d20, d18[0]    ;"
                        "    vdup.32 d19, d17[1]    ;"
                        "    vdup.32 d18, d17[0]    ;"
                        "    vdup.32 d17, d16[1]    ;"
                        "    vdup.32 d16, d16[0]    ;"
                        "    vstmia %1!, {q0-q7}    ;"
                        "    vstmia %1!, {q8-q15}   ;"
                        "    vstmia r8!, {q0-q7}    ;"
                        "    vstmia r8!, {q8-q15}   ;"
                        "5:  vldmia %0!, {q0-q3}    ;"
                        "    vldmia %0!, {q8-q11}   ;"
                        "    vdup.32 d15, d7[1]     ;"
                        "    vdup.32 d14, d7[0]     ;"
                        "    vdup.32 d13, d6[1]     ;"
                        "    vdup.32 d12, d6[0]     ;"
                        "    vdup.32 d11, d5[1]     ;"
                        "    vdup.32 d10, d5[0]     ;"
                        "    vdup.32 d9, d4[1]      ;"
                        "    vdup.32 d8, d4[0]      ;"
                        "    vdup.32 d7, d3[1]      ;"
                        "    vdup.32 d6, d3[0]      ;"
                        "    vdup.32 d5, d2[1]      ;"
                        "    vdup.32 d4, d2[0]      ;"
                        "    vdup.32 d3, d1[1]      ;"
                        "    vdup.32 d2, d1[0]      ;"
                        "    vdup.32 d1, d0[1]      ;"
                        "    vdup.32 d0, d0[0]      ;"
                        "    vdup.32 d31, d23[1]    ;"
                        "    vdup.32 d30, d23[0]    ;"
                        "    vdup.32 d29, d22[1]    ;"
                        "    vdup.32 d28, d22[0]    ;"
                        "    vdup.32 d27, d21[1]    ;"
                        "    vdup.32 d26, d21[0]    ;"
                        "    vdup.32 d25, d20[1]    ;"
                        "    vdup.32 d24, d20[0]    ;"
                        "    vdup.32 d23, d19[1]    ;"
                        "    vdup.32 d22, d19[0]    ;"
                        "    vdup.32 d21, d18[1]    ;"
                        "    vdup.32 d20, d18[0]    ;"
                        "    vdup.32 d19, d17[1]    ;"
                        "    vdup.32 d18, d17[0]    ;"
                        "    vdup.32 d17, d16[1]    ;"
                        "    vdup.32 d16, d16[0]    ;"
                        "    vstmia %1!, {q0-q7}    ;"
                        "    vstmia %1!, {q8-q15}   ;"
                        "    vstmia r8!, {q0-q7}    ;"
                        "    vstmia r8!, {q8-q15}   ;"
                        "6:  vldmia %0!, {q0-q3}    ;"
                        "    vldmia %0!, {q8-q11}   ;"
                        "    vdup.32 d15, d7[1]     ;"
                        "    vdup.32 d14, d7[0]     ;"
                        "    vdup.32 d13, d6[1]     ;"
                        "    vdup.32 d12, d6[0]     ;"
                        "    vdup.32 d11, d5[1]     ;"
                        "    vdup.32 d10, d5[0]     ;"
                        "    vdup.32 d9, d4[1]      ;"
                        "    vdup.32 d8, d4[0]      ;"
                        "    vdup.32 d7, d3[1]      ;"
                        "    vdup.32 d6, d3[0]      ;"
                        "    vdup.32 d5, d2[1]      ;"
                        "    vdup.32 d4, d2[0]      ;"
                        "    vdup.32 d3, d1[1]      ;"
                        "    vdup.32 d2, d1[0]      ;"
                        "    vdup.32 d1, d0[1]      ;"
                        "    vdup.32 d0, d0[0]      ;"
                        "    vdup.32 d31, d23[1]    ;"
                        "    vdup.32 d30, d23[0]    ;"
                        "    vdup.32 d29, d22[1]    ;"
                        "    vdup.32 d28, d22[0]    ;"
                        "    vdup.32 d27, d21[1]    ;"
                        "    vdup.32 d26, d21[0]    ;"
                        "    vdup.32 d25, d20[1]    ;"
                        "    vdup.32 d24, d20[0]    ;"
                        "    vdup.32 d23, d19[1]    ;"
                        "    vdup.32 d22, d19[0]    ;"
                        "    vdup.32 d21, d18[1]    ;"
                        "    vdup.32 d20, d18[0]    ;"
                        "    vdup.32 d19, d17[1]    ;"
                        "    vdup.32 d18, d17[0]    ;"
                        "    vdup.32 d17, d16[1]    ;"
                        "    vdup.32 d16, d16[0]    ;"
                        "    vstmia %1!, {q0-q7}    ;"
                        "    vstmia %1!, {q8-q15}   ;"
                        "    vstmia r8!, {q0-q7}    ;"
                        "    vstmia r8!, {q8-q15}   ;"
                        "7:  vldmia %0!, {q0-q3}    ;"
                        "    vldmia %0!, {q8-q11}   ;"
                        "    vdup.32 d15, d7[1]     ;"
                        "    vdup.32 d14, d7[0]     ;"
                        "    vdup.32 d13, d6[1]     ;"
                        "    vdup.32 d12, d6[0]     ;"
                        "    vdup.32 d11, d5[1]     ;"
                        "    vdup.32 d10, d5[0]     ;"
                        "    vdup.32 d9, d4[1]      ;"
                        "    vdup.32 d8, d4[0]      ;"
                        "    vdup.32 d7, d3[1]      ;"
                        "    vdup.32 d6, d3[0]      ;"
                        "    vdup.32 d5, d2[1]      ;"
                        "    vdup.32 d4, d2[0]      ;"
                        "    vdup.32 d3, d1[1]      ;"
                        "    vdup.32 d2, d1[0]      ;"
                        "    vdup.32 d1, d0[1]      ;"
                        "    vdup.32 d0, d0[0]      ;"
                        "    vdup.32 d31, d23[1]    ;"
                        "    vdup.32 d30, d23[0]    ;"
                        "    vdup.32 d29, d22[1]    ;"
                        "    vdup.32 d28, d22[0]    ;"
                        "    vdup.32 d27, d21[1]    ;"
                        "    vdup.32 d26, d21[0]    ;"
                        "    vdup.32 d25, d20[1]    ;"
                        "    vdup.32 d24, d20[0]    ;"
                        "    vdup.32 d23, d19[1]    ;"
                        "    vdup.32 d22, d19[0]    ;"
                        "    vdup.32 d21, d18[1]    ;"
                        "    vdup.32 d20, d18[0]    ;"
                        "    vdup.32 d19, d17[1]    ;"
                        "    vdup.32 d18, d17[0]    ;"
                        "    vdup.32 d17, d16[1]    ;"
                        "    vdup.32 d16, d16[0]    ;"
                        "    vstmia %1!, {q0-q7}    ;"
                        "    vstmia %1!, {q8-q15}   ;"
                        "    vstmia r8!, {q0-q7}    ;"
                        "    vstmia r8!, {q8-q15}   ;"
                        "8:  vldmia %0!, {q0-q3}    ;"
                        "    vldmia %0!, {q8-q11}   ;"
                        "    vdup.32 d15, d7[1]     ;"
                        "    vdup.32 d14, d7[0]     ;"
                        "    vdup.32 d13, d6[1]     ;"
                        "    vdup.32 d12, d6[0]     ;"
                        "    vdup.32 d11, d5[1]     ;"
                        "    vdup.32 d10, d5[0]     ;"
                        "    vdup.32 d9, d4[1]      ;"
                        "    vdup.32 d8, d4[0]      ;"
                        "    vdup.32 d7, d3[1]      ;"
                        "    vdup.32 d6, d3[0]      ;"
                        "    vdup.32 d5, d2[1]      ;"
                        "    vdup.32 d4, d2[0]      ;"
                        "    vdup.32 d3, d1[1]      ;"
                        "    vdup.32 d2, d1[0]      ;"
                        "    vdup.32 d1, d0[1]      ;"
                        "    vdup.32 d0, d0[0]      ;"
                        "    vdup.32 d31, d23[1]    ;"
                        "    vdup.32 d30, d23[0]    ;"
                        "    vdup.32 d29, d22[1]    ;"
                        "    vdup.32 d28, d22[0]    ;"
                        "    vdup.32 d27, d21[1]    ;"
                        "    vdup.32 d26, d21[0]    ;"
                        "    vdup.32 d25, d20[1]    ;"
                        "    vdup.32 d24, d20[0]    ;"
                        "    vdup.32 d23, d19[1]    ;"
                        "    vdup.32 d22, d19[0]    ;"
                        "    vdup.32 d21, d18[1]    ;"
                        "    vdup.32 d20, d18[0]    ;"
                        "    vdup.32 d19, d17[1]    ;"
                        "    vdup.32 d18, d17[0]    ;"
                        "    vdup.32 d17, d16[1]    ;"
                        "    vdup.32 d16, d16[0]    ;"
                        "    vstmia %1!, {q0-q7}    ;"
                        "    vstmia %1!, {q8-q15}   ;"
                        "    vstmia r8!, {q0-q7}    ;"
                        "    vstmia r8!, {q8-q15}   ;"
                        "    add %1, %1, %2         ;"
                        "    subs %3, #1            ;"
                        "    bne 0b                 ;"
                        :
                        : "r"(pixels), "r"(gfx.tmp.virAddr), "r"(NDS_Wx2 * 4), "r"(NDS_H)
                        : "r8", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7", "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15", "memory", "cc"
                    );

                    copy_it = 0;
                    srcrect.x = 0;
                    srcrect.y = 0;
                    srcrect.w = NDS_Wx2;
                    srcrect.h = NDS_Hx2;
                    pitch = srcrect.w * 4;
                }
            }
            else {
                int x = 0, y = 0;
                uint16_t *s0 = NULL;
                uint16_t *s1 = (uint16_t*)pixels;
                uint16_t *d = (uint16_t*)gfx.tmp.virAddr;

                for (y=0; y<srcrect.h; y++) {
                    s0 = d;
                    for (x=0; x<srcrect.w; x++) {
                        *d++ = *s1;
                        *d++ = *s1++;
                    }
                    neon_memcpy(d, s0, 1024);
                    d+= NDS_Wx2;
                }

                copy_it = 0;
                srcrect.x = 0;
                srcrect.y = 0;
                srcrect.w = NDS_Wx2;
                srcrect.h = NDS_Hx2;
                pitch = srcrect.w * 2;
            }
        } while(0);
    }

    if (copy_it) {
        if (dma_found == 0) {
            neon_memcpy(gfx.tmp.virAddr, pixels, srcrect.h * pitch);
            gfx.hw.src.surf.phyAddr = gfx.tmp.phyAddr;
            MI_SYS_FlushInvCache(gfx.tmp.virAddr, pitch * srcrect.h);
        }
    }
    else {
#if USE_MASK
        gfx.hw.src.surf.phyAddr = gfx.mask.phyAddr[1];
        MI_SYS_FlushInvCache(gfx.tmp.virAddr, pitch * srcrect.h);
#else
        gfx.hw.src.surf.phyAddr = gfx.tmp.phyAddr;
        MI_SYS_FlushInvCache(gfx.tmp.virAddr, pitch * srcrect.h);
#endif
    }

    gfx.hw.opt.u32GlobalSrcConstColor = 0;
    gfx.hw.opt.eRotate = rotate;
    gfx.hw.opt.eSrcDfbBldOp = E_MI_GFX_DFB_BLD_ONE;
    gfx.hw.opt.eDstDfbBldOp = 0;
    gfx.hw.opt.eDFBBlendFlag = 0;

    gfx.hw.src.rt.s32Xpos = srcrect.x;
    gfx.hw.src.rt.s32Ypos = srcrect.y;
    gfx.hw.src.rt.u32Width = srcrect.w;
    gfx.hw.src.rt.u32Height = srcrect.h;
    gfx.hw.src.surf.u32Width = srcrect.w;
    gfx.hw.src.surf.u32Height = srcrect.h;
    gfx.hw.src.surf.u32Stride = pitch;
    gfx.hw.src.surf.eColorFmt = is_rgb565 ? E_MI_GFX_FMT_RGB565 : E_MI_GFX_FMT_ARGB8888;

    gfx.hw.dst.rt.s32Xpos = dstrect.x;
    gfx.hw.dst.rt.s32Ypos = dstrect.y;
    gfx.hw.dst.rt.u32Width = dstrect.w;
    gfx.hw.dst.rt.u32Height = dstrect.h;
    gfx.hw.dst.surf.u32Width = FB_W;
    gfx.hw.dst.surf.u32Height = FB_H;
    gfx.hw.dst.surf.u32Stride = FB_W * FB_BPP;
    gfx.hw.dst.surf.eColorFmt = E_MI_GFX_FMT_ARGB8888;
    gfx.hw.dst.surf.phyAddr = gfx.fb.phyAddr + (FB_W * gfx.vinfo.yoffset * FB_BPP);

    MI_GFX_BitBlit(&gfx.hw.src.surf, &gfx.hw.src.rt, &gfx.hw.dst.surf, &gfx.hw.dst.rt, &gfx.hw.opt, &u16Fence);
    MI_GFX_WaitAllDone(FALSE, u16Fence);

    if ((nds.menu.enable == 0) && (srcrect.w != 800) && ((srcrect.w == NDS_W) || (srcrect.w == NDS_Wx2)) && (nds.overlay.sel < nds.overlay.max)) {
        gfx.hw.overlay.surf.phyAddr = gfx.overlay.phyAddr;
        gfx.hw.overlay.surf.eColorFmt = E_MI_GFX_FMT_ARGB8888;
        gfx.hw.overlay.surf.u32Width = FB_W;
        gfx.hw.overlay.surf.u32Height = FB_H;
        gfx.hw.overlay.surf.u32Stride = FB_W * FB_BPP;
        gfx.hw.overlay.rt.s32Xpos = 0;
        gfx.hw.overlay.rt.s32Ypos = 0;
        gfx.hw.overlay.rt.u32Width = FB_W;
        gfx.hw.overlay.rt.u32Height = FB_H;

        gfx.hw.dst.rt.s32Xpos = 0;
        gfx.hw.dst.rt.s32Ypos = 0;
        gfx.hw.dst.rt.u32Width = FB_W;
        gfx.hw.dst.rt.u32Height = FB_H;
        gfx.hw.dst.surf.phyAddr = gfx.fb.phyAddr + (FB_W * gfx.vinfo.yoffset * FB_BPP);

        gfx.hw.opt.u32GlobalSrcConstColor = 0xff000000;
        gfx.hw.opt.eRotate = E_MI_GFX_ROTATE_180;
        gfx.hw.opt.eSrcDfbBldOp = E_MI_GFX_DFB_BLD_ONE;
        gfx.hw.opt.eDstDfbBldOp = E_MI_GFX_DFB_BLD_INVSRCALPHA;
        gfx.hw.opt.eDFBBlendFlag = E_MI_GFX_DFB_BLEND_SRC_PREMULTIPLY | E_MI_GFX_DFB_BLEND_COLORALPHA | E_MI_GFX_DFB_BLEND_ALPHACHANNEL;
        MI_GFX_BitBlit(&gfx.hw.overlay.surf, &gfx.hw.overlay.rt, &gfx.hw.dst.surf, &gfx.hw.dst.rt, &gfx.hw.opt, &u16Fence);
        MI_GFX_WaitAllDone(FALSE, u16Fence);
    }
#endif
    return 0;
}

void GFX_Flip(void)
{
#if defined(A30)
    eglSwapBuffers(vid.eglDisplay, vid.eglSurface);

    if (nds.theme.img) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, vid.texID[TEX_BG]);
        glVertexAttribPointer(vid.posLoc, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), bgVertices);
        glVertexAttribPointer(vid.texLoc, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), &bgVertices[3]);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);
    }
#endif

#ifdef MINI
    ioctl(gfx.fb_dev, FBIOPAN_DISPLAY, &gfx.vinfo);
    gfx.vinfo.yoffset ^= FB_H;
#endif
}

int get_font_width(const char *info)
{
    int w = 0, h = 0;

    if (nds.font && info) {
        TTF_SizeUTF8(nds.font, info, &w, &h);
    }
    return w;
}

int get_font_height(const char *info)
{
    int w = 0, h = 0;

    if (nds.font && info) {
        TTF_SizeUTF8(nds.font, info, &w, &h);
    }
    return h;
}

const char *to_lang(const char *p)
{
    const char *info = p;
    char buf[MAX_PATH] = {0};
    int cc = 0, r = 0, len = 0;
    
    if (!strcmp(nds.lang.trans[DEF_LANG_SLOT], DEF_LANG_LANG) || (p == NULL)) {
        return p;
    }

    strcpy(buf, p);
    strcat(buf, "=");
    len = strlen(buf);
    if ((len == 0) || (len >= MAX_PATH)) {
        return 0;
    }

    for (cc=0; translate[cc]; cc++) {
        if (memcmp(buf, translate[cc], len) == 0) {
            r = 1;
            info = &translate[cc][len];
            //printf(PREFIX"Translate \'%s\' as \'%s\'\n", p, info);
            break;
        }
    }

    if (r == 0) {
        //printf(PREFIX"Failed to find the translation: \'%s\'(len=%d)\n", p, len);
        info = p;
    }
    return info;
}

int draw_info(SDL_Surface *dst, const char *info, int x, int y, uint32_t fgcolor, uint32_t bgcolor)
{
    int w = 0, h = 0;
    SDL_Color fg = {0};
    SDL_Rect rt = {0, 0, 0, 0};
    SDL_Surface *t0 = NULL;
    SDL_Surface *t1 = NULL;
    SDL_Surface *t2 = NULL;

    h = strlen(info);
    if ((nds.font == NULL) || (h == 0) || (h >= MAX_PATH)) {
        return -1;
    }

    fg.r = (fgcolor >> 16) & 0xff;
    fg.g = (fgcolor >> 8) & 0xff;
    fg.b = (fgcolor >> 0) & 0xff;
    TTF_SizeUTF8(nds.font, info, &w, &h);
    t0 = TTF_RenderUTF8_Solid(nds.font, info, fg);
    if (t0) {
        if (dst == NULL) {
            t1 = SDL_CreateRGBSurface(SDL_SWSURFACE, t0->w, t0->h, 32, 0, 0, 0, 0);
            if (t1) {
                SDL_FillRect(t1, &t1->clip_rect, bgcolor);
                SDL_BlitSurface(t0, NULL, t1, NULL);

                t2 = SDL_ConvertSurface(t1, cvt->format, 0);
                if (t2) {
                    rt.x = x;
                    rt.y = y;
                    rt.w = t2->w;
                    rt.h = t2->h;
                    GFX_Copy(-1, t2->pixels, t2->clip_rect, rt, t2->pitch, 0, E_MI_GFX_ROTATE_180);
                    SDL_FreeSurface(t2);
                }
                SDL_FreeSurface(t1);
            }
        }
        else {
            rt.x = x;
            rt.y = y;
            SDL_BlitSurface(t0, NULL, dst, &rt);
        }
        SDL_FreeSurface(t0);
    }
    return 0;
}

int reload_pen(void)
{
    static int pre_sel = -1;

    char buf[MAX_PATH] = {0};
    SDL_Surface *t = NULL;

    if (pre_sel != nds.pen.sel) {
        pre_sel = nds.pen.sel;

        if (nds.pen.img) {
            SDL_FreeSurface(nds.pen.img);
            nds.pen.img = NULL;
        }

        nds.pen.type = PEN_LB;
        if (get_file_path(nds.pen.path, nds.pen.sel, buf, 1) == 0) {
            t = IMG_Load(buf);
            if (t) {
#if defined(A30)
                int x = 0;
                int y = 0;
                uint32_t *p = malloc(t->pitch * t->h);
                uint32_t *src = t->pixels;
                uint32_t *dst = p;

                for (y = 0; y < t->h; y++) {
                    for (x = 0; x < t->w; x++) {
                        *dst++ = *src++;
                    }
                }
                glBindTexture(GL_TEXTURE_2D, vid.texID[TEX_PEN]);
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t->w, t->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, p);
                free(p);
#endif
                nds.pen.img = SDL_ConvertSurface(t, cvt->format, 0);
                SDL_FreeSurface(t);

                if (strstr(buf, "_lt")) {
                    nds.pen.type = PEN_LT;
                }
                else if (strstr(buf, "_rt")) {
                    nds.pen.type = PEN_RT;
                }
                else if (strstr(buf, "_rb")) {
                    nds.pen.type = PEN_RB;
                }
                else if (strstr(buf, "_lb")) {
                    nds.pen.type = PEN_LB;
                }
                else {
                    nds.pen.type = PEN_CP;
                }
            }
            else {
                printf(PREFIX"Failed to load pen (%s)\n", buf);
            }
        }
    }
    return 0;
}

int reload_menu(void)
{
    SDL_Surface *t = NULL;
    char folder[MAX_PATH] = {0};
    char buf[MAX_PATH << 1] = {0};

    if (get_dir_path(nds.menu.path, nds.menu.sel, folder) != 0) {
        return -1;
    }

    snprintf(buf, sizeof(buf), "%s/%s", folder, MENU_BG_FILE);
    t = IMG_Load(buf);
    if (t) {
        if (nds.menu.bg) {
            SDL_FreeSurface(nds.menu.bg);
        }
        nds.menu.bg = SDL_ConvertSurface(t, cvt->format, 0);
        SDL_FreeSurface(t);
    }

    snprintf(buf, sizeof(buf), "%s/%s", folder, MENU_CURSOR_FILE);
    nds.menu.cursor = IMG_Load(buf);

    snprintf(buf, sizeof(buf), "%s/%s", folder, DRASTIC_MENU_BG0_FILE);
    t = IMG_Load(buf);
    if (t) {
        if (nds.menu.drastic.bg0) {
            SDL_FreeSurface(nds.menu.drastic.bg0);
        }
        nds.menu.drastic.bg0 = SDL_ConvertSurface(t, cvt->format, 0);
        SDL_FreeSurface(t);
    }

    snprintf(buf, sizeof(buf), "%s/%s", folder, DRASTIC_MENU_BG1_FILE);
    t = IMG_Load(buf);
    if (t) {
        if (nds.menu.drastic.bg1) {
            SDL_FreeSurface(nds.menu.drastic.bg1);
        }
        nds.menu.drastic.bg1 = SDL_ConvertSurface(t, cvt->format, 0);
        SDL_FreeSurface(t);
    }

    snprintf(buf, sizeof(buf), "%s/%s", folder, DRASTIC_MENU_CURSOR_FILE);
    nds.menu.drastic.cursor = IMG_Load(buf);

    snprintf(buf, sizeof(buf), "%s/%s", folder, DRASTIC_MENU_YES_FILE);
    t = IMG_Load(buf);
    if (t) {
        SDL_Rect nrt = {0, 0, LINE_H - 2, LINE_H - 2};
        if (nds.menu.drastic.yes) {
            SDL_FreeSurface(nds.menu.drastic.yes);
        }
        nds.menu.drastic.yes = SDL_CreateRGBSurface(SDL_SWSURFACE, nrt.w, nrt.h, 32, t->format->Rmask, t->format->Gmask, t->format->Bmask, t->format->Amask);
        if (nds.menu.drastic.yes) {
            SDL_SoftStretch(t, NULL, nds.menu.drastic.yes, NULL);
        }
        SDL_FreeSurface(t);
    }

    snprintf(buf, sizeof(buf), "%s/%s", folder, DRASTIC_MENU_NO_FILE);
    t = IMG_Load(buf);
    if (t) {
        SDL_Rect nrt = {0, 0, LINE_H - 2, LINE_H - 2};
        if (nds.menu.drastic.no) {
            SDL_FreeSurface(nds.menu.drastic.no);
        }
        nds.menu.drastic.no = SDL_CreateRGBSurface(SDL_SWSURFACE, nrt.w, nrt.h, 32, t->format->Rmask, t->format->Gmask, t->format->Bmask, t->format->Amask);
        if (nds.menu.drastic.no) {
            SDL_SoftStretch(t, NULL, nds.menu.drastic.no, NULL);
        }
        SDL_FreeSurface(t);
    }

    return 0;
}

int reload_bg(void)
{
    static int pre_sel = -1;
    static int pre_mode = -1;

    char buf[MAX_PATH] = {0};
    SDL_Surface *t = NULL;
    SDL_Rect srt = {0, 0, IMG_W, IMG_H};
    SDL_Rect drt = {0, 0, FB_W, FB_H};

    if (nds.enable_752x560) {
        srt.w = FB_W;
        srt.h = FB_H;
    }

    if (nds.overlay.sel >= nds.overlay.max) {
        if ((pre_sel != nds.theme.sel) || (pre_mode != nds.dis_mode)) {
            pre_mode = nds.dis_mode;
            pre_sel = nds.theme.sel;

            if (nds.theme.img) {
                SDL_FreeSurface(nds.theme.img);
                nds.theme.img = NULL;
            }

            nds.theme.img = SDL_CreateRGBSurface(SDL_SWSURFACE, srt.w, srt.h, 32, 0, 0, 0, 0);
            if (nds.theme.img) {
                SDL_FillRect(nds.theme.img, &nds.theme.img->clip_rect, SDL_MapRGB(nds.theme.img->format, 0x00, 0x00, 0x00));

                if (get_dir_path(nds.theme.path, nds.theme.sel, buf) == 0) {
                    switch (nds.dis_mode) {
                    case NDS_LAYOUT_0:
                    case NDS_LAYOUT_1:
                        return 0;
                    case NDS_LAYOUT_2:
                        strcat(buf, "/bg_s0.png");
                        break;
                    case NDS_LAYOUT_3:
                        return 0;
                    case NDS_LAYOUT_4:
                        strcat(buf, "/bg_v0.png");
                        break;
                    case NDS_LAYOUT_5:
                        strcat(buf, "/bg_v1.png");
                        break;
                    case NDS_LAYOUT_6:
                        strcat(buf, "/bg_h0.png");
                        break;
                    case NDS_LAYOUT_7:
                        strcat(buf, "/bg_h1.png");
                        break;
                    case NDS_LAYOUT_8:
                        strcat(buf, "/bg_vh_s0.png");
                        break;
                    case NDS_LAYOUT_9:
                        strcat(buf, "/bg_vh_s1.png");
                        break;
                    case NDS_LAYOUT_16:
                        strcat(buf, "/bg_vh_s2.png");
                        break;
                    case NDS_LAYOUT_10:
                        strcat(buf, "/bg_vh_c0.png");
                        break;
                    case NDS_LAYOUT_11:
                        strcat(buf, "/bg_vh_c1.png");
                        break;
                    case NDS_LAYOUT_12:
                    case NDS_LAYOUT_13:
                        strcat(buf, "/bg_hh0.png");
                        break;
                    case NDS_LAYOUT_17:
                        strcat(buf, "/bg_hres0.png");
                        break;
                    case NDS_LAYOUT_18:
                        return 0;
                    }
                    
                    t = IMG_Load(buf);
                    if (t) {
                        SDL_BlitSurface(t, NULL, nds.theme.img, NULL);
                        SDL_FreeSurface(t);
#if !defined(A30)
                        GFX_Copy(-1, nds.theme.img->pixels, nds.theme.img->clip_rect, drt, nds.theme.img->pitch, 0, E_MI_GFX_ROTATE_180);
#endif
                    }
                    else {
                        printf(PREFIX"Failed to load wallpaper (%s)\n", buf);
                    }
                }
            }
        }
        else {
            if (nds.theme.img) {
#if !defined(A30)
                GFX_Copy(-1, nds.theme.img->pixels, nds.theme.img->clip_rect, drt, nds.theme.img->pitch, 0, E_MI_GFX_ROTATE_180);
#else
                glBindTexture(GL_TEXTURE_2D, vid.texID[TEX_BG]);
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, nds.theme.img->w, nds.theme.img->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nds.theme.img->pixels);
#endif
            }
        }
    }
    else {
        t = SDL_CreateRGBSurface(SDL_SWSURFACE, IMG_W, IMG_H, 32, 0, 0, 0, 0);
        if (t) {
            SDL_FillRect(t, &t->clip_rect, SDL_MapRGB(t->format, 0x00, 0x00, 0x00));
            GFX_Copy(-1, t->pixels, t->clip_rect, drt, t->pitch, 0, E_MI_GFX_ROTATE_180);
            SDL_FreeSurface(t);
        }
    }

    return 0;
}

int reload_overlay(void)
{
    static int pre_sel = -1;

    char buf[MAX_PATH] = {0};
    SDL_Surface *t = NULL;

    if ((nds.overlay.sel < nds.overlay.max) && (pre_sel != nds.overlay.sel)) {
        pre_sel = nds.overlay.sel;

        if (nds.overlay.img) {
            SDL_FreeSurface(nds.overlay.img);
            nds.overlay.img = NULL;
        }

        nds.overlay.img = SDL_CreateRGBSurface(SDL_SWSURFACE, FB_W, FB_H, 32, 0, 0, 0, 0);
        if (nds.overlay.img) {
            SDL_FillRect(nds.overlay.img, &nds.overlay.img->clip_rect, SDL_MapRGB(nds.overlay.img->format, 0x00, 0x00, 0x00));

            if (get_file_path(nds.overlay.path, nds.overlay.sel, buf, 1) == 0) {
                t = IMG_Load(buf);
                if (t) {
                    SDL_BlitSurface(t, NULL, nds.overlay.img, NULL);
                    SDL_FreeSurface(t);

#ifdef MINI
                    neon_memcpy(gfx.overlay.virAddr, nds.overlay.img->pixels, FB_W * FB_H * 4);
                    MI_SYS_FlushInvCache(gfx.overlay.virAddr, FB_W * FB_H * FB_BPP);
#endif
                }
                else {
                    printf(PREFIX"Failed to load overlay (%s)\n", buf);
                }
            }
        }
    }
    return 0;
}

static void MiyooDeleteDevice(SDL_VideoDevice *device)
{
    SDL_free(device);
}

int MiyooCreateWindow(_THIS, SDL_Window *window)
{
    vid.window = window;
    SDL_SetMouseFocus(window);
    SDL_SetKeyboardFocus(window);
    printf(PREFIX"Window:%p, Width:%d, Height:%d\n", window, window->w, window->h);
    return 0;
}

int MiyooCreateWindowFrom(_THIS, SDL_Window *window, const void *data)
{
    return SDL_Unsupported();
}

static SDL_VideoDevice *CreateDevice(int devindex)
{
    SDL_VideoDevice *device = NULL;

    device = (SDL_VideoDevice *)SDL_calloc(1, sizeof(SDL_VideoDevice));
    if(!device) {
        SDL_OutOfMemory();
        return NULL;
    }
    device->is_dummy = SDL_TRUE;

    device->VideoInit = MiyooVideoInit;
    device->VideoQuit = MiyooVideoQuit;
    device->SetDisplayMode = MiyooSetDisplayMode;
    device->CreateSDLWindow = MiyooCreateWindow;
    device->CreateSDLWindowFrom = MiyooCreateWindowFrom;
    device->free = MiyooDeleteDevice;
    device->PumpEvents = PumpEvents;

    unlink(LOG_FILE_NAME);
    init_config_settings();
    load_config_settings();
    return device;
}

VideoBootStrap MiyooVideo_bootstrap = {"Miyoo", "Miyoo Video Driver", CreateDevice};

int MiyooVideoInit(_THIS)
{
#ifdef MINI
    FILE *fd = NULL;
    char buf[MAX_PATH] = {0};
#endif

    SDL_DisplayMode mode = {0};
    SDL_VideoDisplay display = {0};

    printf(PREFIX"MiyooVideoInit\n");
#ifndef UT
    signal(SIGTERM, sigterm_handler);
#endif

    SDL_zero(mode);
    mode.format = SDL_PIXELFORMAT_RGB565;
    mode.w = 800;
    mode.h = 480;
    mode.refresh_rate = 60;
    SDL_AddDisplayMode(&display, &mode);

    SDL_zero(mode);
    mode.format = SDL_PIXELFORMAT_ARGB8888;
    mode.w = 800;
    mode.h = 480;
    mode.refresh_rate = 60;
    SDL_AddDisplayMode(&display, &mode);
    SDL_zero(mode);
    mode.format = SDL_PIXELFORMAT_RGB565;
    mode.w = 800;
    mode.h = 600;
    mode.refresh_rate = 60;
    SDL_AddDisplayMode(&display, &mode);

    SDL_zero(mode);
    mode.format = SDL_PIXELFORMAT_ARGB8888;
    mode.w = 800;
    mode.h = 600;
    mode.refresh_rate = 60;
    SDL_AddDisplayMode(&display, &mode);

    SDL_zero(mode);
    mode.format = SDL_PIXELFORMAT_RGB565;
    mode.w = 480;
    mode.h = 272;
    mode.refresh_rate = 60;
    SDL_AddDisplayMode(&display, &mode);

    SDL_zero(mode);
    mode.format = SDL_PIXELFORMAT_ARGB8888;
    mode.w = 480;
    mode.h = 272;
    mode.refresh_rate = 60;
    SDL_AddDisplayMode(&display, &mode);

    SDL_zero(mode);
    mode.format = SDL_PIXELFORMAT_RGB565;
    mode.w = 320;
    mode.h = 240;
    mode.refresh_rate = 60;
    SDL_AddDisplayMode(&display, &mode);

    SDL_zero(mode);
    mode.format = SDL_PIXELFORMAT_ARGB8888;
    mode.w = 320;
    mode.h = 240;
    mode.refresh_rate = 60;
    SDL_AddDisplayMode(&display, &mode);

    SDL_AddVideoDisplay(&display, SDL_FALSE);

    LINE_H = 30;
    FONT_SIZE = DEF_FONT_SIZE;

    FB_W = DEF_FB_W;
    FB_H = DEF_FB_H;
    FB_SIZE = FB_W * FB_H * FB_BPP * 2;
    TMP_SIZE = FB_W * FB_H * FB_BPP;

#ifdef MINI
    fd = popen("fbset | grep \"mode \"", "r");
    if (fd) {
        fgets(buf, sizeof(buf), fd);
        pclose(fd);

        if (strstr(buf, "752")) {
            FONT_SIZE = 27;
            LINE_H = FONT_SIZE + 8;

            FB_W = 752;
            FB_H = 560;
            FB_SIZE = (FB_W * FB_H * FB_BPP * 2);
            TMP_SIZE = (FB_W * FB_H * FB_BPP);
            nds.enable_752x560 = 1;
        }
    }
#endif

    GFX_Init();
    read_config();
    EventInit();

    set_page_size(sysconf(_SC_PAGESIZE));
    add_save_load_state_handler(nds.states.path);
    printf(PREFIX"Installed hooking for drastic functions\n");

//    detour_hook(FUN_PRINT_STRING, (intptr_t)sdl_print_string);
//    detour_hook(FUN_SAVESTATE_PRE, (intptr_t)sdl_savestate_pre);
//    detour_hook(FUN_SAVESTATE_POST, (intptr_t)sdl_savestate_post);
//    detour_hook(FUN_BLIT_SCREEN_MENU, (intptr_t)sdl_blit_screen_menu);
//    detour_hook(FUN_UPDATE_SCREEN, (intptr_t)sdl_update_screen);
#ifndef UT
    detour_hook(FUN_RENDER_POLYGON_SETUP_PERSPECTIVE_STEPS, (intptr_t)render_polygon_setup_perspective_steps);
#endif

    printf(PREFIX"Installed hooking for libc functions\n");
//    detour_hook(FUN_MALLOC, (intptr_t)sdl_malloc);
//    detour_hook(FUN_REALLOC, (intptr_t)sdl_realloc);
//    detour_hook(FUN_FREE, (intptr_t)sdl_free);
    return 0;
}

static int MiyooSetDisplayMode(_THIS, SDL_VideoDisplay *display, SDL_DisplayMode *mode)
{
    return 0;
}

void MiyooVideoQuit(_THIS)
{
    printf(PREFIX"MiyooVideoQuit\n");
    printf(PREFIX"Wait for savestate complete\n");
    while (savestate_busy) {
        usleep(1000000);
    }
    if (system("sync") < 0) {
        printf("Failed to do sync command\n");
    }
    restore_detour_hook();
    write_config();

    if (fps_info) {
        SDL_FreeSurface(fps_info);
        fps_info = NULL;
    }

    if (nds.pen.img) {
        SDL_FreeSurface(nds.pen.img);
        nds.pen.img = NULL;
    }

    if (nds.theme.img) {
        SDL_FreeSurface(nds.theme.img);
        nds.theme.img = NULL;
    }

    if (nds.overlay.img) {
        SDL_FreeSurface(nds.overlay.img);
        nds.overlay.img = NULL;
    }

    if (nds.menu.bg) {
        SDL_FreeSurface(nds.menu.bg);
        nds.menu.bg = NULL;
    }

    if (nds.menu.cursor) {
        SDL_FreeSurface(nds.menu.cursor);
        nds.menu.cursor = NULL;
    }

    if (nds.menu.drastic.bg0) {
        SDL_FreeSurface(nds.menu.drastic.bg0);
        nds.menu.drastic.bg0 = NULL;
    }

    if (nds.menu.drastic.bg1) {
        SDL_FreeSurface(nds.menu.drastic.bg1);
        nds.menu.drastic.bg1 = NULL;
    }

    if (nds.menu.drastic.cursor) {
        SDL_FreeSurface(nds.menu.drastic.cursor);
        nds.menu.drastic.cursor = NULL;
    }

    if (nds.menu.drastic.main) {
        SDL_FreeSurface(nds.menu.drastic.main);
        nds.menu.drastic.main = NULL;
    }

    if (nds.menu.drastic.yes) {
        SDL_FreeSurface(nds.menu.drastic.yes);
        nds.menu.drastic.yes = NULL;
    }

    if (nds.menu.drastic.no) {
        SDL_FreeSurface(nds.menu.drastic.no);
        nds.menu.drastic.no = NULL;
    }

    if (nds.font) {
        TTF_CloseFont(nds.font);
        nds.font = NULL;
    }

    printf(PREFIX"Free TTF resources\n");
    TTF_Quit();

    printf(PREFIX"Free GFX resources\n");
    GFX_Quit();

    printf(PREFIX"Free Event resources\n");
    EventDeinit();

    printf(PREFIX"Free Lang resources\n");
    lang_unload();
}

static const char *DIS_MODE0_640[] = {
    "640*480",
    "640*480",
    "512*384",
    "640*480",
    "256*192",
    "320*240",
    "256*192",
    "320*240",
    "480*360",
    "384*288",
    "384*288",
    "384*288",
    "384*288",
    "427*320",
    "427*320",
    "480*320",
    "480*320",
};

static const char *DIS_MODE1_640[] = {
    "170*128",
    "256*192",
    "",
    "",
    "256*192",
    "320*240",
    "256*192",
    "320*240",
    "160*120",
    "256*192",
    "256*192",
    "256*192",
    "256*192",
    "427*320",
    "427*320",
    "480*320",
    "480*320",
};

static const char *DIS_MODE0_752[] = {
    "752*560",
    "752*560",
    "512*384",
    "752*560",
    "256*192",
    "373*280",
    "256*192",
    "373*280",
    "592*440",
    "496*368",
    "496*368",
    "496*368",
    "496*368",
    "501*376",
    "501*376",
    "560*376",
    "560*376",
};

static const char *DIS_MODE1_752[] = {
    "170*128",
    "256*192",
    "",
    "",
    "256*192",
    "373*280",
    "256*192",
    "373*280",
    "160*120",
    "256*192",
    "256*192",
    "256*192",
    "256*192",
    "501*376",
    "501*376",
    "560*376",
    "560*376",
};

static const char *POS[] = {
    "Top-Right", "Top-Left", "Bottom-Left", "Bottom-Right"
};

static const char *BORDER[] = {
    "None", "White", "Red", "Green", "Blue", "Black", "Yellow", "Cyan"
};

static const char *DPAD[] = {
    "0°", "90°", "270°"
};

static const char *HOTKEY[] = {
    "MENU", "SELECT"
};

#ifdef A30
static const char *JOY_MODE[] = {
    "Disable", "D-Pad", "Stylus", "Customized Key"
};

static const char *JOY_CUSKEY[] = {
    "UP", "DOWN", "LEFT", "RIGHT", "A", "B", "X", "Y", "L1", "R1", "L2", "R2", "SELECT", "START"
};
#endif

static int lang_next(void)
{
    int cc = 0;

    for (cc=1; cc<(MAX_LANG_FILE-1); cc++) {
        if (!strcmp(nds.lang.trans[DEF_LANG_SLOT], nds.lang.trans[cc])) {
            if (strcmp(nds.lang.trans[cc + 1], "")) {
                strcpy(nds.lang.trans[DEF_LANG_SLOT], nds.lang.trans[cc + 1]);
                return 0;
            }
        }
    }
    return -1;
}

static int lang_prev(void)
{
    int cc = 0;

    for (cc=(MAX_LANG_FILE-1); cc>1; cc--) {
        if (!strcmp(nds.lang.trans[DEF_LANG_SLOT], nds.lang.trans[cc])) {
            if (strcmp(nds.lang.trans[cc - 1], "")) {
                strcpy(nds.lang.trans[DEF_LANG_SLOT], nds.lang.trans[cc - 1]);
                return 0;
            }
        }
    }
    return -1;
}

enum {
    MENU_LANG = 0,
#if defined(A30)
    MENU_CPU_CORE,
#ifdef A30
    MENU_CPU_CLOCK,
#endif
#else
    MENU_CPU,
#endif
    MENU_OVERLAY,
    MENU_DIS,
    MENU_DIS_ALPHA,
    MENU_DIS_BORDER,
    MENU_DIS_POSITION,
    MENU_ALT,
    MENU_KEYS,
    MENU_HOTKEY,
    MENU_SWAP_L1L2,
    MENU_SWAP_R1R2,
    MENU_PEN_XV,
    MENU_PEN_YV,
    MENU_CURSOR,
    MENU_FAST_FORWARD,
#ifdef A30
    MENU_JOY_MODE,
    MENU_JOY_CUSKEY0,
    MENU_JOY_CUSKEY1,
    MENU_JOY_CUSKEY2,
    MENU_JOY_CUSKEY3,
    MENU_JOY_DZONE,
#endif

    MENU_CHK_BAT,
    MENU_LAST,
};

static const char *MENU_ITEM[] = {
    "Language",
#if defined(A30)
    "CPU Core",
#ifdef A30
    "CPU Clock",
#endif
#else
    "CPU",
#endif
    "Overlay",
    "Display",
    "Alpha",
    "Border",
    "Position",
    "Alt. Display",
    "Keys",
    "Hotkey",
    "Swap L1-L2",
    "Swap R1-R2",
    "Pen X Speed",
    "Pen Y Speed",
    "Cursor",
    "Fast Forward",
#ifdef A30
    "Joy Mode",
    "  Joy Up",
    "  Joy Down",
    "  Joy Left",
    "  Joy Right",
    "Joy Dead Zone",
#endif
    "Check Battery",
};

int handle_menu(int key)
{
    static int pre_ff = 0;
    static int cur_sel = 0;
#if defined(A30)
    static uint32_t cur_cpucore = INIT_CPU_CORE;
    static uint32_t pre_cpucore = INIT_CPU_CORE;
#ifdef A30
    static uint32_t cur_cpuclock = INIT_CPU_CLOCK;
    static uint32_t pre_cpuclock = INIT_CPU_CLOCK;
#endif
#else
    static uint32_t cur_cpuclock = 0;
    static uint32_t pre_cpuclock = 0;
#endif
    static char pre_lang[LANG_FILE_LEN] = {0};

    const int SX = nds.enable_752x560 ? 200 : 150;
    const int SY = nds.enable_752x560 ? 120 : 107;
    const int SSX = nds.enable_752x560 ? 410 : 385;

    int cc = 0;
    int sx = 0;
    int sy = 0;
    int s0 = 0;
    int s1 = 0;
    int idx = 0;
    int h = LINE_H;
    int dis_mode = -1;
    SDL_Rect rt = {0};
    char buf[MAX_PATH] = {0};
    uint32_t sel_col = nds.menu.c0;
    uint32_t unsel_col = nds.menu.c1;
    uint32_t dis_col = 0x808080;
    uint32_t val_col = nds.menu.c0;
    uint32_t col0 = 0, col1 = 0;

    if (pre_lang[0] == 0) {
        strcpy(pre_lang, nds.lang.trans[DEF_LANG_SLOT]);
    }

    if (pre_cpuclock == 0) {
#ifdef MINI
        cur_cpuclock = pre_cpuclock = get_cpuclock();
#endif
    }

    switch (key) {
    case KEY_BIT_UP:
        if (cur_sel > 0) {
            cur_sel-= 1;
        }
        break;
    case KEY_BIT_DOWN:
        if (cur_sel < (MENU_LAST - 1)) {
            cur_sel+= 1;
        }
        break;
    case KEY_BIT_LEFT:
        switch(cur_sel) {
        case MENU_LANG:
            lang_prev();
            break;
#if defined(A30)
        case MENU_CPU_CORE:
            if (cur_cpucore > nds.mincore) {
                cur_cpucore-= 1;
            }
            break;
#ifdef A30
        case MENU_CPU_CLOCK:
            if (cur_cpuclock > nds.mincpu) {
                cur_cpuclock-= 50;
            }
            break;
#endif
#else
        case MENU_CPU:
            if (cur_cpuclock > nds.mincpu) {
                cur_cpuclock-= 50;
            }
            break;
#endif
        case MENU_HOTKEY:
            nds.hotkey = HOTKEY_BIND_MENU;
            break;
        case MENU_SWAP_L1L2:
            nds.swap_l1l2 = 0;
            break;
        case MENU_SWAP_R1R2:
            nds.swap_r1r2 = 0;
            break;
        case MENU_OVERLAY:
            if (nds.overlay.sel < nds.overlay.max) {
                nds.overlay.sel+= 1;
            }
            break;
        case MENU_DIS:
            if (nds.hres_mode == 0) {
                if (nds.dis_mode > 0) {
                    nds.dis_mode-= 1;
                }
            }
            else {
                nds.dis_mode = NDS_LAYOUT_17;
            }
            break;
        case MENU_DIS_ALPHA:
            if (((nds.dis_mode == NDS_LAYOUT_0) || (nds.dis_mode == NDS_LAYOUT_1))) {
                if (nds.alpha.val > 0) {
                    nds.alpha.val-= 1;
                }
            }
            break;
        case MENU_DIS_BORDER:
            if ((nds.alpha.val > 0) && ((nds.dis_mode == NDS_LAYOUT_0) || (nds.dis_mode == NDS_LAYOUT_1))) {
                if (nds.alpha.border > 0) {
                    nds.alpha.border-= 1;
                }
            }
            break;
        case MENU_DIS_POSITION:
            if (((nds.dis_mode == NDS_LAYOUT_0) || (nds.dis_mode == NDS_LAYOUT_1))) {
                if (nds.alpha.pos > 0) {
                    nds.alpha.pos-= 1;
                }
            }
            break;
        case MENU_ALT:
            if ((nds.hres_mode == 0) && (nds.alt_mode > 0)) {
                nds.alt_mode-= 1;
            }
            break;
        case MENU_KEYS:
            if (nds.keys_rotate > 0) {
                nds.keys_rotate-= 1;
            }
            break;
        case MENU_PEN_XV:
            if (nds.pen.xv > PEN_XV_MIN) {
                nds.pen.xv-= PEN_XV_DEC;
            }
            break;
        case MENU_PEN_YV:
            if (nds.pen.yv > PEN_YV_MIN) {
                nds.pen.yv-= PEN_YV_DEC;
            }
            break;
        case MENU_CURSOR:
            nds.menu.show_cursor = 0;
            break;
        case MENU_FAST_FORWARD:
            if (nds.fast_forward > 1) {
                nds.fast_forward -= 1;
            }
            break;
#ifdef A30
        case MENU_JOY_MODE:
            if (nds.joy.mode > 0) {
                nds.joy.mode -= 1;
            }
            break;
        case MENU_JOY_CUSKEY0:
            if (nds.joy.mode == MYJOY_MODE_CUSKEY) {
                if (nds.joy.cuskey[0] > 0) {
                    nds.joy.cuskey[0] -= 1;
                }
            }
            break;
        case MENU_JOY_CUSKEY1:
            if (nds.joy.mode == MYJOY_MODE_CUSKEY) {
                if (nds.joy.cuskey[1] > 0) {
                    nds.joy.cuskey[1] -= 1;
                }
            }
            break;
        case MENU_JOY_CUSKEY2:
            if (nds.joy.mode == MYJOY_MODE_CUSKEY) {
                if (nds.joy.cuskey[2] > 0) {
                    nds.joy.cuskey[2] -= 1;
                }
            }
            break;
        case MENU_JOY_CUSKEY3:
            if (nds.joy.mode == MYJOY_MODE_CUSKEY) {
                if (nds.joy.cuskey[3] > 0) {
                    nds.joy.cuskey[3] -= 1;
                }
            }
            break;
        case MENU_JOY_DZONE:
            if (nds.joy.dzone > 0) {
                nds.joy.dzone -= 1;
            }
            break;
#endif
        case MENU_CHK_BAT:
            nds.chk_bat = 0;
            break;
        default:
            break;
        }
        break;
    case KEY_BIT_RIGHT:
        switch(cur_sel) {
        case MENU_LANG:
            lang_next();
            break;
#if defined(A30)
        case MENU_CPU_CORE:
            if (cur_cpucore < nds.maxcore) {
                cur_cpucore+= 1;
            }
            break;
#ifdef A30
        case MENU_CPU_CLOCK:
            if (cur_cpuclock < nds.maxcpu) {
                cur_cpuclock+= 50;
            }
            break;
#endif
#else
        case MENU_CPU:
            if (cur_cpuclock < nds.maxcpu) {
                cur_cpuclock+= 50;
            }
            break;
#endif
        case MENU_HOTKEY:
            nds.hotkey = HOTKEY_BIND_SELECT;
            break;
        case MENU_SWAP_L1L2:
            nds.swap_l1l2 = 1;
            break;
        case MENU_SWAP_R1R2:
            nds.swap_r1r2 = 1;
            break;
        case MENU_OVERLAY:
            if (nds.overlay.sel > 0) {
                nds.overlay.sel-= 1;
            }
            break;
        case MENU_DIS:
            if (nds.hres_mode == 0) {
                if (nds.dis_mode < NDS_LAYOUT_LAST) {
                    nds.dis_mode+= 1;
                }
            }
            else {
                nds.dis_mode = NDS_LAYOUT_18;
            }
            break;
        case MENU_DIS_ALPHA:
            if (((nds.dis_mode == NDS_LAYOUT_0) || (nds.dis_mode == NDS_LAYOUT_1))) {
                if (nds.alpha.val < NDS_ALPHA_MAX) {
                    nds.alpha.val+= 1;
                }
            }
            break;
        case MENU_DIS_BORDER:
            if ((nds.alpha.val > 0) && ((nds.dis_mode == NDS_LAYOUT_0) || (nds.dis_mode == NDS_LAYOUT_1))) {
                if (nds.alpha.border < NDS_BORDER_MAX) {
                    nds.alpha.border+= 1;
                }
            }
            break;
        case MENU_DIS_POSITION:
            if (((nds.dis_mode == NDS_LAYOUT_0) || (nds.dis_mode == NDS_LAYOUT_1))) {
                if (nds.alpha.pos < 3) {
                    nds.alpha.pos+= 1;
                }
            }
            break;
        case MENU_ALT:
            if ((nds.hres_mode == 0) && (nds.alt_mode < NDS_LAYOUT_LAST)) {
                nds.alt_mode+= 1;
            }
            break;
        case MENU_KEYS:
            if (nds.keys_rotate < 2) {
                nds.keys_rotate+= 1;
            }
            break;
        case MENU_PEN_XV:
            if (nds.pen.xv < PEN_XV_MAX) {
                nds.pen.xv+= PEN_XV_INC;
            }
            break;
        case MENU_PEN_YV:
            if (nds.pen.yv < PEN_YV_MAX) {
                nds.pen.yv+= PEN_YV_INC;
            }
            break;
        case MENU_CURSOR:
            nds.menu.show_cursor = 1;
            break;
        case MENU_FAST_FORWARD:
            if (nds.fast_forward < 255) {
                nds.fast_forward += 1;
            }
            break;
#ifdef A30
        case MENU_JOY_MODE:
            if (nds.joy.mode < MYJOY_MODE_LAST) {
                nds.joy.mode += 1;
            }
            if (nds.joy.mode == MYJOY_MODE_STYLUS) {
                if (evt.mode == MMIYOO_MOUSE_MODE) {
                    evt.mode = MiyooKEYPAD_MODE;
                }
            }
            break;
        case MENU_JOY_CUSKEY0:
            if (nds.joy.mode == MYJOY_MODE_CUSKEY) {
                if (nds.joy.cuskey[0] < 13) {
                    nds.joy.cuskey[0] += 1;
                }
            }
            break;
        case MENU_JOY_CUSKEY1:
            if (nds.joy.mode == MYJOY_MODE_CUSKEY) {
                if (nds.joy.cuskey[1] < 13) {
                    nds.joy.cuskey[1] += 1;
                }
            }
            break;
        case MENU_JOY_CUSKEY2:
            if (nds.joy.mode == MYJOY_MODE_CUSKEY) {
                if (nds.joy.cuskey[2] < 13) {
                    nds.joy.cuskey[2] += 1;
                }
            }
            break;
        case MENU_JOY_CUSKEY3:
            if (nds.joy.mode == MYJOY_MODE_CUSKEY) {
                if (nds.joy.cuskey[3] < 13) {
                    nds.joy.cuskey[3] += 1;
                }
            }
            break;
        case MENU_JOY_DZONE:
            if (nds.joy.dzone < 255) {
                nds.joy.dzone += 1;
            }
            break;
#endif
        case MENU_CHK_BAT:
            nds.chk_bat = 1;
            break;
        default:
            break;
        }
        break;
    case KEY_BIT_B:
        if (cur_cpuclock != pre_cpuclock) {
#ifdef MINI
            set_cpuclock(cur_cpuclock);
#endif

#ifdef A30
            set_best_match_cpu_clock(cur_cpuclock);
#endif
            pre_cpuclock = cur_cpuclock;
        }

#if defined(A30)
        if (cur_cpucore != pre_cpucore) {
            set_core(cur_cpucore);
            pre_cpucore = cur_cpucore;
        }
#endif

        if (strcmp(pre_lang, nds.lang.trans[DEF_LANG_SLOT])) {
            lang_unload();
            lang_load(nds.lang.trans[DEF_LANG_SLOT]);
            memset(pre_lang, 0, sizeof(pre_lang));
        }

        if (pre_ff != nds.fast_forward) {
            set_fast_forward(nds.fast_forward);
            pre_ff = nds.fast_forward;
        }
        nds.menu.enable = 0;
        return 0;
    default:
        break;
    }

    if (cur_sel == MENU_DIS) {
        dis_mode = nds.dis_mode;
    }
    if (cur_sel == MENU_ALT) {
        dis_mode = nds.alt_mode;
    }
    SDL_SoftStretch(nds.menu.bg, NULL, cvt, NULL);

    if (cur_sel < 3) {
        s0 = 0;
        s1 = 7;
    }
    else if (cur_sel >= (MENU_LAST - 4)) {
        s0 = MENU_LAST - 8;
        s1 = MENU_LAST - 1;
    }
    else {
        s0 = cur_sel - 3;
        s1 = cur_sel + 4;
    }

    for (cc=0, idx=0; cc<MENU_LAST; cc++) {
        if ((cc < s0) || (cc > s1)) {
            continue;
        }

        sx = 0;
        col0 = (cur_sel == cc) ? sel_col : unsel_col;
        col1 = (cur_sel == cc) ? val_col : unsel_col;
        switch (cc) {
        case MENU_DIS_ALPHA:
            sx = 20;
            if ((cur_sel == MENU_DIS_ALPHA) && ((nds.dis_mode == NDS_LAYOUT_0) || (nds.dis_mode == NDS_LAYOUT_1))) {
                col1 = val_col;
            }
            else {
                if ((nds.dis_mode == NDS_LAYOUT_0) || (nds.dis_mode == NDS_LAYOUT_1)) {
                    col1 = unsel_col;
                }
                else {
                    col1 = dis_col;
                }
            }
            break;
        case MENU_DIS_BORDER:
            sx = 20;
            if ((cur_sel == MENU_DIS_BORDER) && (nds.alpha.val > 0) && ((nds.dis_mode == NDS_LAYOUT_0) || (nds.dis_mode == NDS_LAYOUT_1))) {
                col1 = val_col;
            }
            else {
                if ((nds.alpha.val > 0) && ((nds.dis_mode == NDS_LAYOUT_0) || (nds.dis_mode == NDS_LAYOUT_1))) {
                    col1 = unsel_col;
                }
                else {
                    col1 = dis_col;
                }
            }
            break;
        case MENU_DIS_POSITION:
            sx = 20;
            if ((cur_sel == MENU_DIS_POSITION) && ((nds.dis_mode == NDS_LAYOUT_0) || (nds.dis_mode == NDS_LAYOUT_1))) {
                col1 = val_col;
            }
            else {
                if ((nds.dis_mode == NDS_LAYOUT_0) || (nds.dis_mode == NDS_LAYOUT_1)) {
                    col1 = unsel_col;
                }
                else {
                    col1 = dis_col;
                }
            }
            break;
        case MENU_ALT:
            if (nds.hres_mode == 0) {
                col1 = (cur_sel == cc) ? val_col : unsel_col;
            }
            else {
                col1 = unsel_col;
            }
            break;
#ifdef A30
        case MENU_JOY_CUSKEY0:
        case MENU_JOY_CUSKEY1:
        case MENU_JOY_CUSKEY2:
        case MENU_JOY_CUSKEY3:
            if (nds.joy.mode == MYJOY_MODE_CUSKEY) {
                col1 = (cur_sel == cc) ? val_col : unsel_col;
            }
            else {
                col1 = dis_col;
            }
            break;
#endif
        default:
            break;
        }

        if (col0 == sel_col) {
            if (nds.menu.show_cursor) {
                rt.x = SX - (nds.enable_752x560 ? 43 : 10);
                rt.w = 461 + (nds.enable_752x560 ? 41 : -40);
            }
            else {
                rt.x = SX - (nds.enable_752x560 ? 83 : 50);
                rt.w = 461 + (nds.enable_752x560 ? 81 : 0);
            }
            rt.y = SY + (h * idx) - 2;
            rt.h = FONT_SIZE + 3;

            if ((cc == MENU_DIS_ALPHA) || (cc == MENU_KEYS)) {
                rt.w-= (nds.enable_752x560 ? 179 : 121);
            }
            if (col1 == dis_col) {
                col1 = val_col;
                SDL_FillRect(cvt, &rt, SDL_MapRGB(nds.menu.drastic.main->format, 0x80, 0x80, 0x80));
            }
            else {
                SDL_FillRect(cvt, &rt, SDL_MapRGB(nds.menu.drastic.main->format, (nds.menu.c2 >> 16) & 0xff, (nds.menu.c2 >> 8) & 0xff, nds.menu.c2 & 0xff));
            }

            if (nds.menu.show_cursor && nds.menu.cursor) {
                rt.x -= (nds.menu.cursor->w + 5);
                rt.y -= ((nds.menu.cursor->h - LINE_H) / 2);
                rt.w = 0;
                rt.h = 0;
                SDL_BlitSurface(nds.menu.cursor, NULL, cvt, &rt);
            }

            if ((cc == MENU_DIS) || (cc == MENU_ALT)) {
                rt.x = nds.enable_752x560 ? 480 : 440;
                rt.y = SY + (h * (idx + 1)) - 7;
                rt.w = nds.enable_752x560 ? 179 : 121;
                rt.h = FONT_SIZE + 8;
                SDL_FillRect(cvt, &rt, SDL_MapRGB(nds.menu.drastic.main->format, (nds.menu.c2 >> 16) & 0xff, (nds.menu.c2 >> 8) & 0xff, nds.menu.c2 & 0xff));
            }
        }
        draw_info(cvt, to_lang(MENU_ITEM[cc]), SX + sx, SY + (h * idx), col0, 0);

        sx = 0;
        switch (cc) {
        case MENU_LANG:
            snprintf(buf, sizeof(buf), "%s", nds.lang.trans[DEF_LANG_SLOT]);
            break;
#if defined(A30)
        case MENU_CPU_CORE:
            snprintf(buf, sizeof(buf), "%d", cur_cpucore);
            break;
#ifdef A30
        case MENU_CPU_CLOCK:
            snprintf(buf, sizeof(buf), "%dMHz", cur_cpuclock);
            break;
#endif
#else
        case MENU_CPU:
            snprintf(buf, sizeof(buf), "%dMHz", cur_cpuclock);
            break;
#endif
        case MENU_HOTKEY:
            snprintf(buf, sizeof(buf), "%s", to_lang(HOTKEY[nds.hotkey]));
            break;
        case MENU_SWAP_L1L2:
            snprintf(buf, sizeof(buf), "%s", to_lang(nds.swap_l1l2 ? "Yes" : "No"));
            break;
        case MENU_SWAP_R1R2:
            snprintf(buf, sizeof(buf), "%s", to_lang(nds.swap_r1r2 ? "Yes" : "No"));
            break;
        case MENU_OVERLAY:
            if (nds.overlay.sel < nds.overlay.max) {
                get_file_path(nds.overlay.path, nds.overlay.sel, buf, 0);
                reload_overlay();
            }
            else {
                snprintf(buf, sizeof(buf), "%s", to_lang("None"));
            }
            break;
        case MENU_DIS:
            if (nds.hres_mode == 0) {
                snprintf(buf, sizeof(buf), "[%d]   %s", nds.dis_mode, nds.enable_752x560 ? DIS_MODE0_752[nds.dis_mode] : DIS_MODE0_640[nds.dis_mode]);
            }
            else {
                if (nds.enable_752x560) {
                    snprintf(buf, sizeof(buf), "[%d]   %s", nds.dis_mode, nds.dis_mode == NDS_LAYOUT_17 ? "512*384" : "752*560");
                }
                else {
                    snprintf(buf, sizeof(buf), "[%d]   %s", nds.dis_mode, nds.dis_mode == NDS_LAYOUT_17 ? "512*384" : "640*480");
                }
            }
            break;
        case MENU_DIS_ALPHA:
            if (nds.hres_mode == 0) {
                snprintf(buf, sizeof(buf), "[%d]   ", nds.dis_mode);
                sx = get_font_width(buf);
                snprintf(buf, sizeof(buf), "%s", nds.enable_752x560 ? DIS_MODE1_752[nds.dis_mode] : DIS_MODE1_640[nds.dis_mode]);
                draw_info(cvt, buf, SSX + sx, SY + (h * idx), (cur_sel == MENU_DIS) ? nds.menu.c0 : nds.menu.c1, 0);
            }

            sx = 0;
            snprintf(buf, sizeof(buf), "%d", nds.alpha.val);
            break;
        case MENU_DIS_BORDER:
            snprintf(buf, sizeof(buf), "%s", to_lang(BORDER[nds.alpha.border]));
            break;
        case MENU_DIS_POSITION:
            snprintf(buf, sizeof(buf), "%s", to_lang(POS[nds.alpha.pos]));
            break;
        case MENU_ALT:
            snprintf(buf, sizeof(buf), "[%d]   %s", nds.alt_mode, nds.enable_752x560 ? DIS_MODE0_752[nds.alt_mode] : DIS_MODE0_640[nds.alt_mode]);
            break;
        case MENU_KEYS:
            snprintf(buf, sizeof(buf), "[%d]   ", nds.alt_mode);
            sx = get_font_width(buf);
            snprintf(buf, sizeof(buf), "%s", nds.enable_752x560 ? DIS_MODE1_752[nds.alt_mode] : DIS_MODE1_640[nds.alt_mode]);
            draw_info(cvt, buf, SSX + sx, SY + (h * idx), (nds.hres_mode == 0) && (cur_sel == MENU_ALT) ? val_col : unsel_col, 0);

            sx = 0;
            snprintf(buf, sizeof(buf), "%s", DPAD[nds.keys_rotate % 3]);
            break;
        case MENU_PEN_XV:
#ifdef A30
            snprintf(buf, sizeof(buf), "%d (80000)", nds.pen.xv);
#else
            snprintf(buf, sizeof(buf), "%d (30000)", nds.pen.xv);
#endif
            break;
        case MENU_PEN_YV:
#ifdef A30
            snprintf(buf, sizeof(buf), "%d (85000)", nds.pen.yv);
#else
            snprintf(buf, sizeof(buf), "%d (35000)", nds.pen.yv);
#endif
            break;
        case MENU_CURSOR:
            snprintf(buf, sizeof(buf), "%s", to_lang(nds.menu.show_cursor ? "Show" : "Hide"));
            break;
        case MENU_FAST_FORWARD:
            snprintf(buf, sizeof(buf), "%d (6)", nds.fast_forward);
            break;
#ifdef A30
        case MENU_JOY_MODE:
            snprintf(buf, sizeof(buf), "%s", to_lang(JOY_MODE[nds.joy.mode]));
            break;
        case MENU_JOY_CUSKEY0:
            snprintf(buf, sizeof(buf), "Miyoo %s", to_lang(JOY_CUSKEY[nds.joy.cuskey[0]]));
            break;
        case MENU_JOY_CUSKEY1:
            snprintf(buf, sizeof(buf), "Miyoo %s", to_lang(JOY_CUSKEY[nds.joy.cuskey[1]]));
            break;
        case MENU_JOY_CUSKEY2:
            snprintf(buf, sizeof(buf), "Miyoo %s", to_lang(JOY_CUSKEY[nds.joy.cuskey[2]]));
            break;
        case MENU_JOY_CUSKEY3:
            snprintf(buf, sizeof(buf), "Miyoo %s", to_lang(JOY_CUSKEY[nds.joy.cuskey[3]]));
            break;
        case MENU_JOY_DZONE:
            snprintf(buf, sizeof(buf), "%d (65)", nds.joy.dzone);
            break;
#endif
        case MENU_CHK_BAT:
            snprintf(buf, sizeof(buf), "%s", to_lang(nds.chk_bat ? "Yes" : "No"));
            break;
        }
        draw_info(cvt, buf, SSX + sx, SY + (h * idx), col1, 0);
        idx+= 1;
    }

    sx = nds.enable_752x560 ? 540 : 450;
    sy = nds.enable_752x560 ? 430 : 360;
    if ((cur_sel == MENU_OVERLAY) && (nds.overlay.sel < nds.overlay.max) && (nds.overlay.img)) {
        rt.x = sx;
        rt.y = sy;
        rt.w = 128;
        rt.h = 96;
        SDL_SoftStretch(nds.overlay.img, NULL, cvt, &rt);
    }
    else if(dis_mode >= 0) {
        switch (dis_mode) {
        case NDS_LAYOUT_0:
            rt.x = sx;
            rt.y = sy;
            rt.w = 128;
            rt.h = 96;
            SDL_FillRect(cvt, &rt, SDL_MapRGB(cvt->format, 0x80, 0x00, 0x00));
           
            rt.w = 34;
            rt.h = 26;
            switch (nds.alpha.pos) {
            case 0:
                rt.x = (sx + 128) - rt.w;
                rt.y = sy;
                break;
            case 1:
                rt.x = sx;
                rt.y = sy;
                break;
            case 2:
                rt.x = sx;
                rt.y = (sy + 96) - rt.h;
                break;
            case 3:
                rt.x = (sx + 128) - rt.w;
                rt.y = (sy + 96) - rt.h;
                break;
            }
            SDL_FillRect(cvt, &rt, SDL_MapRGB(cvt->format, 0x00, 0x00, (30 * nds.alpha.val)));
            break;
        case NDS_LAYOUT_1:
            rt.x = sx;
            rt.y = sy;
            rt.w = 128;
            rt.h = 96;
            SDL_FillRect(cvt, &rt, SDL_MapRGB(cvt->format, 0x80, 0x00, 0x00));
           
            rt.w = 51;
            rt.h = 38;
            switch (nds.alpha.pos) {
            case 0:
                rt.x = (sx + 128) - rt.w;
                rt.y = sy;
                break;
            case 1:
                rt.x = sx;
                rt.y = sy;
                break;
            case 2:
                rt.x = sx;
                rt.y = (sy + 96) - rt.h;
                break;
            case 3:
                rt.x = (sx + 128) - rt.w;
                rt.y = (sy + 96) - rt.h;
                break;
            }
            SDL_FillRect(cvt, &rt, SDL_MapRGB(cvt->format, 0x00, 0x00, (30 * nds.alpha.val)));
            break;
        case NDS_LAYOUT_2:
            rt.x = sx;
            rt.y = sy;
            rt.w = 128;
            rt.h = 96;
            SDL_FillRect(cvt, &rt, SDL_MapRGB(cvt->format, 0x00, 0x80, 0x00));
            
            rt.w = 102;
            rt.h = 76;
            rt.x = sx + ((128 - rt.w) / 2);
            rt.y = sy + ((96 - rt.h) / 2);
            SDL_FillRect(cvt, &rt, SDL_MapRGB(cvt->format, 0x80, 0x00, 0x00));
            break;
        case NDS_LAYOUT_3:
            rt.x = sx;
            rt.y = sy;
            rt.w = 128;
            rt.h = 96;
            SDL_FillRect(cvt, &rt, SDL_MapRGB(cvt->format, 0x80, 0x00, 0x00));
            break;
        case NDS_LAYOUT_4:
            rt.x = sx;
            rt.y = sy;
            rt.w = 128;
            rt.h = 96;
            SDL_FillRect(cvt, &rt, SDL_MapRGB(cvt->format, 0x00, 0x80, 0x00));
            
            rt.w = 51;
            rt.h = 38;
            rt.x = sx + ((128 - rt.w) / 2);
            rt.y = sy + ((96 - (rt.h * 2)) / 2);
            SDL_FillRect(cvt, &rt, SDL_MapRGB(cvt->format, 0x80, 0x00, 0x00));
            
            rt.w = 51;
            rt.h = 38;
            rt.x = sx + ((128 - rt.w) / 2);
            rt.y = sy + ((96 - (rt.h * 2)) / 2) + rt.h;
            SDL_FillRect(cvt, &rt, SDL_MapRGB(cvt->format, 0x00, 0x00, 0x80));
            break;
        case NDS_LAYOUT_5:
            rt.x = sx;
            rt.y = sy;
            rt.w = 128;
            rt.h = 96;
            SDL_FillRect(cvt, &rt, SDL_MapRGB(cvt->format, 0x00, 0x80, 0x00));
            
            rt.w = 64;
            rt.h = 48;
            rt.x = sx + ((128 - rt.w) / 2);
            rt.y = sy + ((96 - (rt.h * 2)) / 2);
            SDL_FillRect(cvt, &rt, SDL_MapRGB(cvt->format, 0x80, 0x00, 0x00));
            
            rt.w = 64;
            rt.h = 48;
            rt.x = sx + ((128 - rt.w) / 2);
            rt.y = sy + ((96 - (rt.h * 2)) / 2) + rt.h;
            SDL_FillRect(cvt, &rt, SDL_MapRGB(cvt->format, 0x00, 0x00, 0x80));
            break;
        case NDS_LAYOUT_6:
            rt.x = sx;
            rt.y = sy;
            rt.w = 128;
            rt.h = 96;
            SDL_FillRect(cvt, &rt, SDL_MapRGB(cvt->format, 0x00, 0x80, 0x00));
            
            rt.w = 51;
            rt.h = 38;
            rt.x = sx + ((128 - (rt.w * 2)) / 2);
            rt.y = sy + ((96 - rt.h) / 2);
            SDL_FillRect(cvt, &rt, SDL_MapRGB(cvt->format, 0x80, 0x00, 0x00));
            
            rt.w = 51;
            rt.h = 38;
            rt.x = sx + ((128 - (rt.w * 2)) / 2) + rt.w;
            rt.y = sy + ((96 - rt.h) / 2);
            SDL_FillRect(cvt, &rt, SDL_MapRGB(cvt->format, 0x00, 0x00, 0x80));
            break;
        case NDS_LAYOUT_7:
            rt.x = sx;
            rt.y = sy;
            rt.w = 128;
            rt.h = 96;
            SDL_FillRect(cvt, &rt, SDL_MapRGB(cvt->format, 0x00, 0x80, 0x00));
            
            rt.w = 64;
            rt.h = 48;
            rt.x = sx + ((128 - (rt.w * 2)) / 2);
            rt.y = sy + ((96 - rt.h) / 2);
            SDL_FillRect(cvt, &rt, SDL_MapRGB(cvt->format, 0x80, 0x00, 0x00));
            
            rt.w = 64;
            rt.h = 48;
            rt.x = sx + ((128 - (rt.w * 2)) / 2) + rt.w;
            rt.y = sy + ((96 - rt.h) / 2);
            SDL_FillRect(cvt, &rt, SDL_MapRGB(cvt->format, 0x00, 0x00, 0x80));
            break;
        case NDS_LAYOUT_8:
            rt.x = sx;
            rt.y = sy;
            rt.w = 128;
            rt.h = 96;
            SDL_FillRect(cvt, &rt, SDL_MapRGB(cvt->format, 0x00, 0x80, 0x00));
            
            rt.w = 96;
            rt.h = 72;
            rt.x = sx;
            rt.y = sy;
            SDL_FillRect(cvt, &rt, SDL_MapRGB(cvt->format, 0x80, 0x00, 0x00));
            
            rt.w = 32;
            rt.h = 24;
            rt.x = sx + (128 - rt.w);
            rt.y = sy + (96 - rt.h);
            SDL_FillRect(cvt, &rt, SDL_MapRGB(cvt->format, 0x00, 0x00, 0x80));
            break;
        case NDS_LAYOUT_9:
            rt.x = sx;
            rt.y = sy;
            rt.w = 128;
            rt.h = 96;
            SDL_FillRect(cvt, &rt, SDL_MapRGB(cvt->format, 0x00, 0x80, 0x00));
            
            rt.w = 77;
            rt.h = 58;
            rt.x = sx;
            rt.y = sy;
            SDL_FillRect(cvt, &rt, SDL_MapRGB(cvt->format, 0x80, 0x00, 0x00));
            
            rt.w = 51;
            rt.h = 38;
            rt.x = sx + (128 - rt.w);
            rt.y = sy + (96 - rt.h);
            SDL_FillRect(cvt, &rt, SDL_MapRGB(cvt->format, 0x00, 0x00, 0x80));
            break;
        case NDS_LAYOUT_16:
            rt.x = sx;
            rt.y = sy;
            rt.w = 128;
            rt.h = 96;
            SDL_FillRect(cvt, &rt, SDL_MapRGB(cvt->format, 0x00, 0x80, 0x00));
            
            rt.w = 96;
            rt.h = 72;
            rt.x = sx + ((128 - rt.w) / 2);
            rt.y = sy;
            SDL_FillRect(cvt, &rt, SDL_MapRGB(cvt->format, 0x80, 0x00, 0x00));
            
            rt.w = 32;
            rt.h = 24;
            rt.x = sx + ((128 - rt.w) / 2);
            rt.y = sy + (96 - rt.h);
            SDL_FillRect(cvt, &rt, SDL_MapRGB(cvt->format, 0x00, 0x00, 0x80));
            break;
        case NDS_LAYOUT_10:
            rt.x = sx;
            rt.y = sy;
            rt.w = 128;
            rt.h = 96;
            SDL_FillRect(cvt, &rt, SDL_MapRGB(cvt->format, 0x00, 0x80, 0x00));
            
            rt.w = 77;
            rt.h = 58;
            rt.x = sx + ((128 - rt.w) / 2);
            rt.y = sy;
            SDL_FillRect(cvt, &rt, SDL_MapRGB(cvt->format, 0x80, 0x00, 0x00));
            
            rt.w = 51;
            rt.h = 38;
            rt.x = sx + ((128 - rt.w) / 2);
            rt.y = sy + (96 - rt.h);
            SDL_FillRect(cvt, &rt, SDL_MapRGB(cvt->format, 0x00, 0x00, 0x80));
            break;
        case NDS_LAYOUT_11:
            rt.x = sx;
            rt.y = sy;
            rt.w = 128;
            rt.h = 96;
            SDL_FillRect(cvt, &rt, SDL_MapRGB(cvt->format, 0x00, 0x80, 0x00));
            
            rt.w = 77;
            rt.h = 58;
            rt.x = sx;
            rt.y = sy + ((96 - rt.h) / 2);
            SDL_FillRect(cvt, &rt, SDL_MapRGB(cvt->format, 0x80, 0x00, 0x00));
            
            rt.w = 51;
            rt.h = 38;
            rt.x = sx + (128 - rt.w);
            rt.y = sy + ((96 - rt.h) / 2);
            SDL_FillRect(cvt, &rt, SDL_MapRGB(cvt->format, 0x00, 0x00, 0x80));
            break;
        case NDS_LAYOUT_12:
            rt.x = sx;
            rt.y = sy;
            rt.w = 128;
            rt.h = 96;
            SDL_FillRect(cvt, &rt, SDL_MapRGB(cvt->format, 0x00, 0x80, 0x00));
            
            rt.w = 64;
            rt.h = 85;
            rt.x = sx;
            rt.y = sy + ((96 - rt.h) / 2);
            SDL_FillRect(cvt, &rt, SDL_MapRGB(cvt->format, 0x80, 0x00, 0x00));
            
            rt.w = 64;
            rt.h = 85;
            rt.x = sx + (128 - rt.w);
            rt.y = sy + ((96 - rt.h) / 2);
            SDL_FillRect(cvt, &rt, SDL_MapRGB(cvt->format, 0x00, 0x00, 0x80));
            break;
        case NDS_LAYOUT_13:
            rt.x = sx;
            rt.y = sy;
            rt.w = 128;
            rt.h = 96;
            SDL_FillRect(cvt, &rt, SDL_MapRGB(cvt->format, 0x00, 0x80, 0x00));

            rt.w = 64;
            rt.h = 85;
            rt.x = sx;
            rt.y = sy + ((96 - rt.h) / 2);
            SDL_FillRect(cvt, &rt, SDL_MapRGB(cvt->format, 0x00, 0x00, 0x80));

            rt.w = 64;
            rt.h = 85;
            rt.x = sx + (128 - rt.w);
            rt.y = sy + ((96 - rt.h) / 2);
            SDL_FillRect(cvt, &rt, SDL_MapRGB(cvt->format, 0x80, 0x00, 0x00));
            break;
        case NDS_LAYOUT_14:
            rt.x = sx;
            rt.y = sy;
            rt.w = 128;
            rt.h = 96;
            SDL_FillRect(cvt, &rt, SDL_MapRGB(cvt->format, 0x00, 0x80, 0x00));

            rt.w = 64;
            rt.h = 96;
            rt.x = sx;
            rt.y = sy;
            SDL_FillRect(cvt, &rt, SDL_MapRGB(cvt->format, 0x80, 0x00, 0x00));

            rt.w = 64;
            rt.h = 96;
            rt.x = sx + (128 - rt.w);
            rt.y = sy;
            SDL_FillRect(cvt, &rt, SDL_MapRGB(cvt->format, 0x00, 0x00, 0x80));
            break;
        case NDS_LAYOUT_15:
            rt.x = sx;
            rt.y = sy;
            rt.w = 128;
            rt.h = 96;
            SDL_FillRect(cvt, &rt, SDL_MapRGB(cvt->format, 0x00, 0x80, 0x00));

            rt.w = 64;
            rt.h = 96;
            rt.x = sx;
            rt.y = sy;
            SDL_FillRect(cvt, &rt, SDL_MapRGB(cvt->format, 0x00, 0x00, 0x80));

            rt.w = 64;
            rt.h = 96;
            rt.x = sx + (128 - rt.w);
            rt.y = sy;
            SDL_FillRect(cvt, &rt, SDL_MapRGB(cvt->format, 0x80, 0x00, 0x00));
            break;
        case NDS_LAYOUT_17:
            rt.x = sx;
            rt.y = sy;
            rt.w = 128;
            rt.h = 96;
            SDL_FillRect(cvt, &rt, SDL_MapRGB(cvt->format, 0x00, 0x80, 0x00));
            
            rt.w = 102;
            rt.h = 76;
            rt.x = sx + ((128 - rt.w) / 2);
            rt.y = sy + ((96 - rt.h) / 2);
            SDL_FillRect(cvt, &rt, SDL_MapRGB(cvt->format, 0x80, 0x00, 0x00));
            break;
        case NDS_LAYOUT_18:
            rt.x = sx;
            rt.y = sy;
            rt.w = 128;
            rt.h = 96;
            SDL_FillRect(cvt, &rt, SDL_MapRGB(cvt->format, 0x80, 0x00, 0x00));
            break;
        }
    }

#if defined(A30)
    nds.update_menu = 1;
#else
    GFX_Copy(-1, cvt->pixels, cvt->clip_rect, cvt->clip_rect, cvt->pitch, 0, E_MI_GFX_ROTATE_180);
    GFX_Flip();
#endif
    need_reload_bg = RELOAD_BG_COUNT;
    return 0;
}

#ifdef UT
TEST_GROUP_RUNNER(sdl2_video_miyoo)
{
    RUN_TEST_CASE(sdl2_video_miyoo, get_current_menu_layer);
}
#endif

