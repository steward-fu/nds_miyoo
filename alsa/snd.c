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

#if defined(MINI)
#include "mi_ao.h"
#include "mi_sys.h"
#include "mi_common_datatype.h"
#endif

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

static int16_t *adpcm_step_table = (int16_t *)VAR_ADPCM_STEP_TABLE;
static int8_t *adpcm_index_step_table = (int8_t *)VAR_ADPCM_INDEX_STEP_TABLE;

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

    if (!channel) {
        err(SND"Invalid channel parameter\n");
        return;
    }

    uVar3 = channel->adpcm_cache_block_offset;
    uVar1 = (uint32_t)(channel->adpcm_current_index);
    sample_delta = (uint32_t)(channel->adpcm_sample);
    uVar2 = *(uint32_t *)(channel->samples + (uVar3 >> 1));
    channel->adpcm_cache_block_offset = uVar3 + 8;
    psVar7 = channel->adpcm_sample_cache + (uVar3 & 0x3f);
    do {
        uVar5 = (uint32_t)(adpcm_step_table[uVar1]);
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
}

#if defined(MINI) || defined(UT)
static int set_volume_raw(int value, int add)
{
    int fd = -1;
    int prev_value = 0;
    int buf2[] = { 0, 0 };
    uint64_t buf1[] = { sizeof(buf2), (uintptr_t)buf2 };

    fd = open(MI_DEV, O_RDWR);
    if (fd < 0) {
        err(SND"Failed to open "MI_DEV"\n");
        return -1;
    }

    if (ioctl(fd, MI_AO_GETVOLUME, buf1) < 0) {
        err(SND"Failed to get volume\n");
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
        warn(SND"Volume is same as previous value\n");
        return prev_value;
    }

    buf2[1] = value;
    if (ioctl(fd, MI_AO_SETVOLUME, buf1) < 0) {
        err(SND"Failed to set volume(%d)\n", buf2[1]);
        return -1;
    }

    if (prev_value <= MIN_RAW_VALUE && value > MIN_RAW_VALUE) {
        buf2[1] = 0;
        if (ioctl(fd, MI_AO_SETMUTE, buf1) < 0) {
            err(SND"Failed to set mute(%d)\n", buf2[1]);
        }
    }
    else if (prev_value > MIN_RAW_VALUE && value <= MIN_RAW_VALUE) {
        buf2[1] = 1;
        if (ioctl(fd, MI_AO_SETMUTE, buf1) < 0) {
            err(SND"Failed to set mute(%d)\n", buf2[1]);
            return -1;
        }
    }
    close(fd);

    info(SND"New volume is %d\n", value);
    return value;
}

static int set_volume(int volume)
{
    int volume_raw = 0;
    int div = 0;//get_cfg_half_volume() ? 2 : 1;

    if (volume > MAX_VOLUME) {
        volume = MAX_VOLUME;
    }
    else if (volume < 0) {
        volume = 0;
    }

    if (volume != 0) {
        volume_raw = round(48 * log10(1 + volume));
    }

    set_volume_raw(volume_raw / div, 0);
    return volume;
}
#endif

int volume_inc(void)
{
    int vol = 0;//get_sys_volume();

    if (vol < MAX_VOLUME) {
        vol += 1;

#if defined(MINI)
        set_volume(vol);
#endif

#if defined(A30)
        *vol_ptr = ((vol_base + (vol << vol_mul)) << 8) | (vol_base + (vol << vol_mul));
#endif

        //set_sys_volume(vol);
    }

    return vol;
}

int volume_dec(void)
{
    int vol = 0;//get_sys_volume();

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

        //set_sys_volume(vol);
    }

    return vol;
}

