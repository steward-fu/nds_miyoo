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

#ifndef __SDL_JOYSTICK_MIYOO_H__
#define __SDL_JOYSTICK_MIYOO_H__

#define JOYSTICK_NAME "Miyoo Joystick"

#if defined(A30) || defined(UT)

#if !defined(UT)
#define A30_JOYSTICK_CFG    "/config/joypad.config"
#else
#define A30_JOYSTICK_CFG    "./joypad.config"
#endif

#define A30_JOYSTICK_DEV    "/dev/ttyS0"
#define A30_AXIS_MAX_LEN    16
#define A30_FRAME_LEN       6
#define A30_FRAME_START     0xff
#define A30_FRAME_STOP      0xfe

typedef struct a30_frame {
    uint8_t magic_start;
    uint8_t unused0;
    uint8_t unused1;
    uint8_t axis0;
    uint8_t axis1;
    uint8_t magic_end;
} a30_frame_t;
#endif

typedef struct miyoo_joystick {
#if defined(A30) || defined(UT)
    int dev_fd;
    int last_x;
    int last_y;
    a30_frame_t cur_frame;
    int32_t cur_axis[A30_AXIS_MAX_LEN];
    int32_t last_axis[A30_AXIS_MAX_LEN];

    int running;
    SDL_Thread *thread;

    int x_min;
    int x_max;
    int x_zero;
    int y_min;
    int y_max;
    int y_zero;
#endif
} miyoo_joystick_t;

#endif

