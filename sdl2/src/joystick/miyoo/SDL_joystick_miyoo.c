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

#include "../../SDL_internal.h"
#include "../../events/SDL_events_c.h"
#include "../../core/linux/SDL_evdev.h"
#include "../../thread/SDL_systhread.h"

#include "SDL_events.h"
#include "SDL_joystick.h"
#include "../SDL_sysjoystick.h"
#include "../SDL_joystick_c.h"
#include "../../video/miyoo/SDL_video_miyoo.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/input.h>

#ifdef A30
#define SERIAL_GAMEDECK         "/dev/ttyS0"
#define MIYOO_AXIS_MAX_COUNT    16
#define MIYOO_PLAYER_MAGIC      0xFF
#define MIYOO_PLAYER_MAGIC_END  0xFE
#define MIYOO_PAD_FRAME_LEN     6

struct MIYOO_PAD_FRAME {
    uint8_t magic;
    uint8_t unused0;
    uint8_t unused1;
    uint8_t axis0;
    uint8_t axis1;
    uint8_t magicEnd;
};

int g_lastX = 0;
int g_lastY = 0;

static int s_fd = -1;
static struct MIYOO_PAD_FRAME s_frame = {0};
static int32_t s_miyoo_axis[MIYOO_AXIS_MAX_COUNT] = {0};
static int32_t s_miyoo_axis_last[MIYOO_AXIS_MAX_COUNT] = {0};

static int running = 0;
static SDL_Thread *thread = NULL;

extern NDS nds;
extern MMIYOO_VideoInfo vid;

static int uart_open(const char *port)
{
    int fd = -1;

    fd = open(port, O_RDWR | O_NOCTTY | O_NDELAY);
    if (-1 == fd) {
        printf("Failed to open uart\n");
        return -1;
    }

    if (fcntl(fd, F_SETFL, 0) < 0) {
        printf("Failed to call fcntl\n");
        return -1;
    }
    return fd;
}

static void uart_close(int fd)
{
    close(fd);
}

static int uart_set(int fd, int speed, int flow_ctrl, int databits, int stopbits, int parity)
{
    int i = 0;
    int speed_arr[] = {B115200, B19200, B9600, B4800, B2400, B1200, B300};
    int name_arr[] = {115200, 19200, 9600, 4800, 2400, 1200, 300};
    struct termios options = {0};

    if (tcgetattr(fd, &options) != 0) {
        printf("Failed to get uart attributes\n");
        return -1;
    }

    for (i = 0; i < sizeof(speed_arr) / sizeof(int); i++) {
        if (speed == name_arr[i]) {
            cfsetispeed(&options, speed_arr[i]);
            cfsetospeed(&options, speed_arr[i]);
        }
    }

    options.c_cflag |= CLOCAL;
    options.c_cflag |= CREAD;
    switch (flow_ctrl) {
    case 0:
        options.c_cflag &= ~CRTSCTS;
        break;
    case 1:
        options.c_cflag |= CRTSCTS;
        break;
    case 2:
        options.c_cflag |= IXON | IXOFF | IXANY;
        break;
    }

    options.c_cflag &= ~CSIZE;
    switch (databits) {
    case 5:
        options.c_cflag |= CS5;
        break;
    case 6:
        options.c_cflag |= CS6;
        break;
    case 7:
        options.c_cflag |= CS7;
        break;
    case 8:
        options.c_cflag |= CS8;
        break;
    default:
        return -1;
    }

    switch (parity) {
    case 'n':
    case 'N':
        options.c_cflag &= ~PARENB;
        options.c_iflag &= ~INPCK;
        break;
    case 'o':
    case 'O':
        options.c_cflag |= (PARODD | PARENB);
        options.c_iflag |= INPCK;
        break;
    case 'e':
    case 'E':
        options.c_cflag |= PARENB;
        options.c_cflag &= ~PARODD;
        options.c_iflag |= INPCK;
        break;
    case 's':
    case 'S':
        options.c_cflag &= ~PARENB;
        options.c_cflag &= ~CSTOPB;
        break;
    default:
        return -1;
    }

    switch (stopbits) {
    case 1:
        options.c_cflag &= ~CSTOPB;
        break;
    case 2:
        options.c_cflag |= CSTOPB;
        break;
    default:
        return -1;
    }

    options.c_oflag &= ~OPOST;
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_iflag &= ~(INLCR | ICRNL | IGNCR);
    options.c_oflag &= ~(ONLCR | OCRNL);
    options.c_cc[VTIME] = 1;
    options.c_cc[VMIN] = 1;

    tcflush(fd, TCIFLUSH);
    if (tcsetattr(fd, TCSANOW, &options) != 0) {
        return -1;
    }
    return 0;
}

