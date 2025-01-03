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

#if defined(A30) || defined(UT)
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
#endif

#include "log.h"
#include "joystick_miyoo.h"

#if defined(UT)
#include "unity_fixture.h"
#endif

extern NDS nds;
static miyoo_joystick_t myjoy = { 0 };

#if defined(UT)
TEST_GROUP(sdl2_joystick_miyoo);

TEST_SETUP(sdl2_joystick_miyoo)
{
    FILE *f = fopen(A30_JOYSTICK_CFG, "w+");

    if (f) {
        fprintf(f, "x_min=%d\n", UT_X_MIN);
        fprintf(f, "x_max=%d\n", UT_X_MAX);
        fprintf(f, "x_zero=%d\n", UT_X_ZERO);
        fprintf(f, "y_min=%d\n", UT_Y_MIN);
        fprintf(f, "y_max=%d\n", UT_Y_MAX);
        fprintf(f, "y_zero=%d\n", UT_Y_ZERO);
        fclose(f);
    }
}

TEST_TEAR_DOWN(sdl2_joystick_miyoo)
{
    unlink(A30_JOYSTICK_CFG);
}
#endif

#if defined(A30) || defined(UT)
static int uart_open(const char *port)
{
    int fd = -1;

    if (!port) {
        err(SDL"invalid parameter(0x%x) in %s\n", port, __func__);
        return -1;
    }

#if defined(UT)
    return 0;
#endif

    fd = open(port, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd < 0) {
        err(SDL"failed to open \"%s\" in %s\n", port, __func__);
        return -1;
    }

    if (fcntl(fd, F_SETFL, 0) < 0) {
        err(SDL"failed to set fcntl in %s\n", __func__);
        return -1;
    }

    return fd;
}

#if defined(UT)
TEST(sdl2_joystick_miyoo, uart_open)
{
    TEST_ASSERT_EQUAL_INT(-1, uart_open(NULL));
    TEST_ASSERT_EQUAL_INT(0, uart_open("/dev"));
}
#endif

static int uart_set(int fd, int speed, int flow_ctrl, int databits, int stopbits, int parity)
{
#if !defined(UT)
    int i = 0;
    int speed_arr[] = {B115200, B19200, B9600, B4800, B2400, B1200, B300};
    int name_arr[] = {115200, 19200, 9600, 4800, 2400, 1200, 300};
#endif
    struct termios options = {0};

    if (fd < 0) {
        err(SDL"invalid parameters(0x%x) in %s\n", fd, __func__);
        return -1;
    }

#if !defined(UT)
    if (tcgetattr(fd, &options) != 0) {
        err(SDL"failed to get uart attributes in %s\n", __func__);
        return -1;
    }

    for (i = 0; i < sizeof(speed_arr) / sizeof(int); i++) {
        if (speed == name_arr[i]) {
            cfsetispeed(&options, speed_arr[i]);
            cfsetospeed(&options, speed_arr[i]);
        }
    }
#endif

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
    default:
        err(SDL"invalid flow bit(0x%x) in %s\n", flow_ctrl, __func__);
        return -1;
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
        err(SDL"invalid data bit(0x%x) in %s\n", databits, __func__);
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
        err(SDL"invalid parity bit(0x%x) in %s\n", parity, __func__);
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
        err(SDL"invalid stop bit(0x%x) in %s\n", stopbits, __func__);
        return -1;
    }

    options.c_oflag &= ~OPOST;
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_iflag &= ~(INLCR | ICRNL | IGNCR);
    options.c_oflag &= ~(ONLCR | OCRNL);
    options.c_cc[VTIME] = 1;
    options.c_cc[VMIN] = 1;

#if !defined(UT)
    tcflush(fd, TCIFLUSH);
    if (tcsetattr(fd, TCSANOW, &options) != 0) {
        err(SDL"failed to get uart attributes in %s\n", __func__);
        return -1;
    }
#endif

    return 0;
}

#if defined(UT)
TEST(sdl2_joystick_miyoo, uart_set)
{
    TEST_ASSERT_EQUAL_INT(-1, uart_set(0, 0, 0, 0, 0, 0));
    TEST_ASSERT_EQUAL_INT(-1, uart_set(0, 9600, 0, 32, 10, 'X'));
    TEST_ASSERT_EQUAL_INT(0, uart_set(0, 9600, 0, 8, 1, 'N'));
}
#endif

