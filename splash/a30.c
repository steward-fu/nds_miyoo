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
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <stdint.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <linux/rtc.h>
#include <json-c/json.h>

#include <SDL.h>
#include <SDL_image.h>

#include "hex_a30_en.h"
#include "hex_a30_cn.h"

#define JSON_PATH "resources/settings.json"
#define JSON_LANG "lang"
#define JSON_SPLASH "splash"

int main(int argc, char *argv[])
{
    int FB_W = 640;
    int FB_H = 480;

    int val = 1;
    int is_en = 1;
    FILE *f = NULL;
    char buf[255] = { 0 }; 
    SDL_RWops *rw = NULL;

    SDL_Rect rt = { 0 };
    SDL_Surface *screen = NULL;
    SDL_Window *window = NULL;
    SDL_Texture *texture = NULL;
    SDL_Renderer *renderer = NULL;

    struct json_object *jval = NULL;
    struct json_object *jfile = NULL;

#ifndef PC
    jfile = json_object_from_file(JSON_PATH);
    if (jfile == NULL) {
        printf("Failed to open json file (%s)\n", JSON_PATH);
        return -1;
    }
    else {
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
    }
#endif

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);
    window = SDL_CreateWindow(
        "Main",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        480,
        640,
        SDL_WINDOW_SHOWN);

    renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGB888,
        SDL_TEXTUREACCESS_TARGET,
        640,
        480);

    screen = SDL_CreateRGBSurface(0, 640, 480, 32, 0, 0, 0, 0);
    SDL_ShowCursor(0);

    rt.x = (480 - 640) / 2;
    rt.y = (640 - 480) / 2;
    rt.w = 640;
    rt.h = 480;

    if (is_en) {
        rw = SDL_RWFromMem(hex_a30_en, sizeof(hex_a30_en));
    }
    else {
        rw = SDL_RWFromMem(hex_a30_cn, sizeof(hex_a30_cn));
    }

    SDL_Surface* t = IMG_Load_RW(rw, 1);
    if (t) {
        SDL_Surface *png = SDL_ConvertSurface(t, screen->format, 0);
        SDL_BlitSurface(png, NULL, screen, NULL);

        printf("Show splash..\n");
        SDL_SetRenderTarget(renderer, NULL);
        SDL_UpdateTexture(texture, NULL, screen->pixels, screen->pitch);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
        SDL_SetRenderTarget(renderer, texture);

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
        printf("Done\n");
    }

    SDL_FreeSurface(screen);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