static int uart_init(int fd, int speed, int flow_ctrl, int databits, int stopbits, int parity)
{
    if (uart_set(fd, speed, flow_ctrl, databits, stopbits, parity) == -1) {
        return -1;
    }
    return 0;
}

static int uart_read(int fd, char *rcv_buf, int data_len)
{
    int f_sel;
    fd_set f_read;
    struct timeval time = {0};

    FD_ZERO(&f_read);
    FD_SET(fd, &f_read);

    time.tv_sec = 10;
    time.tv_usec = 0;
    f_sel = select(fd + 1, &f_read, NULL, NULL, &time);
    if (f_sel) {
        return read(fd, rcv_buf, data_len);
    }
    return 0;
}

static int filterDeadzone(int newAxis, int oldAxis)
{
    if (abs(newAxis - oldAxis) < nds.joy.dzone) {
        return 1;
    }
    return 0;
}

static int limitValue8(int value)
{
    if (value > 127) {
        value = 127;
    }
    else if (value < -128) {
        value = -128;
    }
    return value;
}

static void check_axis_event(void)
{
    int i = 0;

    for (i = 0; i < MIYOO_AXIS_MAX_COUNT; i++) {
        if (s_miyoo_axis[i] != s_miyoo_axis_last[i]) {
            if (!filterDeadzone(s_miyoo_axis[i], s_miyoo_axis_last[i])) {
                if (i == 0) {
                    g_lastX = limitValue8(s_miyoo_axis[i]);
                    //printf("X %d\n", g_lastX);
                }
                else if (i == 1) {
                    g_lastY = limitValue8(s_miyoo_axis[i]);
                    //printf("Y %d\n", g_lastY);
                }
            }
        }
        s_miyoo_axis_last[i] = s_miyoo_axis[i];
    }
}

static int miyoo_frame_to_axis_x(uint8_t rawX)
{
    int value = 0;

    if (rawX > nds.joy.zero_x) {
        value = (rawX - nds.joy.zero_x) * 126 / (nds.joy.max_x - nds.joy.zero_x);
    }

    if (rawX < nds.joy.zero_x) {
        value = (rawX - nds.joy.zero_x) * 126 / (nds.joy.zero_x - nds.joy.min_x);
    }

    if (value > 0 && value < nds.joy.dzone) {
        return 0;
    }

    if (value < 0 && value > -(nds.joy.dzone)) {
        return 0;
    }
    return value;
}

static int miyoo_frame_to_axis_y(uint8_t rawY)
{
    int value = 0;

    if (rawY > nds.joy.zero_y) {
        value = (rawY - nds.joy.zero_y) * 126 / (nds.joy.max_y - nds.joy.zero_y);
    }

    if (rawY < nds.joy.zero_y) {
        value = (rawY - nds.joy.zero_y) * 126 / (nds.joy.zero_y - nds.joy.min_y);
    }

    if (value > 0 && value < nds.joy.dzone) {
        return 0;
    }

    if (value < 0 && value > -(nds.joy.dzone)) {
        return 0;
    }
    return value;
}