static int uart_init(int fd, int speed, int flow_ctrl, int databits, int stopbits, int parity)
{
    if (fd < 0) {
        err(SDL"invalid parameters(0x%x) in %s\n", fd, __func__);
        return -1;
    }

    if (uart_set(fd, speed, flow_ctrl, databits, stopbits, parity) < 0) {
        err(SDL"failed to set joystick device in %s\n", __func__);
        return -1;
    }

    return 0;
}

#if defined(UT)
TEST(sdl2_joystick_miyoo, uart_init)
{
    TEST_ASSERT_EQUAL_INT(-1, uart_init(0, 0, 0, 0, 0, 0));
    TEST_ASSERT_EQUAL_INT(0, uart_init(0, 9600, 0, 8, 1, 'N'));
}
#endif

static int uart_read(int fd, char *buf, int len)
{
    int f_sel = 0;
    fd_set f_read = { 0 };
    struct timeval time = { 0 };

    if ((fd < 0) || !buf || (len <= 0)) {
        err(SDL"invalid parameters(0x%x, 0x%x, 0x%x) in %s\n", fd, buf, len, __func__);
        return -1;
    }

#if defined(UT)
    return 0;
#endif

    FD_ZERO(&f_read);
    FD_SET(fd, &f_read);

    time.tv_sec = 10;
    time.tv_usec = 0;
    f_sel = select(fd + 1, &f_read, NULL, NULL, &time);
    if (f_sel) {
        return read(fd, buf, len);
    }
    return 0;
}

#if defined(UT)
TEST(sdl2_joystick_miyoo, uart_read)
{
    char buf[32] = { 0 };

    TEST_ASSERT_EQUAL_INT(-1, uart_read(-1, NULL, 0));
    TEST_ASSERT_EQUAL_INT(-1, uart_read(0, NULL, 0));
    TEST_ASSERT_EQUAL_INT(-1, uart_read(0, buf, 0));
    TEST_ASSERT_EQUAL_INT(0, uart_read(0, buf, sizeof(buf)));
}
#endif

static int filter_dead_zone(int newAxis, int oldAxis)
{
    if (abs(newAxis - oldAxis) < nds.joy.dzone) {
        return 1;
    }
    return 0;
}

#if defined(UT)
TEST(sdl2_joystick_miyoo, filter_dead_zone)
{
    nds.joy.dzone = 10;
    TEST_ASSERT_EQUAL_INT(0, filter_dead_zone(100, 0));

    nds.joy.dzone = 100;
    TEST_ASSERT_EQUAL_INT(1, filter_dead_zone(10, 0));
}
#endif

static int limit_value(int value)
{
    if (value > 127) {
        value = 127;
    }
    else if (value < -128) {
        value = -128;
    }
    return value;
}

#if defined(UT)
TEST(sdl2_joystick_miyoo, limit_value)
{
    TEST_ASSERT_EQUAL_INT(127, limit_value(255));
    TEST_ASSERT_EQUAL_INT(0, limit_value(0));
    TEST_ASSERT_EQUAL_INT(-128, limit_value(-255));
}
#endif

static void check_axis_event(void)
{
    int i = 0;

    for (i = 0; i < A30_AXIS_MAX_LEN; i++) {
        if (myjoy.cur_axis[i] != myjoy.last_axis[i]) {
            if (!filter_dead_zone(myjoy.cur_axis[i], myjoy.last_axis[i])) {
                if (i == 0) {
                    myjoy.last_x = limit_value(myjoy.cur_axis[i]);
                    debug(SDL"joystick x:%d in %s\n", myjoy.last_x, __func__);
                }
                else if (i == 1) {
                    myjoy.last_y = limit_value(myjoy.cur_axis[i]);
                    debug(SDL"joystick y:%d in %s\n", myjoy.last_y, __func__);
                }
            }
        }
        myjoy.last_axis[i] = myjoy.cur_axis[i];
    }
}

