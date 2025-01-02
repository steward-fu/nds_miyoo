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

#ifndef __SND_H__
#define __SND_H__

#define PCM_FREQ            44100
#define PCM_PERIOD          2048
#define PCM_CHANNELS        2
#define PCM_SAMPLES         8192

#define MAX_VOLUME          20
#define MI_DEV              "/dev/mi_ao"
#define DSP_DEV             "/dev/dsp"
#define MIN_RAW_VALUE       -60
#define MAX_RAW_VALUE       30

#define MI_AO_SETVOLUME     0x4008690b
#define MI_AO_GETVOLUME     0xc008690c
#define MI_AO_SETMUTE       0x4008690d

#define DEF_QUEUE_SIZE      (2 * 1024 * 1024)
#define DEF_PCM_AVAIL       2048

typedef struct {
    size_t size;
    size_t read;
    size_t write;
    uint8_t *buffer;
    pthread_mutex_t lock;
} queue_t;

#endif

