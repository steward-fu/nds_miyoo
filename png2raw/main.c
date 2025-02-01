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
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
 
int main(int argc, char** argv)
{
    int cc = 0;
    char buf[255] = { 0 };
    SDL_Surface *tmp = NULL;

    //SDL_Init(SDL_INIT_VIDEO);
    tmp = SDL_CreateRGBSurface(SDL_SWSURFACE, 32, 32, 16, 0, 0, 0, 0);
    if (!tmp) {
        return -1;
    }

    for (cc = 0; cc <= 1; cc++) {
        snprintf(buf, sizeof(buf), "resources/logo/%d.png", cc);
        SDL_Surface *t0 = IMG_Load(buf);

        if (!t0) {
            continue;
        }
        SDL_Surface *t1 = SDL_ConvertSurface(t0, tmp->format, 0);

        if (t1) {
            int fd = -1;

            sprintf(buf, "drastic_logo_%d.raw", cc);
            fd = open(buf, O_CREAT | O_WRONLY);
            if (fd > 0) {
                write(fd, t1->pixels, 400 * 150 * 2);
                close(fd);
            }
            SDL_FreeSurface(t1);
        }
        SDL_FreeSurface(t0);
    }
    SDL_FreeSurface(tmp);
    //SDL_Quit();
    return 0;
}