#if defined(UT)
TEST(sdl2_joystick_miyoo, check_axis_event)
{
    nds.joy.dzone = 0;
    myjoy.last_x = 0;
    myjoy.cur_axis[0] = 100;
    myjoy.last_axis[0] = 0;

    myjoy.last_y = 0;
    myjoy.cur_axis[1] = 100;
    myjoy.last_axis[1] = 0;
    check_axis_event();
    TEST_ASSERT_EQUAL_INT(100, myjoy.last_x);
    TEST_ASSERT_EQUAL_INT(100, myjoy.last_y);

    nds.joy.dzone = 110;
    myjoy.last_x = 0;
    myjoy.cur_axis[0] = 100;
    myjoy.last_axis[0] = 0;

    myjoy.last_y = 0;
    myjoy.cur_axis[1] = 100;
    myjoy.last_axis[1] = 0;
    check_axis_event();
    TEST_ASSERT_EQUAL_INT(0, myjoy.last_x);
    TEST_ASSERT_EQUAL_INT(0, myjoy.last_y);
}
#endif

static int frame_to_axis_x(uint8_t x)
{
    int div = 0;
    int value = 0;

    div = nds.joy.max_x - nds.joy.zero_x;
    if ((x > nds.joy.zero_x) && (div > 0)) {
        value = ((x - nds.joy.zero_x) * 126) / div;
    }

    div = nds.joy.zero_x - nds.joy.min_x;
    if ((x < nds.joy.zero_x) && (div > 0)) {
        value = ((x - nds.joy.zero_x) * 126) / div;
    }

    if (value > 0 && value < nds.joy.dzone) {
        return 0;
    }

    if (value < 0 && value > -(nds.joy.dzone)) {
        return 0;
    }
    return value;
}

#if defined(UT)
TEST(sdl2_joystick_miyoo, frame_to_axis_x)
{
    nds.joy.max_x = 0;
    nds.joy.zero_x = 0;
    nds.joy.dzone = 0;
    TEST_ASSERT_EQUAL_INT(0, frame_to_axis_x(0));

    nds.joy.max_x = 1;
    nds.joy.zero_x = 0;
    nds.joy.dzone = 0;
    TEST_ASSERT_EQUAL_INT(32130, frame_to_axis_x(255));

    nds.joy.max_x = 1;
    nds.joy.zero_x = 0;
    nds.joy.dzone = 32131;
    TEST_ASSERT_EQUAL_INT(0, frame_to_axis_x(255));
}
#endif

static int frame_to_axis_y(uint8_t y)
{
    int div = 0;
    int value = 0;

    div = nds.joy.max_y - nds.joy.zero_y;
    if ((y > nds.joy.zero_y) && (div > 0)) {
        value = ((y - nds.joy.zero_y) * 126) / div;
    }

    div = nds.joy.zero_y - nds.joy.min_y;
    if ((y < nds.joy.zero_y) && (div > 0)) {
        value = ((y - nds.joy.zero_y) * 126) / div;
    }

    if ((value > 0) && (value < nds.joy.dzone)) {
        return 0;
    }

    if ((value < 0) && (value > -(nds.joy.dzone))) {
        return 0;
    }

    return value;
}

#if defined(UT)
TEST(sdl2_joystick_miyoo, frame_to_axis_y)
{
    nds.joy.max_y = 0;
    nds.joy.zero_y = 0;
    nds.joy.dzone = 0;
    TEST_ASSERT_EQUAL_INT(0, frame_to_axis_y(0));

    nds.joy.max_y = 1;
    nds.joy.zero_y = 0;
    nds.joy.dzone = 0;
    TEST_ASSERT_EQUAL_INT(32130, frame_to_axis_y(255));

    nds.joy.max_y = 1;
    nds.joy.zero_y = 0;
    nds.joy.dzone = 32131;
    TEST_ASSERT_EQUAL_INT(0, frame_to_axis_y(255));
}
#endif

static int parse_serial_buf(const char *cmd, int len)
{
    int i = 0;
    int p = 0;
    int s = 0;

    if (!cmd || (len < A30_FRAME_LEN)) {
        err(SDL"invalid parameters(0x%x, 0x%x) in %s\n", cmd, len, __func__);
        return - 1;
    }

    s = len - A30_FRAME_LEN;
    s += 1;
    for (i = 0; i < s; i += A30_FRAME_LEN) {
        for (p = 0; p < (A30_FRAME_LEN - 1); p++) {
            int tail = i + (A30_FRAME_LEN - 1);

            if (tail >= len) {
                break;
            }

            if ((cmd[i] == (char)A30_FRAME_START) && (cmd[tail] == (char)A30_FRAME_STOP)) {
                memcpy(&myjoy.cur_frame, cmd + i, sizeof(myjoy.cur_frame));
                break;
            }
            else {
                i += 1;
            }
        }
    }
    myjoy.cur_axis[ABS_X] = frame_to_axis_x(myjoy.cur_frame.axis0);
    myjoy.cur_axis[ABS_Y] = frame_to_axis_y(myjoy.cur_frame.axis1);
    check_axis_event();
    return 0;
}

