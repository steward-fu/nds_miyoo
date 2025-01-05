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
#include <sys/ioctl.h>
#include <alsa/output.h>
#include <alsa/input.h>
#include <alsa/conf.h>
#include <alsa/global.h>
#include <alsa/timer.h>
#include <alsa/pcm.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/time.h>
#include <syslog.h>
#include <linux/rtc.h>
#include <linux/soundcard.h>

#if defined(UT)
#include "unity_fixture.h"
#endif

#include "cfg.h"
#include "log.h"
#include "snd.h"
#include "hook.h"
#include "drastic.h"
#include "cfg.pb.h"

#if defined(MINI)
#include "mi_ao.h"
#include "mi_sys.h"
#include "mi_common_datatype.h"
#endif

extern settings cfg;

static queue_t queue = {0};
static pthread_t thread = 0;

#if defined(MINI)
static MI_AO_CHN AoChn = 0;
static MI_AUDIO_DEV AoDevId = 0;
static MI_AUDIO_Attr_t stSetAttr = {0};
static MI_AUDIO_Attr_t stGetAttr = {0};
#endif

#if defined(A30) || defined(UT)
int mem_fd = -1;
int vol_mul = 1;
int vol_base = 100;
uint8_t *mem_ptr = NULL;
uint32_t *vol_ptr = NULL;
static int dsp_fd = -1;
#endif

static int pcm_ready = 0;
static int pcm_buf_len = 0;
static uint8_t *pcm_buf = NULL;

static int queue_init(queue_t *q, size_t size);
static int queue_destroy(queue_t *q);
static int queue_put(queue_t *q, uint8_t *buffer, size_t size);

#if defined(UT)
TEST_GROUP(alsa_snd);

TEST_SETUP(alsa_snd)
{
    FILE *f = NULL;

    f = fopen(JSON_SYS_FILE, "w+");
    if (f) {
        fprintf(f, "{\n");
        fprintf(f, "\"vol\": 100\n");
        fprintf(f, "}");
        fclose(f);
    }
}

TEST_TEAR_DOWN(alsa_snd)
{
    unlink(JSON_SYS_FILE);
}
#endif

static void spu_adpcm_decode_block(spu_channel_struct *channel)
{
    uint32_t uVar1 = 0;
    uint32_t uVar2 = 0;
    uint32_t uVar3 = 0;
    uint32_t uVar4 = 0;
    uint32_t sample_delta = 0;
    uint32_t current_index = 0;
    uint32_t adpcm_data_x8 = 0;
    uint32_t adpcm_cache_block_offset = 0;
    uint32_t adpcm_step = 0;
    uint32_t uVar5 = 0;
    int32_t sample = 0;
    int16_t *psVar6 = NULL;
    int16_t *psVar7 = NULL;
    int16_t *adpcm_step_table = (int16_t *)get_adpcm_step_table();
    int8_t *adpcm_index_step_table = (int8_t *)get_adpcm_index_step_table();


    do {
        if (!channel) {
            err(SND"invalid parameter(0x%x) in %s\n", channel, __func__);
            break;
        }

        uVar3 = channel->adpcm_cache_block_offset;
        uVar1 = (uint32_t)(channel->adpcm_current_index);
        sample_delta = (uint32_t)(channel->adpcm_sample);
        uVar2 = *((uint32_t *)(channel->samples + (uVar3 >> 1)));
        channel->adpcm_cache_block_offset = uVar3 + 8;
        psVar7 = channel->adpcm_sample_cache + (uVar3 & 0x3f);
        do {
            uVar5 = (uint32_t)adpcm_step_table[uVar1];
            uVar4 = uVar5 >> 3;
            if ((uVar2 & 1) != 0) {
                uVar4 = uVar4 + (uVar5 >> 2);
            }
            if ((uVar2 & 2) != 0) {
                uVar4 = uVar4 + (uVar5 >> 1);
            }
            if ((uVar2 & 4) != 0) {
                uVar4 = uVar4 + uVar5;
            }
            if ((uVar2 & 8) == 0) {
                sample_delta = sample_delta - uVar4;
                if ((int)sample_delta < -0x7fff) {
                    sample_delta = 0xffff8001;
                }
            }
            else {
                sample_delta = sample_delta + uVar4;
                if (0x7ffe < (int)sample_delta) {
                    sample_delta = 0x7fff;
                }
            }
            uVar1 = uVar1 + (int)(adpcm_index_step_table[uVar2 & 7]);
            if (0x58 < uVar1) {
                if ((int)uVar1 < 0) {
                    uVar1 = 0;
                }
                else {
                    uVar1 = 0x58;
                }
            }
            uVar2 = uVar2 >> 4;
            psVar6 = psVar7 + 1;
            *psVar7 = (int16_t)sample_delta;
            psVar7 = psVar6;
        } while (channel->adpcm_sample_cache + (uVar3 & 0x3f) + 8 != psVar6);
        channel->adpcm_sample = (int16_t)sample_delta;
        channel->adpcm_current_index = (uint8_t)uVar1;
    } while(0);
}

