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

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <stdint.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <linux/rtc.h>
#include <json-c/json.h>

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#include "hex_mini_en.h"
#include "hex_mini_cn.h"

#define JSON_PATH "resources/settings.json"
#define JSON_LANG "lang"
#define JSON_SPLASH "splash"

int main(int argc, char *argv[])
{
    int FB_W = 640;
    int FB_H = 480;
    int FB_BPP = 32;

    int val = 1;
    int is_en = 1;
    FILE *f = NULL;
    char buf[255] = { 0 }; 
    SDL_RWops *rw = NULL;
    SDL_Surface *screen = NULL;

    struct json_object *jval = NULL;
    struct json_object *jfile = NULL;

    DIR* dir = opendir("/mnt/SDCARD/Emus/");
    if (dir) {
        FB_W = 320;
        FB_H = 240;
        FB_BPP = 16;
        closedir(dir);
    }

    jfile = json_object_from_file(JSON_PATH);
    if (jfile == NULL) {
        return -1;
    }

    json_object_object_get_ex(jfile, JSON_SPLASH, &jval);
    if (jval) {
        val = json_object_get_int(jval);
    }
    if (val <= 0) {
        json_object_put(jfile);
        return 0;
    }
    val-= 1;

    json_object_object_get_ex(jfile, JSON_LANG, &jval);
    if (jval) {
        if (strcmp(json_object_get_string(jval), "chinese_cn") == 0) {
            is_en = 0;
        }
    }

    json_object_object_add(jfile, JSON_SPLASH, json_object_new_int(val));
    json_object_to_file_ext(JSON_PATH, jfile, JSON_C_TO_STRING_PRETTY);
    json_object_put(jfile);

    SDL_Init(SDL_INIT_VIDEO);
    screen = SDL_SetVideoMode(FB_W, FB_H, FB_BPP, SDL_SWSURFACE | SDL_DOUBLEBUF);
    SDL_ShowCursor(0);

    if (is_en) {
        if (FB_BPP == 32) {
            rw = SDL_RWFromMem(hex_mini_en, sizeof(hex_mini_en));
        }
    }
    else {
        if (FB_BPP == 32) {
            rw = SDL_RWFromMem(hex_mini_cn, sizeof(hex_mini_cn));
        }
    }

    SDL_Surface* t = IMG_Load_RW(rw, 1);
    if (t) {
        SDL_Surface *png = SDL_ConvertSurface(t, screen->format, 0);
        if (FB_BPP == 32) {
            SDL_BlitSurface(png, NULL, screen, NULL);
        }
        else {
            int x = 0;
            int y = 0;
            uint16_t *src = (uint16_t *)png->pixels;
            uint16_t *dst = (uint16_t *)screen->pixels;

            for (y = 0; y < FB_H; y++) {
                for (x = 0; x < FB_W; x++) {
                    dst[(((FB_W - 1) - x) * FB_H) + y] = *src++;
                }
            }
        }
        SDL_Flip(screen);
        SDL_FreeSurface(png);
        SDL_FreeSurface(t);

        int cc = 1000;
        SDL_Event event = {0};
        while (cc--) {
            if (SDL_PollEvent(&event)) {
                if (event.type == SDL_KEYDOWN) {
                    break;
                }
            }
            SDL_Delay(30);
        }
    }
    SDL_FillRect(
        screen,
        &screen->clip_rect,
        SDL_MapRGB(screen->format, 0, 0, 0));

    SDL_Flip(screen);

    SDL_FillRect(
        screen,
        &screen->clip_rect,
        SDL_MapRGB(screen->format, 0, 0, 0));

    SDL_Flip(screen);
    SDL_Quit();
    return 0;
}