#if defined(UT)
TEST(sdl2_joystick_miyoo, parse_serial_buf)
{
    char buf0[] = { A30_FRAME_START, 1, 2, 11, 22, A30_FRAME_STOP };
    char buf1[] = { 5, 6, A30_FRAME_START, 1, 2, 33, 44, A30_FRAME_STOP };
    char buf2[] = { 5, 6, A30_FRAME_START, 1, 2, 3 };

    TEST_ASSERT_EQUAL_INT(-1, parse_serial_buf(NULL, 0));
    TEST_ASSERT_EQUAL_INT(-1, parse_serial_buf(buf0, 0));

    TEST_ASSERT_EQUAL_INT(0, parse_serial_buf(buf0, sizeof(buf0)));
    TEST_ASSERT_EQUAL_INT(A30_FRAME_START, myjoy.cur_frame.magic_start);
    TEST_ASSERT_EQUAL_INT(buf0[1], myjoy.cur_frame.unused0);
    TEST_ASSERT_EQUAL_INT(buf0[2], myjoy.cur_frame.unused1);
    TEST_ASSERT_EQUAL_INT(buf0[3], myjoy.cur_frame.axis0);
    TEST_ASSERT_EQUAL_INT(buf0[4], myjoy.cur_frame.axis1);
    TEST_ASSERT_EQUAL_INT(A30_FRAME_STOP, myjoy.cur_frame.magic_end);

    TEST_ASSERT_EQUAL_INT(0, parse_serial_buf(buf1, sizeof(buf1)));
    TEST_ASSERT_EQUAL_INT(A30_FRAME_START, myjoy.cur_frame.magic_start);
    TEST_ASSERT_EQUAL_INT(buf1[3], myjoy.cur_frame.unused0);
    TEST_ASSERT_EQUAL_INT(buf1[4], myjoy.cur_frame.unused1);
    TEST_ASSERT_EQUAL_INT(buf1[5], myjoy.cur_frame.axis0);
    TEST_ASSERT_EQUAL_INT(buf1[6], myjoy.cur_frame.axis1);
    TEST_ASSERT_EQUAL_INT(A30_FRAME_STOP, myjoy.cur_frame.magic_end);

    memset(&myjoy.cur_frame, 0, sizeof(myjoy.cur_frame));
    TEST_ASSERT_EQUAL_INT(0, parse_serial_buf(buf2, sizeof(buf2)));
    TEST_ASSERT_EQUAL_INT(0, myjoy.cur_frame.magic_start);
    TEST_ASSERT_EQUAL_INT(0, myjoy.cur_frame.unused0);
    TEST_ASSERT_EQUAL_INT(0, myjoy.cur_frame.unused1);
    TEST_ASSERT_EQUAL_INT(0, myjoy.cur_frame.axis0);
    TEST_ASSERT_EQUAL_INT(0, myjoy.cur_frame.axis1);
    TEST_ASSERT_EQUAL_INT(0, myjoy.cur_frame.magic_end);
}
#endif

static int init_serial_input(void)
{
    myjoy.last_x = 0;
    myjoy.last_y = 0;
    memset(myjoy.cur_axis, 0, sizeof(myjoy.cur_axis));
    memset(myjoy.last_axis, 0, sizeof(myjoy.last_axis));
    memset(&(myjoy.cur_frame), 0, sizeof(myjoy.cur_frame));

    myjoy.dev_fd = uart_open(A30_JOYSTICK_DEV);
    if (myjoy.dev_fd < 0) {
        return -1;
    }

    if (uart_init(myjoy.dev_fd, 9600, 0, 8, 1, 'N') < 0) {
        return -1;
    }

    return 0;
}

#if defined(UT)
TEST(sdl2_joystick_miyoo, init_serial_input)
{
    TEST_ASSERT_EQUAL_INT(0, init_serial_input());
    TEST_ASSERT_EQUAL_INT(0, myjoy.cur_frame.magic_start);
    TEST_ASSERT_EQUAL_INT(0, myjoy.cur_axis[0]);
    TEST_ASSERT_EQUAL_INT(0, myjoy.last_axis[0]);
}
#endif