#if defined(UT)
TEST(alsa_snd, spu_adpcm_decode_block)
{
    spu_adpcm_decode_block(NULL);
    TEST_PASS();
}
#endif

#if defined(MINI) || defined(UT)
static int set_volume_raw(int value, int add)
{
    int fd = -1;
    int prev_value = 0;
    int buf2[] = { 0, 0 };
    uint64_t buf1[] = { sizeof(buf2), (uintptr_t)buf2 };

    fd = open(MI_DEV, O_RDWR);
    if (fd < 0) {
        err(SND"failed to open "MI_DEV" in %s\n", __func__);
        return -1;
    }

    if (ioctl(fd, MI_AO_GETVOLUME, buf1) < 0) {
        err(SND"failed to get volume in %s\n", __func__);
        return -1;
    }
    prev_value = buf2[1];

    if (add) {
        value = prev_value + add;
    }
    else {
        value += MIN_RAW_VALUE;
    }

    if (value > MAX_RAW_VALUE) {
        value = MAX_RAW_VALUE;
    }
    else if (value < MIN_RAW_VALUE) {
        value = MIN_RAW_VALUE;
    }

    if (value == prev_value) {
        close(fd);
        warn(SND"volume is same as previous value in %s\n", __func__);
        return prev_value;
    }

    buf2[1] = value;
    if (ioctl(fd, MI_AO_SETVOLUME, buf1) < 0) {
        err(SND"failed to set volume(%d) in %s\n", buf2[1], __func__);
        return -1;
    }

    if (prev_value <= MIN_RAW_VALUE && value > MIN_RAW_VALUE) {
        buf2[1] = 0;
        if (ioctl(fd, MI_AO_SETMUTE, buf1) < 0) {
            err(SND"failed to set mute(%d) in $s\n", buf2[1], __func__);
        }
    }
    else if (prev_value > MIN_RAW_VALUE && value <= MIN_RAW_VALUE) {
        buf2[1] = 1;
        if (ioctl(fd, MI_AO_SETMUTE, buf1) < 0) {
            err(SND"failed to set mute(%d) in\n", buf2[1], __func__);
            return -1;
        }
    }
    close(fd);

    info(SND"new volume is %d in %s\n", value, __func__);
    return value;
}

#if defined(UT)
TEST(alsa_snd, set_volume_raw)
{
    TEST_ASSERT_EQUAL_INT(-1, set_volume_raw(0, 0));
}
#endif

static int set_volume(int volume)
{
    int volume_raw = 0;
    int div = cfg.half_volume ? 2 : 1;

    if (volume > MAX_VOLUME) {
        volume = MAX_VOLUME;
    }
    else if (volume < 0) {
        volume = 0;
    }

    if (volume != 0) {
        volume_raw = round(48 * log10(1 + volume));
    }

    if (set_volume_raw(volume_raw / div, 0) < 0) {
        return -1;
    }
    return volume;
}

#if defined(UT)
TEST(alsa_snd, set_volume)
{
    TEST_ASSERT_EQUAL_INT(-1, set_volume(-1));
    TEST_ASSERT_EQUAL_INT(-1, set_volume(0));
    TEST_ASSERT_EQUAL_INT(-1, set_volume(MAX_VOLUME + 1));
}
#endif
#endif

int volume_inc(void)
{
    int vol = cfg.system_volume;

    if (vol < MAX_VOLUME) {
        vol += 1;

#if defined(MINI)
        set_volume(vol);
#endif

#if defined(A30)
        *vol_ptr = ((vol_base + (vol << vol_mul)) << 8) | (vol_base + (vol << vol_mul));
#endif
        set_system_volume(vol);
    }

    return vol;
}

#if defined(UT)
TEST(alsa_snd, volume_inc)
{
    set_system_volume(0);
    TEST_ASSERT_EQUAL_INT(1, volume_inc());
}
#endif

int volume_dec(void)
{
    int vol = cfg.system_volume;

    if (vol > 0) {
        vol -= 1;

#if defined(MINI)
        set_volume(vol);
#endif

#if defined(A30)
        if (vol == 0) {
            *vol_ptr = 0;
        }
        else {
            *vol_ptr = ((vol_base + (vol << vol_mul)) << 8) | (vol_base + (vol << vol_mul));
        }
#endif
        set_system_volume(vol);
    }

    return vol;
}