#if defined(A30) || defined(UT)
static int open_dsp(void)
{
    int arg = 0;
    int vol = 0;//get_sys_volume();

    if (dsp_fd > 0) {
        close(dsp_fd);
        warn(SND"Already opened "DSP_DEV", close it\n");
    }

    dsp_fd = open(DSP_DEV, O_WRONLY);
    if (dsp_fd < 0) {
        err(SND"Failed to open /dev/dsp\n");
        return -1;
    }

    if (system("amixer set \'DACL Mixer AIF1DA0L\' on") < 0) {
        warn(SND"Failed to set amixer AIF1DA0L\n");
    }

    if (system("amixer set \'DACL Mixer AIF1DA0R\' on") < 0) {
        warn(SND"Failed to set amixer AIF1DA0R\n");
    }

    vol_ptr = (uint32_t *)(&mem_ptr[0xc00 + 0x258]);
    *vol_ptr = ((vol_base + (vol << vol_mul)) << 8) | (vol_base + (vol << vol_mul));

    arg = 16;
    if (ioctl(dsp_fd, SOUND_PCM_WRITE_BITS, &arg) < 0) {
        err(SND"Failed to set PCM bits\n");
        return -1;
    }

    arg = CHANNELS;
    if (ioctl(dsp_fd, SOUND_PCM_WRITE_CHANNELS, &arg) < 0) {
        err(SND"Failed to set PCM channels\n");
        return -1;
    }

    arg = FREQ;
    if (ioctl(dsp_fd, SOUND_PCM_WRITE_RATE, &arg) < 0) {
        err(SND"Failed to set PCM rate\n");
        return -1;
    }
    return 0;
}
#endif

static void queue_init(queue_t *q, size_t s)
{
    q->buffer = (uint8_t *)malloc(s);
    q->size = s;
    q->read = q->write = 0;
    pthread_mutex_init(&q->lock, NULL);
}

static void queue_destroy(queue_t *q)
{
    if (q->buffer) {
        free(q->buffer);
        q->buffer = NULL;
    }
    pthread_mutex_destroy(&q->lock);
}

static int queue_size_for_read(queue_t *q)
{
    if (q->read == q->write) {
        return 0;
    }
    else if(q->read < q->write){
        return q->write - q->read;
    }
    return (QUEUE_SIZE - q->read) + q->write;
}

static int queue_size_for_write(queue_t *q)
{
    if (q->write == q->read) {
        return QUEUE_SIZE;
    }
    else if(q->write < q->read){
        return q->read - q->write;
    }
    return (QUEUE_SIZE - q->write) + q->read;
}

static int queue_put(queue_t *q, uint8_t *buffer, size_t size)
{
    int r = 0, tmp = 0, avai = 0;

    pthread_mutex_lock(&q->lock);
    avai = queue_size_for_write(q);
    if (size > avai) {
        size = avai;
    }
    r = size;

    if (size > 0) {
        if ((q->write >= q->read) && ((q->write + size) > QUEUE_SIZE)) {
            tmp = QUEUE_SIZE - q->write;
            size-= tmp;
#if !defined(UT)
            neon_memcpy(&q->buffer[q->write], buffer, tmp);
            neon_memcpy(q->buffer, &buffer[tmp], size);
#endif
            q->write = size;
        }
        else {
#if !defined(UT)
            neon_memcpy(&q->buffer[q->write], buffer, size);
#endif
            q->write += size;
        }
    }
    pthread_mutex_unlock(&q->lock);
    return r;
}

static size_t queue_get(queue_t *q, uint8_t *buffer, size_t max)
{
    int r = 0, tmp = 0, avai = 0, size = max;

    pthread_mutex_lock(&q->lock);
    avai = queue_size_for_read(q);
    if (size > avai) {
        size = avai;
    }
    r = size;

    if (size > 0) {
        if ((q->read > q->write) && (q->read + size) > QUEUE_SIZE) {
            tmp = QUEUE_SIZE - q->read;
            size-= tmp;
#if !defined(UT)
            neon_memcpy(buffer, &q->buffer[q->read], tmp);
            neon_memcpy(&buffer[tmp], q->buffer, size);
#endif
            q->read = size;
        }
        else {
#if !defined(UT)
            neon_memcpy(buffer, &q->buffer[q->read], size);
#endif
            q->read+= size;
        }
    }
    pthread_mutex_unlock(&q->lock);
    return r;
}