static int read_joystick_config(void)
{
    FILE *f = NULL;
    char buf[255] = {0};

    f = fopen(A30_JOYSTICK_CFG, "r");
    if (!f) {
        err(SDL"failed to open \"%s\" in %s\n", A30_JOYSTICK_CFG, __func__);
        return -1;
    }

    while (fgets(buf, sizeof(buf), f)) {
        if (strcasestr(buf, "x_min=")) {
            myjoy.x_min = atoi(&buf[6]);
            info(SDL"joystick x_min:%d in %s\n", myjoy.x_min, __func__);
        }
        else if (strcasestr(buf, "x_max=")) {
            myjoy.x_max = atoi(&buf[6]);
            info(SDL"joystick x_max:%d in %s\n", myjoy.x_min, __func__);
        }
        else if (strcasestr(buf, "x_zero=")) {
            myjoy.x_zero = atoi(&buf[7]);
            info(SDL"joystick x_zero:%d in %s\n", myjoy.x_zero, __func__);
        }
        else if (strcasestr(buf, "y_min=")) {
            myjoy.y_min = atoi(&buf[6]);
            info(SDL"joystick y_min:%d in %s\n", myjoy.y_min, __func__);
        }
        else if (strcasestr(buf, "y_max=")) {
            myjoy.y_max = atoi(&buf[6]);
            info(SDL"joystick y_max:%d in %s\n", myjoy.y_max, __func__);
        }
        else if (strcasestr(buf, "y_zero=")) {
            myjoy.y_zero = atoi(&buf[7]);
            info(SDL"joystick y_zero:%d in %s\n", myjoy.y_zero, __func__);
        }
    }
    fclose(f);
    return 0;
}

#if defined(UT)
TEST(sdl2_joystick_miyoo, read_joystick_config)
{
    TEST_ASSERT_EQUAL_INT(0, read_joystick_config());
    TEST_ASSERT_EQUAL_INT(UT_X_MIN, myjoy.x_min);
    TEST_ASSERT_EQUAL_INT(UT_X_MAX, myjoy.x_max);
    TEST_ASSERT_EQUAL_INT(UT_X_ZERO, myjoy.x_zero);
    TEST_ASSERT_EQUAL_INT(UT_Y_MIN, myjoy.y_min);
    TEST_ASSERT_EQUAL_INT(UT_Y_MAX, myjoy.y_max);
    TEST_ASSERT_EQUAL_INT(UT_Y_ZERO, myjoy.y_zero);
}
#endif

int joystick_handler(void *param)
{
    int len = 0;
    char rcv_buf[255] = {0};

#if defined(UT)
    myjoy.running = 0;
#endif

    while (myjoy.running) {
        len = uart_read(myjoy.dev_fd, rcv_buf, 99);

        if (len > 0) {
            rcv_buf[len] = 0;
            parse_serial_buf(rcv_buf, len);
        }
        usleep(10000);
    }
    return 0;
}
#endif

#if defined(UT)
TEST(sdl2_joystick_miyoo, joystick_handler)
{
    TEST_ASSERT_EQUAL_INT(0, joystick_handler(NULL));
}
#endif

int JoystickInit(void)
{
#if defined(A30) || defined(UT)
    myjoy.running = 1;
    read_joystick_config();
    init_serial_input();
    myjoy.thread = SDL_CreateThreadInternal(joystick_handler, "miyoo_joystick_thread", 4096, NULL);
    if(myjoy.thread == NULL) {
        err(SDL"failed to create thread for joystick in %s\n", __func__);
        return -1;
    }
#endif

    return 0;
}

#if defined(UT)
TEST(sdl2_joystick_miyoo, JoystickInit)
{
    TEST_ASSERT_EQUAL_INT(0, JoystickInit());
}
#endif

void JoystickQuit(void)
{
#if defined(A30) || defined(UT)
    myjoy.running = 0;
    SDL_WaitThread(myjoy.thread, NULL);

    if (myjoy.dev_fd > 0) {
        close(myjoy.dev_fd);
        myjoy.dev_fd = -1;
    }
#endif
}

#if defined(UT)
TEST(sdl2_joystick_miyoo, JoystickQuit)
{
    JoystickQuit();
    TEST_PASS();
}
#endif

static int JoystickGetCount(void)
{
    return 1;
}