#if defined(UT)
TEST(alsa_snd, volume_dec)
{
    set_system_volume(1);
    TEST_ASSERT_EQUAL_INT(0, volume_dec());
}
#endif

#if defined(A30) || defined(UT)
static int open_dsp(void)
{
    int arg = 0;
    int vol = cfg.system_volume;

    if (dsp_fd > 0) {
        close(dsp_fd);
        warn(SND"reopened "DSP_DEV" in %s\n", __func__);
    }

    dsp_fd = open(DSP_DEV, O_WRONLY);
    if (dsp_fd < 0) {
        err(SND"failed to open "DSP_DEV" in %s\n", __func__);
        return -1;
    }

    if (system("amixer set \'DACL Mixer AIF1DA0L\' on") < 0) {
        warn(SND"failed to set amixer AIF1DA0L in %s\n", __func__);
    }

    if (system("amixer set \'DACL Mixer AIF1DA0R\' on") < 0) {
        warn(SND"failed to set amixer AIF1DA0R in %s\n", __func__);
    }

    vol_ptr = (uint32_t *)(&mem_ptr[0xc00 + 0x258]);
    *vol_ptr = ((vol_base + (vol << vol_mul)) << 8) | (vol_base + (vol << vol_mul));

    arg = 16;
    if (ioctl(dsp_fd, SOUND_PCM_WRITE_BITS, &arg) < 0) {
        err(SND"failed to set PCM bits in %s\n", __func__);
        return -1;
    }

    arg = PCM_CHANNELS;
    if (ioctl(dsp_fd, SOUND_PCM_WRITE_CHANNELS, &arg) < 0) {
        err(SND"failed to set PCM channels in %s\n", __func__);
        return -1;
    }

    arg = PCM_FREQ;
    if (ioctl(dsp_fd, SOUND_PCM_WRITE_RATE, &arg) < 0) {
        err(SND"failed to set PCM rate in %s\n", __func__);
        return -1;
    }
    return 0;
}
#endif

#if defined(UT)
TEST(alsa_snd, open_dsp)
{
    TEST_ASSERT_EQUAL_INT(-1, open_dsp());
}
#endif

static int queue_init(queue_t *q, size_t s)
{
    q->buffer = (uint8_t *)malloc(s);
    q->size = s;
    q->read = q->write = 0;
    pthread_mutex_init(&q->lock, NULL);
    return 0;
}

#if defined(UT)
TEST(alsa_snd, queue_init)
{
    queue_t t = {0};
    const int size = 1024;

    TEST_ASSERT_EQUAL_INT(0, queue_init(&t, size));
    TEST_ASSERT_NOT_NULL(t.buffer);
    TEST_ASSERT_EQUAL_INT(0, t.read);
    TEST_ASSERT_EQUAL_INT(0, t.write);
    TEST_ASSERT_EQUAL_INT(size, t.size);
    TEST_ASSERT_EQUAL_INT(0, queue_destroy(&t));
}
#endif

static int queue_destroy(queue_t *q)
{
    if (q->buffer) {
        free(q->buffer);
        q->buffer = NULL;
    }
    pthread_mutex_destroy(&q->lock);
    return 0;
}

#if defined(UT)
TEST(alsa_snd, queue_destroy)
{
    queue_t t = {0};
    const int size = 1024;

    TEST_ASSERT_EQUAL_INT(0, queue_init(&t, size));
    TEST_ASSERT_NOT_NULL(t.buffer);
    TEST_ASSERT_EQUAL_INT(0, queue_destroy(&t));
    TEST_ASSERT_NULL(t.buffer);
}
#endif

static int queue_size_for_read(queue_t *q)
{
    if (q->read == q->write) {
        return 0;
    }
    else if(q->read < q->write){
        return q->write - q->read;
    }
    return (q->size - q->read) + q->write;
}