static int parser_miyoo_input(const char *cmd, int len)
{
    int i = 0;
    int p = 0;

    if ((!cmd) || (len < MIYOO_PAD_FRAME_LEN)) {
        return - 1;
    }

    for (i = 0; i < len - MIYOO_PAD_FRAME_LEN + 1; i += MIYOO_PAD_FRAME_LEN) {
        for (p = 0; p < MIYOO_PAD_FRAME_LEN - 1; p++) {
            if ((cmd[i] == MIYOO_PLAYER_MAGIC) && (cmd[i + MIYOO_PAD_FRAME_LEN - 1] == MIYOO_PLAYER_MAGIC_END)) {
                memcpy(&s_frame, cmd + i, sizeof(s_frame));
                break;
            }
            else {
                i++;
            }
        }
    }
    s_miyoo_axis[ABS_X] = miyoo_frame_to_axis_x(s_frame.axis0);
    s_miyoo_axis[ABS_Y] = miyoo_frame_to_axis_y(s_frame.axis1);
    check_axis_event();
    return 0;
}

static int miyoo_init_serial_input(void)
{
    memset(&s_frame, 0, sizeof(s_frame));
    memset(s_miyoo_axis, 0, sizeof(s_miyoo_axis));
    memset(s_miyoo_axis_last, 0, sizeof(s_miyoo_axis_last));
    s_fd = uart_open(SERIAL_GAMEDECK);
    uart_init(s_fd, 9600, 0, 8, 1, 'N');
    if (s_fd <= 0) {
        return -1;
    }
    return 0;
}

static void miyoo_close_serial_input(void)
{
}

static int chk_str(const char *src, const char *dst)
{
    int len = strlen(dst);
    return (memcmp(src, dst, len) == 0) ? 1 : 0;
}

static int miyoo_read_joystick_config(void)
{
    FILE *f = NULL;
    char buf[255] = {0};
    const char *path = "/config/joypad.config";

    f = fopen(path, "r");
    if (f) {
        while (fgets(buf, sizeof(buf), f)) {
            if (chk_str(buf, "x_min=")) {
                printf(PREFIX"X_MIN %d\n", atoi(&buf[6]));
            }
            else if (chk_str(buf, "x_max=")) {
                printf(PREFIX"X_MAX %d\n", atoi(&buf[6]));
            }
            else if (chk_str(buf, "x_zero=")) {
                printf(PREFIX"X_ZERO %d\n", atoi(&buf[7]));
            }
            else if (chk_str(buf, "y_min=")) {
                printf(PREFIX"Y_MIN %d\n", atoi(&buf[6]));
            }
            else if (chk_str(buf, "y_max=")) {
                printf(PREFIX"Y_MAX %d\n", atoi(&buf[6]));
            }
            else if (chk_str(buf, "y_zero=")) {
                printf(PREFIX"Y_ZERO %d\n", atoi(&buf[7]));
            }
        }
        fclose(f);
    }
    return 0;
}

int joystick_handler(void *param)
{
    int len = 0;
    char rcv_buf[255] = {0};

    while (running) {
        len = uart_read(s_fd, rcv_buf, 99);
        if (len > 0) {
            rcv_buf[len] = '\0';
            parser_miyoo_input(rcv_buf, len);
        }
        usleep(10000);
    }
    return 0;
}
#endif

int MMIYOO_JoystickInit(void)
{
    printf(PREFIX"%s\n", __func__);

#ifdef A30
    running = 1;
    miyoo_read_joystick_config();
    miyoo_init_serial_input();
    if((thread = SDL_CreateThreadInternal(joystick_handler, "a30_joystick_thread", 4096, NULL)) == NULL) {
        printf(PREFIX"Failed to create joystick thread");
    }
#endif
    return 0;
}

void MMIYOO_JoystickQuit(void)
{
    printf(PREFIX"%s\n", __func__);

#ifdef A30
    running = 0;
    miyoo_close_serial_input();
    SDL_WaitThread(thread, NULL);
    uart_close(s_fd);
    s_fd = -1;
#endif
}

static int MMIYOO_JoystickGetCount(void)
{
    return 1;
}

static void MMIYOO_JoystickDetect(void)
{
}