#if defined(UT)
TEST(sdl2_joystick_miyoo, JoystickGetCount)
{
    TEST_ASSERT_EQUAL_INT(1, JoystickGetCount());
}
#endif

static void JoystickDetect(void)
{
}

#if defined(UT)
TEST(sdl2_joystick_miyoo, JoystickDetect)
{
    JoystickDetect();
    TEST_PASS();
}
#endif

static const char* JoystickGetDeviceName(int device_index)
{
    return JOYSTICK_NAME;
}

#if defined(UT)
TEST(sdl2_joystick_miyoo, JoystickGetDeviceName)
{
    TEST_ASSERT_EQUAL_STRING(JOYSTICK_NAME, JoystickGetDeviceName(0));
}
#endif

static int JoystickGetDevicePlayerIndex(int device_index)
{
    return -1;
}

#if defined(UT)
TEST(sdl2_joystick_miyoo, JoystickGetDevicePlayerIndex)
{
    TEST_ASSERT_EQUAL_INT(-1, JoystickGetDevicePlayerIndex(0));
}
#endif

static void JoystickSetDevicePlayerIndex(int device_index, int player_index)
{
}

#if defined(UT)
TEST(sdl2_joystick_miyoo, JoystickSetDevicePlayerIndex)
{
    JoystickSetDevicePlayerIndex(0, 0);
    TEST_PASS();
}
#endif

static SDL_JoystickGUID JoystickGetDeviceGUID(int device_index)
{
    SDL_JoystickGUID guid;
    const char *name = JoystickGetDeviceName(device_index);

    SDL_zero(guid);
    SDL_memcpy(&guid, name, SDL_min(sizeof(guid), SDL_strlen(name)));
    return guid;
}

#if defined(UT)
TEST(sdl2_joystick_miyoo, JoystickGetDeviceGUID)
{
    const int idx = 1;
    SDL_JoystickGUID guid;
    char *dst = (char *)&guid;
    const char *src = JoystickGetDeviceName(idx);

    guid = JoystickGetDeviceGUID(idx);
    TEST_ASSERT_EQUAL(src[0], dst[0]);
}
#endif

static SDL_JoystickID JoystickGetDeviceInstanceID(int device_index)
{
    return device_index;
}

#if defined(UT)
TEST(sdl2_joystick_miyoo, JoystickGetDeviceInstanceID)
{
    const int idx = 1;

    TEST_ASSERT_EQUAL_INT(idx, JoystickGetDeviceInstanceID(idx));
}
#endif

int JoystickOpen(SDL_Joystick *joystick, int device_index)
{
    if (!joystick) {
        err(SDL"invalid parameter(0x%x) in %s\n", joystick, __func__);
        return -1;
    }

    joystick->nbuttons = 18;
    joystick->naxes = 2;
    joystick->nhats = 0;
    joystick->instance_id = device_index;
    return 0;
}

#if defined(UT)
TEST(sdl2_joystick_miyoo, JoystickOpen)
{
    SDL_Joystick j = { 0 };
    const int idx = 1;

    TEST_ASSERT_EQUAL_INT(-1, JoystickOpen(NULL, 0));

    TEST_ASSERT_EQUAL_INT(0, JoystickOpen(&j, idx));
    TEST_ASSERT_EQUAL_INT(18, j.nbuttons);
    TEST_ASSERT_EQUAL_INT(idx, j.instance_id);
}
#endif

void JoystickClose(SDL_Joystick *joystick)
{
    if (!joystick) {
        err(SDL"invalid parameter(0x%x) in %s\n", joystick, __func__);
    }
}

#if defined(UT)
TEST(sdl2_joystick_miyoo, JoystickClose)
{
    SDL_Joystick j = { 0 };

    JoystickClose(NULL);
    JoystickClose(&j);
    TEST_PASS();
}
#endif

static int JoystickRumble(SDL_Joystick *joystick, uint16_t low_frequency_rumble, uint16_t high_frequency_rumble)
{
    if (!joystick) {
        err(SDL"invalid parameter(0x%x) in %s\n", joystick, __func__);
        return -1;
    }
    return 0;
}

#if defined(UT)
TEST(sdl2_joystick_miyoo, JoystickRumble)
{
    SDL_Joystick j = { 0 };

    TEST_ASSERT_EQUAL_INT(-1, JoystickRumble(NULL, 0, 0));

    TEST_ASSERT_EQUAL_INT(0, JoystickRumble(&j, 0, 0));
}
#endif