static void *audio_handler(void *threadid)
{
#if defined(MINI)
    MI_AUDIO_Frame_t aoTestFrame = {0};
#endif

#if defined(A30)
    int chk_cnt = 0;
#endif

    int r = 0, len = pcm_buf_len, idx = 0;

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

snd_pcm_sframes_t snd_pcm_avail(snd_pcm_t *pcm)
{
    return 2048;
}

int snd_pcm_hw_params(snd_pcm_t *pcm, snd_pcm_hw_params_t *params)
{
    return 0;
}

int snd_pcm_hw_params_any(snd_pcm_t *pcm, snd_pcm_hw_params_t *params)
{
    return 0;
}

void snd_pcm_hw_params_free(snd_pcm_hw_params_t *obj)
{
}

int snd_pcm_hw_params_malloc(snd_pcm_hw_params_t **ptr)
{
    return 0;
}

int snd_pcm_hw_params_set_access(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_access_t _access)
{
    return 0;
}

int snd_pcm_hw_params_set_buffer_size_near(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_uframes_t *val)
{
    *val = SAMPLES * 2 * CHANNELS;
    return 0;
}

int snd_pcm_hw_params_set_channels(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int val)
{
    return 0;
}

int snd_pcm_hw_params_set_format(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_format_t val)
{
    if (val != SND_PCM_FORMAT_S16_LE) {
        return -1;
    }
    return 0;
}

int snd_pcm_hw_params_set_period_size_near(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_uframes_t *val, int *dir)
{
    *val = PERIOD;
    return 0;
}

int snd_pcm_hw_params_set_rate_near(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val, int *dir)
{
    *val = FREQ;
    return 0;
}

int snd_pcm_open(snd_pcm_t **pcm, const char *name, snd_pcm_stream_t stream, int mode)
{
    info(SND"Use customized ALSA library\n");
    if (stream != SND_PCM_STREAM_PLAYBACK) {
        err(SND"Steam format is not equal to SND_PCM_STREAM_PLAYBACK");
        return -1;
    }
    return 0;
}

int snd_pcm_prepare(snd_pcm_t *pcm)
{
    return 0;
}

snd_pcm_sframes_t snd_pcm_readi(snd_pcm_t *pcm, void *buffer, snd_pcm_uframes_t size)
{
    return 0;
}

int snd_pcm_recover(snd_pcm_t *pcm, int err, int silent)
{
    return 0;
}

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

    queue_init(&queue, (size_t)QUEUE_SIZE);
    if (queue.buffer == NULL) {
        err(SND"Failed to allocate buffer for queue\n");
        return -1;
    }
    memset(queue.buffer, 0, QUEUE_SIZE);

    pcm_buf_len = SAMPLES * 2 * CHANNELS;
    pcm_buf = malloc(pcm_buf_len);
    if (pcm_buf == NULL) {
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
    if(miret != MI_SUCCESS) {
        printf(SND"Failed to set PubAttr\n");
        return -1;
    }

    miret = MI_AO_GetPubAttr(AoDevId, &stGetAttr);
    if(miret != MI_SUCCESS) {
        printf(SND"Failed to get PubAttr\n");
        return -1;
    }

    miret = MI_AO_Enable(AoDevId);
    if(miret != MI_SUCCESS) {
        printf(SND"Failed to enable AO\n");
        return -1;
    }

    miret = MI_AO_EnableChn(AoDevId, AoChn);
    if(miret != MI_SUCCESS) {
        printf(SND"Failed to enable Channel\n");
        return -1;
    }

    stAoChn0OutputPort0.eModId = E_MI_MODULE_ID_AO;
    stAoChn0OutputPort0.u32DevId = AoDevId;
    stAoChn0OutputPort0.u32ChnId = AoChn;
    stAoChn0OutputPort0.u32PortId = 0;
    MI_SYS_SetChnOutputPortDepth(&stAoChn0OutputPort0, 12, 13);
    set_volume(cur_volume);
#endif

#if defined(A30)
    mem_fd = open("/dev/mem", O_RDWR);
    mem_ptr = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, 0x1c22000);
    open_dsp();
#endif

    detour_hook(FUN_SPU_ADPCM_DECODE_BLOCK, (intptr_t)spu_adpcm_decode_block);
    printf(SND"Enabled spu hooking\n");

    pcm_ready = 1;
    pthread_create(&thread, NULL, audio_handler, (void *)NULL);
    return 0;
}

int snd_pcm_close(snd_pcm_t *pcm)
{
    void *ret = NULL;

    //if (get_cfg_auto_save_load() > 0) {
    //    dtr_savestate(get_cfg_auto_save_load_slot());
    //}

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
#endif

#if defined(A30)
    *vol_ptr = (160 << 8) | 160;
    munmap(mem_ptr, 4096);
    close(mem_fd);
#endif
    return 0;
}

