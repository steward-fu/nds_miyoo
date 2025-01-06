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

#ifndef __SDL_EVENT_MIYOO_H__
#define __SDL_EVENT_MIYOO_H__

#if defined(A30)
#define INPUT_DEV "/dev/input/event3"
#endif

#if defined(MINI) || defined(UT)
#define INPUT_DEV "/dev/input/event0"
#endif

#if defined(A30)
#define DEV_KEY_CODE_UP         103
#define DEV_KEY_CODE_DOWN       108
#define DEV_KEY_CODE_LEFT       105
#define DEV_KEY_CODE_RIGHT      106
#define DEV_KEY_CODE_A          57
#define DEV_KEY_CODE_B          29
#define DEV_KEY_CODE_X          42
#define DEV_KEY_CODE_Y          56
#define DEV_KEY_CODE_L1         15
#define DEV_KEY_CODE_L2         18
#define DEV_KEY_CODE_R1         14
#define DEV_KEY_CODE_R2         20
#define DEV_KEY_CODE_START      28
#define DEV_KEY_CODE_SELECT     97
#define DEV_KEY_CODE_MENU       1
#define DEV_KEY_CODE_VOLUP      115
#define DEV_KEY_CODE_VOLDOWN    114
#endif

#if defined(MINI) || defined(UT)
#define DEV_KEY_CODE_UP         103
#define DEV_KEY_CODE_DOWN       108
#define DEV_KEY_CODE_LEFT       105
#define DEV_KEY_CODE_RIGHT      106
#define DEV_KEY_CODE_A          57
#define DEV_KEY_CODE_B          29
#define DEV_KEY_CODE_X          42
#define DEV_KEY_CODE_Y          56
#define DEV_KEY_CODE_L1         18
#define DEV_KEY_CODE_L2         15
#define DEV_KEY_CODE_R1         20
#define DEV_KEY_CODE_R2         14
#define DEV_KEY_CODE_START      28
#define DEV_KEY_CODE_SELECT     97
#define DEV_KEY_CODE_MENU       1
#define DEV_KEY_CODE_POWER      116
#define DEV_KEY_CODE_VOLUP      115
#define DEV_KEY_CODE_VOLDOWN    114
#endif

#define KEY_BIT_UP              0
#define KEY_BIT_DOWN            1
#define KEY_BIT_LEFT            2
#define KEY_BIT_RIGHT           3
#define KEY_BIT_A               4
#define KEY_BIT_B               5
#define KEY_BIT_X               6
#define KEY_BIT_Y               7
#define KEY_BIT_L1              8
#define KEY_BIT_R1              9
#define KEY_BIT_L2              10
#define KEY_BIT_R2              11
#define KEY_BIT_SELECT          12
#define KEY_BIT_START           13
#define KEY_BIT_MENU            14
#define KEY_BIT_QSAVE           15
#define KEY_BIT_QLOAD           16
#define KEY_BIT_FF              17
#define KEY_BIT_EXIT            18
#define KEY_BIT_MENU_ONION      19
#define KEY_BIT_LAST            KEY_BIT_MENU_ONION // ignore other keys
#define KEY_BIT_POWER           20
#define KEY_BIT_VOLUP           21
#define KEY_BIT_VOLDOWN         22

#define DEF_THRESHOLD_UP        -30
#define DEF_THRESHOLD_DOWN      30
#define DEF_THRESHOLD_LEFT      -30
#define DEF_THRESHOLD_RIGHT     30

typedef enum _move_dir {
    MOVE_DIR_LEFT_RIGHT = 0,
    MOVE_DIR_UP_DOWN
} move_dir_t;

typedef enum _dev_mode {
    DEV_MODE_KEY = 0,
    DEV_MODE_PEN
} dev_mode_t;

typedef struct _miyoo_event_t {
#if defined(MINI) || defined(UT)
    int stock_os;
#endif

    struct _key {
        uint32_t cur_bits;
        uint32_t pre_bits;
        SDL_Scancode report_key[32];
    } key;

    struct _pen {
        int x;
        int y;
        int max_x;
        int max_y;
        int lower_speed;
        clock_t pre_ticks;
    } pen;

    struct _dev {
        int fd;
        dev_mode_t mode;
    } dev;

    int running;
    SDL_sem *lock;
    SDL_Thread *thread;

#if defined(A30) || defined(UT)
    struct {
        struct {
            int left;
            int right;
            int up;
            int down;
        } threshold;
    } joy;
#endif
} miyoo_event;

void EventInit(void);
void EventDeinit(void);
void PumpEvents(_THIS);

#endif