static int JoystickRumbleTriggers(SDL_Joystick *joystick, uint16_t left_rumble, uint16_t right_rumble)
{
    if (!joystick) {
        err(SDL"invalid parameter(0x%x) in %s\n", joystick, __func__);
        return -1;
    }
    return 0;
}

#if defined(UT)
TEST(sdl2_joystick_miyoo, JoystickRumbleTriggers)
{
    SDL_Joystick j = { 0 };

    TEST_ASSERT_EQUAL_INT(-1, JoystickRumbleTriggers(NULL, 0, 0));

    TEST_ASSERT_EQUAL_INT(0, JoystickRumbleTriggers(&j, 0, 0));
}
#endif

static uint32_t JoystickGetCapabilities(SDL_Joystick *joystick)
{
    if (!joystick) {
        err(SDL"invalid parameter(0x%x) in %s\n", joystick, __func__);
    }
    return 0;
}

#if defined(UT)
TEST(sdl2_joystick_miyoo, JoystickGetCapabilities)
{
    SDL_Joystick j = { 0 };

    TEST_ASSERT_EQUAL_INT(0, JoystickGetCapabilities(NULL));
    TEST_ASSERT_EQUAL_INT(0, JoystickGetCapabilities(&j));
}
#endif

static int JoystickSetLED(SDL_Joystick *joystick, uint8_t red, uint8_t green, uint8_t blue)
{
    if (!joystick) {
        err(SDL"invalid parameter(0x%x) in %s\n", joystick, __func__);
        return -1;
    }
    return 0;
}

#if defined(UT)
TEST(sdl2_joystick_miyoo, JoystickSetLED)
{
    SDL_Joystick j = { 0 };

    TEST_ASSERT_EQUAL_INT(-1, JoystickSetLED(NULL, 0, 0, 0));

    TEST_ASSERT_EQUAL_INT(0, JoystickSetLED(&j, 0, 0, 0));
}
#endif

static int JoystickSendEffect(SDL_Joystick *joystick, const void *data, int size)
{
    if (!joystick || !data) {
        err(SDL"invalid parameter(0x%x, 0x%x) in %s\n", joystick, data, __func__);
        return -1;
    }
    return 0;
}

#if defined(UT)
TEST(sdl2_joystick_miyoo, JoystickSendEffect)
{
    SDL_Joystick j = { 0 };

    TEST_ASSERT_EQUAL_INT(-1, JoystickSendEffect(NULL, NULL, 0));

    TEST_ASSERT_EQUAL_INT(0, JoystickSendEffect(&j, (const void *)0xdeadbeef, 0));
}
#endif

static int JoystickSetSensorsEnabled(SDL_Joystick *joystick, SDL_bool enabled)
{
    if (!joystick) {
        err(SDL"invalid parameter(0x%x) in %s\n", joystick, __func__);
        return -1;
    }
    return 0;
}

#if defined(UT)
TEST(sdl2_joystick_miyoo, JoystickSetSensorsEnabled)
{
    SDL_Joystick j = { 0 };

    TEST_ASSERT_EQUAL_INT(-1, JoystickSetSensorsEnabled(NULL, 0));

    TEST_ASSERT_EQUAL_INT(0, JoystickSetSensorsEnabled(&j, 0));
}
#endif

void JoystickUpdate(SDL_Joystick *joystick)
{
#if defined(A30)
    static int pre_x = -1;
    static int pre_y = -1;
#endif

    do {
        if (!joystick) {
            err(SDL"invalid parameter(0x%x) in %s\n", joystick, __func__);
            break;
        }

#if defined(A30)
        if (last_x != pre_x) {
            pre_x = last_x;
            SDL_PrivateJoystickAxis(joystick, 0, pre_x);
        }

        if (last_y != pre_y) {
            pre_y = last_y;
            SDL_PrivateJoystickAxis(joystick, 1, pre_x);
        }
#endif
    } while (0);
}

#if defined(UT)
TEST(sdl2_joystick_miyoo, JoystickUpdate)
{
    JoystickUpdate(NULL);
    TEST_PASS();
}
#endif

static SDL_bool JoystickGetGamepadMapping(int device_index, SDL_GamepadMapping *out)
{
    if (!out) {
        err(SDL"invalid parameter(0x%x) in %s\n", out, __func__);
    }
    return SDL_FALSE;
}