int snd_pcm_sw_params(snd_pcm_t *pcm, snd_pcm_sw_params_t *params)
{
    return 0;
}

int snd_pcm_sw_params_current(snd_pcm_t *pcm, snd_pcm_sw_params_t *params)
{
    return 0;
}

void snd_pcm_sw_params_free(snd_pcm_sw_params_t *obj)
{
}

int snd_pcm_sw_params_malloc(snd_pcm_sw_params_t **ptr)
{
    return 0;
}

snd_pcm_sframes_t snd_pcm_writei(snd_pcm_t *pcm, const void *buffer, snd_pcm_uframes_t size)
{
    if ((size > 1) && (size != pcm_buf_len)) {
        queue_put(&queue, (uint8_t*)buffer, size * 2 * CHANNELS);
    }
    return size;
}

#if defined(UT)
TEST_GROUP(alsa_snd);

TEST_SETUP(alsa_snd)
{
}

TEST_TEAR_DOWN(alsa_snd)
{
}

TEST(alsa_snd, snd_pcm_close)
{
    TEST_ASSERT_EQUAL_INT(0, snd_pcm_close(NULL));
    TEST_ASSERT_EQUAL_INT(0, pcm_ready);
    TEST_ASSERT_EQUAL_INT(0, pcm_buf);
    TEST_ASSERT_EQUAL_INT(0, queue.buffer);
}

TEST(alsa_snd, snd_pcm_writei)
{
    char buf[32] = {0};

    TEST_ASSERT_EQUAL_INT(0, snd_pcm_writei(NULL, NULL, 0));
    TEST_ASSERT_EQUAL_INT(sizeof(buf), snd_pcm_writei(NULL, buf, sizeof(buf)));
}

TEST(alsa_snd, snd_pcm_sw_params)
{
    TEST_ASSERT_EQUAL_INT(0, snd_pcm_sw_params(NULL, NULL));
}

TEST(alsa_snd, snd_pcm_sw_params_malloc)
{
    TEST_ASSERT_EQUAL_INT(0, snd_pcm_sw_params_malloc(NULL));
}

TEST(alsa_snd, snd_pcm_sw_params_current)
{
    TEST_ASSERT_EQUAL_INT(0, snd_pcm_sw_params_current(NULL, NULL));
}

TEST(alsa_snd, set_volume_raw)
{
    TEST_ASSERT_EQUAL_INT(-1, set_volume_raw(0, 0));
}

TEST(alsa_snd, set_volume)
{
    TEST_ASSERT_EQUAL_INT(MAX_VOLUME, set_volume(MAX_VOLUME + 1));
    TEST_ASSERT_EQUAL_INT(0, set_volume(-1));
}

TEST(alsa_snd, volume_dec)
{
    TEST_ASSERT_EQUAL_INT(1, set_volume(1));
    TEST_ASSERT_EQUAL_INT(0, volume_dec());
}

TEST(alsa_snd, volume_inc)
{
    TEST_ASSERT_EQUAL_INT(0, set_volume(0));
    TEST_ASSERT_EQUAL_INT(1, volume_inc());
}

TEST(alsa_snd, open_dsp)
{
    TEST_ASSERT_EQUAL_INT(-1, open_dsp());
}

TEST(alsa_snd, spu_adpcm_decode_block)
{
    spu_adpcm_decode_block(NULL);
    TEST_PASS();
}

TEST_GROUP_RUNNER(alsa_snd)
{
    RUN_TEST_CASE(alsa_snd, open_dsp);
    RUN_TEST_CASE(alsa_snd, volume_dec);
    RUN_TEST_CASE(alsa_snd, volume_inc);
    RUN_TEST_CASE(alsa_snd, set_volume);
    RUN_TEST_CASE(alsa_snd, set_volume_raw);

    RUN_TEST_CASE(alsa_snd, spu_adpcm_decode_block);

    RUN_TEST_CASE(alsa_snd, snd_pcm_close);
    RUN_TEST_CASE(alsa_snd, snd_pcm_writei);
    RUN_TEST_CASE(alsa_snd, snd_pcm_sw_params);
    RUN_TEST_CASE(alsa_snd, snd_pcm_sw_params_malloc);
    RUN_TEST_CASE(alsa_snd, snd_pcm_sw_params_current);
}
#endif