#if defined(UT)
TEST(alsa_snd, queue_size_for_read)
{
    queue_t t = {0};
    char buf[128] = {0};
    const int size = 1024;

    TEST_ASSERT_EQUAL_INT(0, queue_init(&t, size));
    TEST_ASSERT_NOT_NULL(t.buffer);

    TEST_ASSERT_EQUAL_INT(sizeof(buf), queue_put(&t, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_INT(sizeof(buf), queue_size_for_read(&t));

    TEST_ASSERT_EQUAL_INT(0, queue_destroy(&t));
    TEST_ASSERT_NULL(t.buffer);
}
#endif

static int queue_size_for_write(queue_t *q)
{
    if (q->write == q->read) {
        return q->size;
    }
    else if(q->write < q->read){
        return q->read - q->write;
    }
    return (q->size - q->write) + q->read;
}

#if defined(UT)
TEST(alsa_snd, queue_size_for_write)
{
    queue_t t = {0};
    char buf[128] = {0};
    const int size = 1024;

    TEST_ASSERT_EQUAL_INT(0, queue_init(&t, size));
    TEST_ASSERT_NOT_NULL(t.buffer);

    TEST_ASSERT_EQUAL_INT(sizeof(buf), queue_put(&t, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_INT(size - sizeof(buf), queue_size_for_write(&t));

    TEST_ASSERT_EQUAL_INT(0, queue_destroy(&t));
    TEST_ASSERT_NULL(t.buffer);
}
#endif

static int queue_put(queue_t *q, uint8_t *buffer, size_t size)
{
    int r = 0, tmp = 0, avai = 0;

    if (!q || !buffer || (size < 0)) {
        err(SND"invalid parameter(0x%x, 0x%x, 0x%x) in %s\n", q, buffer, size, __func__);
        return -1;
    }

    if (size > q->size) {
        err(SND"buffer too large(%ld) in %s\n", size, __func__);
        return -1;
    }

    if (size == 0) {
        return 0;
    }

    pthread_mutex_lock(&q->lock);
    avai = queue_size_for_write(q);
    if (size > avai) {
        size = avai;
    }
    r = size;

    if (size > 0) {
        if ((q->write >= q->read) && ((q->write + size) > q->size)) {
            tmp = q->size - q->write;
            size-= tmp;
#if defined(UT)
            memcpy(&q->buffer[q->write], buffer, tmp);
            memcpy(q->buffer, &buffer[tmp], size);
#else
            neon_memcpy(&q->buffer[q->write], buffer, tmp);
            neon_memcpy(q->buffer, &buffer[tmp], size);
#endif
            q->write = size;
        }
        else {
#if defined(UT)
            memcpy(&q->buffer[q->write], buffer, size);
#else
            neon_memcpy(&q->buffer[q->write], buffer, size);
#endif
            q->write += size;
        }
    }
    pthread_mutex_unlock(&q->lock);
    return r;
}

#if defined(UT)
TEST(alsa_snd, queue_put)
{
    queue_t t = {0};
    char buf[128] = {0};
    const int size = 1024;

    TEST_ASSERT_EQUAL_INT(0, queue_init(&t, size));
    TEST_ASSERT_NOT_NULL(t.buffer);

    TEST_ASSERT_EQUAL_INT(-1, queue_put(NULL, NULL, 0));
    TEST_ASSERT_EQUAL_INT(-1, queue_put(&t, NULL, 0));
    TEST_ASSERT_EQUAL_INT(0, queue_put(&t, buf, 0));

    TEST_ASSERT_EQUAL_INT(sizeof(buf), queue_put(&t, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_INT(sizeof(buf), queue_put(&t, buf, sizeof(buf)));

    TEST_ASSERT_EQUAL_INT(0, queue_destroy(&t));
    TEST_ASSERT_NULL(t.buffer);
}
#endif

static size_t queue_get(queue_t *q, uint8_t *buffer, size_t max_size)
{
    int r = 0, tmp = 0, avai = 0, size = max_size;

    if (!q || !buffer || (max_size < 0)) {
        err(SND"invalid parameter(0x%x, 0x%x, 0x%x) in %s\n", q, buffer, size, __func__);
        return -1;
    }

    if (max_size == 0) {
        return 0;
    }

    pthread_mutex_lock(&q->lock);
    avai = queue_size_for_read(q);
    if (size > avai) {
        size = avai;
    }
    r = size;

    if (size > 0) {
        if ((q->read > q->write) && (q->read + size) > q->size) {
            tmp = q->size - q->read;
            size-= tmp;
#if defined(UT)
            memcpy(buffer, &q->buffer[q->read], tmp);
            memcpy(&buffer[tmp], q->buffer, size);
#else
            neon_memcpy(buffer, &q->buffer[q->read], tmp);
            neon_memcpy(&buffer[tmp], q->buffer, size);
#endif
            q->read = size;
        }
        else {
#if defined(UT)
            memcpy(buffer, &q->buffer[q->read], size);
#else
            neon_memcpy(buffer, &q->buffer[q->read], size);
#endif
            q->read+= size;
        }
    }
    pthread_mutex_unlock(&q->lock);
    return r;
}

#if defined(UT)
TEST(alsa_snd, queue_get)
{
    queue_t t = {0};
    char buf[128] = {0};
    const int size = 1024;

    TEST_ASSERT_EQUAL_INT(0, queue_init(&t, size));
    TEST_ASSERT_NOT_NULL(t.buffer);

    TEST_ASSERT_EQUAL_INT(-1, queue_get(NULL, NULL, 0));
    TEST_ASSERT_EQUAL_INT(-1, queue_get(&t, NULL, 0));
    TEST_ASSERT_EQUAL_INT(0, queue_get(&t, buf, 0));

    TEST_ASSERT_EQUAL_INT(0, queue_get(&t, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_INT(sizeof(buf), queue_put(&t, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_INT(sizeof(buf), queue_get(&t, buf, sizeof(buf)));

    TEST_ASSERT_EQUAL_INT(0, queue_destroy(&t));
    TEST_ASSERT_NULL(t.buffer);
}
#endif

static void *audio_handler(void *threadid)
{
#if defined(MINI)
    MI_AUDIO_Frame_t aoTestFrame = {0};
#endif

#if defined(A30)
    int chk_cnt = 0;
#endif

    int r = 0;
    int idx = 0;
    int len = pcm_buf_len;

#if defined(UT)
    return NULL;
#endif

    while (pcm_ready) {
        r = queue_get(&queue, &pcm_buf[idx], len);
        if (r > 0) {
            idx+= r;
            len-= r;
            if (len == 0) {
                idx = 0;
                len = pcm_buf_len;
#if defined(MINI)
                aoTestFrame.eBitwidth = stGetAttr.eBitwidth;
                aoTestFrame.eSoundmode = stGetAttr.eSoundmode;
                aoTestFrame.u32Len = pcm_buf_len;
                aoTestFrame.apVirAddr[0] = pcm_buf;
                aoTestFrame.apVirAddr[1] = NULL;
                MI_AO_SendFrame(AoDevId, AoChn, &aoTestFrame, 1);
#endif

#if defined(A30)
                write(dsp_fd, pcm_buf, pcm_buf_len);
#endif
            }
        }
#if defined(A30)
        else {
            if (chk_cnt == 0) {
                char buf[255] = {0};
                FILE *fd = popen("amixer get \'DACL Mixer AIF1DA0L\' | grep \"Mono: Playback \\[off\\]\" | wc -l", "r");

                if (fd) {
                    fgets(buf, sizeof(buf), fd);
                    pclose(fd);

                    if (atoi(buf) > 0) {
                        open_dsp();
                    }
                }
                chk_cnt = 30000;
            }
            chk_cnt -= 1;
        }
#endif
        usleep(10);
    }
    pthread_exit(NULL);
}

#if defined(UT)
TEST(alsa_snd, audio_handler)
{
    TEST_ASSERT_NULL(audio_handler(0));
}
#endif

snd_pcm_sframes_t snd_pcm_avail(snd_pcm_t *pcm)
{
    return DEF_PCM_AVAIL;
}

#if defined(UT)
TEST(alsa_snd, snd_pcm_avail)
{
    TEST_ASSERT_EQUAL_INT(DEF_PCM_AVAIL, snd_pcm_avail(NULL));
}
#endif

int snd_pcm_hw_params(snd_pcm_t *pcm, snd_pcm_hw_params_t *params)
{
    return 0;
}

#if defined(UT)
TEST(alsa_snd, snd_pcm_hw_params)
{
    TEST_ASSERT_EQUAL_INT(0, snd_pcm_hw_params(NULL, NULL));
}
#endif

int snd_pcm_hw_params_any(snd_pcm_t *pcm, snd_pcm_hw_params_t *params)
{
    return 0;
}

#if defined(UT)
TEST(alsa_snd, snd_pcm_hw_params_any)
{
    TEST_ASSERT_EQUAL_INT(0, snd_pcm_hw_params_any(NULL, NULL));
}
#endif

void snd_pcm_hw_params_free(snd_pcm_hw_params_t *obj)
{
}

#if defined(UT)
TEST(alsa_snd, snd_pcm_hw_params_free)
{
    snd_pcm_hw_params_free(NULL);
    TEST_PASS();
}
#endif

int snd_pcm_hw_params_malloc(snd_pcm_hw_params_t **ptr)
{
    return 0;
}

#if defined(UT)
TEST(alsa_snd, snd_pcm_hw_params_malloc)
{
    TEST_ASSERT_EQUAL_INT(0, snd_pcm_hw_params_malloc(NULL));
}
#endif

int snd_pcm_hw_params_set_access(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_access_t _access)
{
    return 0;
}

#if defined(UT)
TEST(alsa_snd, snd_pcm_hw_params_set_access)
{
    TEST_ASSERT_EQUAL_INT(0, snd_pcm_hw_params_set_access(NULL, NULL, 0));
}
#endif

int snd_pcm_hw_params_set_buffer_size_near(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_uframes_t *val)
{
    *val = PCM_SAMPLES * 2 * PCM_CHANNELS;
    return 0;
}

#if defined(UT)
TEST(alsa_snd, snd_pcm_hw_params_set_buffer_size_near)
{
    snd_pcm_uframes_t t = { 0 };

    TEST_ASSERT_EQUAL_INT(0, snd_pcm_hw_params_set_buffer_size_near(NULL, NULL, &t));
    TEST_ASSERT_EQUAL_INT(PCM_SAMPLES * 2 * PCM_CHANNELS, t);
}
#endif

int snd_pcm_hw_params_set_channels(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int val)
{
    return 0;
}

#if defined(UT)
TEST(alsa_snd, snd_pcm_hw_params_set_channels)
{
    TEST_ASSERT_EQUAL_INT(0, snd_pcm_hw_params_set_channels(NULL, NULL, 0));
}
#endif

int snd_pcm_hw_params_set_format(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_format_t val)
{
    if (val != SND_PCM_FORMAT_S16_LE) {
        return -1;
    }
    return 0;
}

#if defined(UT)
TEST(alsa_snd, snd_pcm_hw_params_set_format)
{
    TEST_ASSERT_EQUAL_INT(-1, snd_pcm_hw_params_set_format(NULL, NULL, 0));
    TEST_ASSERT_EQUAL_INT(0, snd_pcm_hw_params_set_format(NULL, NULL, SND_PCM_FORMAT_S16_LE));
}
#endif

int snd_pcm_hw_params_set_period_size_near(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_uframes_t *val, int *dir)
{
    *val = PCM_PERIOD;
    return 0;
}

#if defined(UT)
TEST(alsa_snd, snd_pcm_hw_params_set_period_size_near)
{
    snd_pcm_uframes_t t = { 0 };

    TEST_ASSERT_EQUAL_INT(0, snd_pcm_hw_params_set_period_size_near(NULL, NULL, &t, NULL));
    TEST_ASSERT_EQUAL_INT(PCM_PERIOD, t);
}
#endif

int snd_pcm_hw_params_set_rate_near(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val, int *dir)
{
    *val = PCM_FREQ;
    return 0;
}

#if defined(UT)
TEST(alsa_snd, snd_pcm_hw_params_set_rate_near)
{
    unsigned int t = 0;

    TEST_ASSERT_EQUAL_INT(0, snd_pcm_hw_params_set_rate_near(NULL, NULL, &t, NULL));
    TEST_ASSERT_EQUAL_INT(PCM_FREQ, t);
}
#endif

int snd_pcm_open(snd_pcm_t **pcm, const char *name, snd_pcm_stream_t stream, int mode)
{
    if (stream != SND_PCM_STREAM_PLAYBACK) {
        err(SND"steam format is not equal to SND_PCM_STREAM_PLAYBACK in %s\n", __func__);
        return -1;
    }

    info(SND"use customized ALSA library in %s\n", __func__);
    return 0;
}

#if defined(UT)
TEST(alsa_snd, snd_pcm_open)
{
    TEST_ASSERT_EQUAL_INT(-1, snd_pcm_open(NULL, NULL, -1, 0));
    TEST_ASSERT_EQUAL_INT(0, snd_pcm_open(NULL, NULL, SND_PCM_STREAM_PLAYBACK, 0));
}
#endif

int snd_pcm_prepare(snd_pcm_t *pcm)
{
    return 0;
}

#if defined(UT)
TEST(alsa_snd, snd_pcm_prepare)
{
    TEST_ASSERT_EQUAL_INT(0, snd_pcm_prepare(NULL));
}
#endif

snd_pcm_sframes_t snd_pcm_readi(snd_pcm_t *pcm, void *buffer, snd_pcm_uframes_t size)
{
    return 0;
}

#if defined(UT)
TEST(alsa_snd, snd_pcm_readi)
{
    TEST_ASSERT_EQUAL_INT(0, snd_pcm_readi(NULL, NULL, 0));
}
#endif

int snd_pcm_recover(snd_pcm_t *pcm, int err, int silent)
{
    return 0;
}

#if defined(UT)
TEST(alsa_snd, snd_pcm_recover)
{
    TEST_ASSERT_EQUAL_INT(0, snd_pcm_recover(NULL, 0, 0));
}
#endif

int snd_pcm_start(snd_pcm_t *pcm)
{
#if defined(MINI)
    MI_S32 miret = 0;
    MI_SYS_ChnPort_t stAoChn0OutputPort0;
#endif

#if defined(A30)
    int arg = 0;
    struct tm ct = {0};
#endif

    queue_init(&queue, DEF_QUEUE_SIZE);
    if (NULL == queue.buffer) {
        err(SND"failed to allocate buffer for queue in %s\n", __func__);
        return -1;
    }
    memset(queue.buffer, 0, DEF_QUEUE_SIZE);

    pcm_buf_len = PCM_SAMPLES * 2 * PCM_CHANNELS;
    pcm_buf = malloc(pcm_buf_len);
    if (NULL == pcm_buf) {
        err(SND"failed to allocate buffer for pcm_buf in %s\n", __func__);
        return -1;
    }
    memset(pcm_buf, 0, pcm_buf_len);

#if defined(MINI)
    stSetAttr.eBitwidth = E_MI_AUDIO_BIT_WIDTH_16;
    stSetAttr.eWorkmode = E_MI_AUDIO_MODE_I2S_MASTER;
    stSetAttr.u32FrmNum = 6;
    stSetAttr.u32PtNumPerFrm = SAMPLES;
    stSetAttr.u32ChnCnt = CHANNELS;
    stSetAttr.eSoundmode = CHANNELS == 2 ? E_MI_AUDIO_SOUND_MODE_STEREO : E_MI_AUDIO_SOUND_MODE_MONO;
    stSetAttr.eSamplerate = (MI_AUDIO_SampleRate_e)FREQ;

    miret = MI_AO_SetPubAttr(AoDevId, &stSetAttr);
    if(MI_SUCCESS != miret) {
        err(SND"failed to set PubAttr in %s\n", __func__);
        return -1;
    }

    miret = MI_AO_GetPubAttr(AoDevId, &stGetAttr);
    if(MI_SUCCESS != miret) {
        err(SND"failed to get PubAttr in %s\n", __func__);
        return -1;
    }

    miret = MI_AO_Enable(AoDevId);
    if(MI_SUCCESS != miret) {
        err(SND"failed to enable AO in %s\n", __func__);
        return -1;
    }

    miret = MI_AO_EnableChn(AoDevId, AoChn);
    if(MI_SUCCESS != miret) {
        err(SND"failed to enable Channel in %s\n", __func__);
        return -1;
    }

    stAoChn0OutputPort0.eModId = E_MI_MODULE_ID_AO;
    stAoChn0OutputPort0.u32DevId = AoDevId;
    stAoChn0OutputPort0.u32ChnId = AoChn;
    stAoChn0OutputPort0.u32PortId = 0;
    MI_SYS_SetChnOutputPortDepth(&stAoChn0OutputPort0, 12, 13);
    if (set_volume(cur_volume) < 0) {
        return -1;
    }
#endif

#if defined(A30)
    mem_fd = open("/dev/mem", O_RDWR);
    mem_ptr = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, 0x1c22000);
    if (open_dsp() < 0) {
        return -1;
    }
#endif

#if !defined(UT)
    if (add_adpcm_decode_hook(spu_adpcm_decode_block) < 0) {
        err(SND"failed to hook adpcm decode in %s\n", __func__);
        return -1;
    }
    info(SND"added spu hooking successfully\n");
#endif

    pcm_ready = 1;
    info(SND"customized ALSA library is ready in %s\n", __func__);
    pthread_create(&thread, NULL, audio_handler, (void *)NULL);
    return 0;
}

#if defined(UT)
TEST(alsa_snd, snd_pcm_start)
{
    TEST_ASSERT_EQUAL_INT(0, snd_pcm_start(NULL));
}
#endif

int snd_pcm_close(snd_pcm_t *pcm)
{
    void *ret = NULL;

    if (cfg.autosave.enable > 0) {
        invoke_drastic_save_state(cfg.autosave.slot);
    }

    pcm_ready = 0;
    pthread_join(thread, &ret);
    if (pcm_buf != NULL) {
        free(pcm_buf);
        pcm_buf = NULL;
    }
    queue_destroy(&queue);

#if defined(MINI)
    MI_AO_DisableChn(AoDevId, AoChn);
    MI_AO_Disable(AoDevId);
#endif

#if defined(A30)
    if (dsp_fd > 0) {
        close(dsp_fd);
        dsp_fd = -1;
    }

    *vol_ptr = (160 << 8) | 160;
    munmap(mem_ptr, 4096);
    close(mem_fd);
#endif

    info(SND"stop customized ALSA library in %s\n", __func__);
    return 0;
}

#if defined(UT)
TEST(alsa_snd, snd_pcm_close)
{
    TEST_ASSERT_EQUAL_INT(0, snd_pcm_close(NULL));
}
#endif

int snd_pcm_sw_params(snd_pcm_t *pcm, snd_pcm_sw_params_t *params)
{
    return 0;
}

#if defined(UT)
TEST(alsa_snd, snd_pcm_sw_params)
{
    TEST_ASSERT_EQUAL_INT(0, snd_pcm_sw_params(NULL, NULL));
}
#endif

int snd_pcm_sw_params_current(snd_pcm_t *pcm, snd_pcm_sw_params_t *params)
{
    return 0;
}

#if defined(UT)
TEST(alsa_snd, snd_pcm_sw_params_current)
{
    TEST_ASSERT_EQUAL_INT(0, snd_pcm_sw_params_current(NULL, NULL));
}
#endif

void snd_pcm_sw_params_free(snd_pcm_sw_params_t *obj)
{
}

#if defined(UT)
TEST(alsa_snd, snd_pcm_sw_params_free)
{
    snd_pcm_sw_params_free(NULL);
    TEST_PASS();
}
#endif

int snd_pcm_sw_params_malloc(snd_pcm_sw_params_t **ptr)
{
    return 0;
}

#if defined(UT)
TEST(alsa_snd, snd_pcm_sw_params_malloc)
{
    TEST_ASSERT_EQUAL_INT(0, snd_pcm_sw_params_malloc(NULL));
}
#endif

snd_pcm_sframes_t snd_pcm_writei(snd_pcm_t *pcm, const void *buffer, snd_pcm_uframes_t size)
{
    if ((size > 1) && (size != pcm_buf_len)) {
#if !defined(UT)
        queue_put(&queue, (uint8_t*)buffer, size * 2 * CHANNELS);
#endif
    }
    return size;
}

#if defined(UT)
TEST(alsa_snd, snd_pcm_writei)
{
    char buf[128] = { 0 };

    TEST_ASSERT_EQUAL_INT(0, snd_pcm_writei(NULL, buf, 0));
    TEST_ASSERT_EQUAL_INT(1, snd_pcm_writei(NULL, buf, 1));
    TEST_ASSERT_EQUAL_INT(sizeof(buf), snd_pcm_writei(NULL, buf, sizeof(buf)));
}
#endif

#if defined(UT)
TEST_GROUP_RUNNER(alsa_snd)
{
    RUN_TEST_CASE(alsa_snd, spu_adpcm_decode_block);
    RUN_TEST_CASE(alsa_snd, set_volume_raw);
    RUN_TEST_CASE(alsa_snd, set_volume);
    RUN_TEST_CASE(alsa_snd, volume_inc);
    RUN_TEST_CASE(alsa_snd, volume_dec);
    RUN_TEST_CASE(alsa_snd, open_dsp);
    RUN_TEST_CASE(alsa_snd, queue_init);
    RUN_TEST_CASE(alsa_snd, queue_destroy);
    RUN_TEST_CASE(alsa_snd, queue_size_for_read);
    RUN_TEST_CASE(alsa_snd, queue_size_for_write);
    RUN_TEST_CASE(alsa_snd, queue_put);
    RUN_TEST_CASE(alsa_snd, queue_get);
    RUN_TEST_CASE(alsa_snd, audio_handler);
    RUN_TEST_CASE(alsa_snd, snd_pcm_avail);
    RUN_TEST_CASE(alsa_snd, snd_pcm_hw_params);
    RUN_TEST_CASE(alsa_snd, snd_pcm_hw_params_any);
    RUN_TEST_CASE(alsa_snd, snd_pcm_hw_params_free);
    RUN_TEST_CASE(alsa_snd, snd_pcm_hw_params_malloc);
    RUN_TEST_CASE(alsa_snd, snd_pcm_hw_params_set_access);
    RUN_TEST_CASE(alsa_snd, snd_pcm_hw_params_set_buffer_size_near);
    RUN_TEST_CASE(alsa_snd, snd_pcm_hw_params_set_channels);
    RUN_TEST_CASE(alsa_snd, snd_pcm_hw_params_set_format);
    RUN_TEST_CASE(alsa_snd, snd_pcm_hw_params_set_period_size_near);
    RUN_TEST_CASE(alsa_snd, snd_pcm_hw_params_set_rate_near);
    RUN_TEST_CASE(alsa_snd, snd_pcm_open);
    RUN_TEST_CASE(alsa_snd, snd_pcm_prepare);
    RUN_TEST_CASE(alsa_snd, snd_pcm_readi);
    RUN_TEST_CASE(alsa_snd, snd_pcm_recover);
    RUN_TEST_CASE(alsa_snd, snd_pcm_start);
    RUN_TEST_CASE(alsa_snd, snd_pcm_close);
    RUN_TEST_CASE(alsa_snd, snd_pcm_sw_params);
    RUN_TEST_CASE(alsa_snd, snd_pcm_sw_params_current);
    RUN_TEST_CASE(alsa_snd, snd_pcm_sw_params_free);
    RUN_TEST_CASE(alsa_snd, snd_pcm_sw_params_malloc);
    RUN_TEST_CASE(alsa_snd, snd_pcm_writei);
}
#endif