#if defined(UT)
TEST(sdl2_joystick_miyoo, JoystickGetGamepadMapping)
{
    SDL_GamepadMapping g = { 0 };

    TEST_ASSERT_EQUAL(SDL_FALSE, JoystickGetGamepadMapping(0, NULL));
    TEST_ASSERT_EQUAL(SDL_FALSE, JoystickGetGamepadMapping(0, &g));
}
#endif

SDL_JoystickDriver SDL_MIYOO_JoystickDriver = {
    JoystickInit,
    JoystickGetCount,
    JoystickDetect,
    JoystickGetDeviceName,
    JoystickGetDevicePlayerIndex,
    JoystickSetDevicePlayerIndex,
    JoystickGetDeviceGUID,
    JoystickGetDeviceInstanceID,
    JoystickOpen,
    JoystickRumble,
    JoystickRumbleTriggers,
    JoystickGetCapabilities,
    JoystickSetLED,
    JoystickSendEffect,
    JoystickSetSensorsEnabled,
    JoystickUpdate,
    JoystickClose,
    JoystickQuit,
    JoystickGetGamepadMapping
};

#if defined(UT)
TEST_GROUP_RUNNER(sdl2_joystick_miyoo)
{
    RUN_TEST_CASE(sdl2_joystick_miyoo, uart_open);
    RUN_TEST_CASE(sdl2_joystick_miyoo, uart_set);
    RUN_TEST_CASE(sdl2_joystick_miyoo, uart_init);
    RUN_TEST_CASE(sdl2_joystick_miyoo, uart_read);
    RUN_TEST_CASE(sdl2_joystick_miyoo, filter_dead_zone);
    RUN_TEST_CASE(sdl2_joystick_miyoo, limit_value);
    RUN_TEST_CASE(sdl2_joystick_miyoo, check_axis_event);
    RUN_TEST_CASE(sdl2_joystick_miyoo, frame_to_axis_x);
    RUN_TEST_CASE(sdl2_joystick_miyoo, frame_to_axis_y);
    RUN_TEST_CASE(sdl2_joystick_miyoo, parse_serial_buf);
    RUN_TEST_CASE(sdl2_joystick_miyoo, init_serial_input);
    RUN_TEST_CASE(sdl2_joystick_miyoo, read_joystick_config);
    RUN_TEST_CASE(sdl2_joystick_miyoo, joystick_handler);
    RUN_TEST_CASE(sdl2_joystick_miyoo, JoystickInit);
    RUN_TEST_CASE(sdl2_joystick_miyoo, JoystickQuit);
    RUN_TEST_CASE(sdl2_joystick_miyoo, JoystickGetCount);
    RUN_TEST_CASE(sdl2_joystick_miyoo, JoystickDetect);
    RUN_TEST_CASE(sdl2_joystick_miyoo, JoystickGetDeviceName);
    RUN_TEST_CASE(sdl2_joystick_miyoo, JoystickGetDevicePlayerIndex);
    RUN_TEST_CASE(sdl2_joystick_miyoo, JoystickSetDevicePlayerIndex);
    RUN_TEST_CASE(sdl2_joystick_miyoo, JoystickGetDeviceGUID);
    RUN_TEST_CASE(sdl2_joystick_miyoo, JoystickGetDeviceInstanceID);
    RUN_TEST_CASE(sdl2_joystick_miyoo, JoystickOpen);
    RUN_TEST_CASE(sdl2_joystick_miyoo, JoystickClose);
    RUN_TEST_CASE(sdl2_joystick_miyoo, JoystickRumble);
    RUN_TEST_CASE(sdl2_joystick_miyoo, JoystickRumbleTriggers);
    RUN_TEST_CASE(sdl2_joystick_miyoo, JoystickGetCapabilities);
    RUN_TEST_CASE(sdl2_joystick_miyoo, JoystickSetLED);
    RUN_TEST_CASE(sdl2_joystick_miyoo, JoystickSendEffect);
    RUN_TEST_CASE(sdl2_joystick_miyoo, JoystickSetSensorsEnabled);
    RUN_TEST_CASE(sdl2_joystick_miyoo, JoystickUpdate);
    RUN_TEST_CASE(sdl2_joystick_miyoo, JoystickGetGamepadMapping);
}
#endif