static const char* MMIYOO_JoystickGetDeviceName(int device_index)
{
    return "Miyoo MMIYOO Joystick";
}

static int MMIYOO_JoystickGetDevicePlayerIndex(int device_index)
{
    return -1;
}

static void MMIYOO_JoystickSetDevicePlayerIndex(int device_index, int player_index)
{
}

static SDL_JoystickGUID MMIYOO_JoystickGetDeviceGUID(int device_index)
{
    SDL_JoystickGUID guid;
    const char *name = MMIYOO_JoystickGetDeviceName(device_index);

    SDL_zero(guid);
    SDL_memcpy(&guid, name, SDL_min(sizeof(guid), SDL_strlen(name)));
    return guid;
}

static SDL_JoystickID MMIYOO_JoystickGetDeviceInstanceID(int device_index)
{
    return device_index;
}

int MMIYOO_JoystickOpen(SDL_Joystick *joystick, int device_index)
{
    printf(PREFIX"%s\n", __func__);
    if (joystick) {
        joystick->nbuttons = 18;
        joystick->naxes = 2;
        joystick->nhats = 0;
        joystick->instance_id = device_index;
    }
    return 0;
}

void MMIYOO_JoystickClose(SDL_Joystick *joystick)
{
    printf(PREFIX"%s\n", __func__);
}

static int MMIYOO_JoystickRumble(SDL_Joystick *joystick, Uint16 low_frequency_rumble, Uint16 high_frequency_rumble)
{
    return SDL_Unsupported();
}

static int MMIYOO_JoystickRumbleTriggers(SDL_Joystick *joystick, Uint16 left_rumble, Uint16 right_rumble)
{
    return SDL_Unsupported();
}

static Uint32 MMIYOO_JoystickGetCapabilities(SDL_Joystick *joystick)
{
    return SDL_Unsupported();
}

static int MMIYOO_JoystickSetLED(SDL_Joystick *joystick, Uint8 red, Uint8 green, Uint8 blue)
{
    return SDL_Unsupported();
}

static int MMIYOO_JoystickSendEffect(SDL_Joystick *joystick, const void *data, int size)
{
    return SDL_Unsupported();
}

static int MMIYOO_JoystickSetSensorsEnabled(SDL_Joystick *joystick, SDL_bool enabled)
{
    return SDL_Unsupported();
}

void MMIYOO_JoystickUpdate(SDL_Joystick *joystick)
{
#ifdef A30
    static int pre_x = -1;
    static int pre_y = -1;

    if (g_lastX != pre_x) {
        pre_x = g_lastX;
        SDL_PrivateJoystickAxis(joystick, 0, pre_x);
    }

    if (g_lastY != pre_y) {
        pre_y = g_lastY;
        SDL_PrivateJoystickAxis(joystick, 1, pre_x);
    }
#endif
}

static SDL_bool MMIYOO_JoystickGetGamepadMapping(int device_index, SDL_GamepadMapping *out)
{
    return SDL_FALSE;
}

SDL_JoystickDriver SDL_MMIYOO_JoystickDriver = {
    MMIYOO_JoystickInit,
    MMIYOO_JoystickGetCount,
    MMIYOO_JoystickDetect,
    MMIYOO_JoystickGetDeviceName,
    MMIYOO_JoystickGetDevicePlayerIndex,
    MMIYOO_JoystickSetDevicePlayerIndex,
    MMIYOO_JoystickGetDeviceGUID,
    MMIYOO_JoystickGetDeviceInstanceID,
    MMIYOO_JoystickOpen,
    MMIYOO_JoystickRumble,
    MMIYOO_JoystickRumbleTriggers,
    MMIYOO_JoystickGetCapabilities,
    MMIYOO_JoystickSetLED,
    MMIYOO_JoystickSendEffect,
    MMIYOO_JoystickSetSensorsEnabled,
    MMIYOO_JoystickUpdate,
    MMIYOO_JoystickClose,
    MMIYOO_JoystickQuit,
    MMIYOO_JoystickGetGamepadMapping
};

