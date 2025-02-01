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
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include "../../SDL_internal.h"
#include "SDL_timer.h"
#include "SDL_audio.h"
#include "../SDL_audio_c.h"
#include "../SDL_audiodev_c.h"

#include "log.h"
#include "audio_miyoo.h"

#if defined(UT)
#include "unity_fixture.h"
#endif

#if defined(MINI)
miyoo_audio myaudio = { 0 };
#endif

#if defined(UT)
TEST_GROUP(sdl2_audio_miyoo);

TEST_SETUP(sdl2_audio_miyoo)
{
}

TEST_TEAR_DOWN(sdl2_audio_miyoo)
{
}
#endif

static void CloseDevice(_THIS)
{
    do {
        if (!this) {
            err(SDL"invalid parameter(0x%x) in %s\n", this, __func__);
            break;
        }

        if (this->hidden->mixbuf) {
            SDL_free(this->hidden->mixbuf);
            this->hidden->mixbuf = NULL;
        }

        if (this->hidden) {
            SDL_free(this->hidden);
            this->hidden = NULL;
        }

#if defined(MINI)
        MI_AO_DisableChn(myaudio.mi.dev, myaudio.mi.channel);
        MI_AO_Disable(myaudio.mi.dev);
#endif
    } while (0);
}

#if defined(UT)
TEST(sdl2_audio_miyoo, CloseDevice)
{
    CloseDevice(NULL);
    TEST_PASS();
}
#endif

static int OpenDevice(_THIS, void *handle, const char *devname, int iscapture)
{
#if defined(MINI)
    MI_S32 miret = 0;
    MI_S32 s32SetVolumeDb = 0;
    MI_S32 s32GetVolumeDb = 0;
    MI_SYS_ChnPort_t stmi_channel0OutputPort0;
#endif

    if (!this || !handle || !devname) {
        err(
            SDL"invalid parameter(0x%x, 0x%x, 0x%x, 0x%x) in %s\n",
            this,
            handle,
            devname,
            iscapture,
            __func__
        );

        return -1;
    }

    this->hidden = 
        (struct SDL_PrivateAudioData *)SDL_malloc((sizeof * this->hidden));

    if(this->hidden == NULL) {
        err(SDL"failed to allocate memory for audio data in %s\n", __func__);
        return SDL_OutOfMemory();
    }
    SDL_zerop(this->hidden);

    this->hidden->mixlen = this->spec.samples * 2 * this->spec.channels;
    this->hidden->mixbuf = (Uint8 *) SDL_malloc(this->hidden->mixlen);
    if(this->hidden->mixbuf == NULL) {
        err(SDL"failed to allocate memory for mix buffer in %s\n", __func__);
        return SDL_OutOfMemory();
    }

#if defined(MINI)
    myaudio.mi.set_attr.eBitwidth = E_MI_AUDIO_BIT_WIDTH_16;
    myaudio.mi.set_attr.eWorkmode = E_MI_AUDIO_MODE_I2S_MASTER;
    myaudio.mi.set_attr.u32FrmNum = 6;
    myaudio.mi.set_attr.u32PtNumPerFrm = this->spec.samples;
    myaudio.mi.set_attr.u32ChnCnt = this->spec.channels;
    myaudio.mi.set_attr.eSoundmode =
        this->spec.channels == 2 ?
        E_MI_AUDIO_SOUND_MODE_STEREO : E_MI_AUDIO_SOUND_MODE_MONO;

    myaudio.mi.set_attr.eSamplerate = (MI_AUDIO_SampleRate_e)this->spec.freq;
    info(
        SDL"audio freq:%d, sample:%d, channels:%d in %s\n",
        this->spec.freq,
        this->spec.samples,
        this->spec.channels,
        __func__
    );

    miret = MI_AO_SetPubAttr(myaudio.mi.dev, &myaudio.mi.set_attr);
    if(MI_SUCCESS != miret) {
        err(SDL"failed to set PubAttr in %s\n", __func__);
        return -1;
    }

    miret = MI_AO_GetPubAttr(myaudio.mi.dev, &myaudio.mi.get_attr);
    if(MI_SUCCESS != miret) {
        err(SDL"failed to get PubAttr in %s\n", __func__);
        return -1;
    }

    miret = MI_AO_Enable(myaudio.mi.dev);
    if(MI_SUCCESS != miret) {
        err(SDL"failed to enable AO in %s\n", __func__);
        return -1;
    }

    miret = MI_AO_EnableChn(myaudio.mi.dev, myaudio.mi.channel);
    if(miret != MI_SUCCESS) {
        err(SDL"failed to enable channel in %s\n", __func__);
        return -1;
    }

    miret = MI_AO_SetVolume(myaudio.mi.dev, s32SetVolumeDb);
    if(MI_SUCCESS != miret) {
        err(SDL"failed to set volume in %s\n", __func__);
        return -1;
    }

    MI_AO_GetVolume(myaudio.mi.dev, &s32GetVolumeDb);
    stmi_channel0OutputPort0.eModId = E_MI_MODULE_ID_AO;
    stmi_channel0OutputPort0.u32DevId = myaudio.mi.dev;
    stmi_channel0OutputPort0.u32ChnId = myaudio.mi.channel;
    stmi_channel0OutputPort0.u32PortId = 0;
    MI_SYS_SetChnOutputPortDepth(&stmi_channel0OutputPort0, 12, 13);
#endif

    return 0;
}

#if defined(UT)
TEST(sdl2_audio_miyoo, OpenDevice)
{
    size_t s = 0;
    char buf[32] = { 0 };
    const char *name = "XXX";
    SDL_AudioDevice au = { 0 };

    TEST_ASSERT_EQUAL_INT(-1, OpenDevice(NULL, NULL, NULL, 0));
    TEST_ASSERT_EQUAL_INT(-1, OpenDevice(&au, buf, NULL, 0));

    au.spec.samples = 1024;
    au.spec.channels = 2;
    s = au.spec.samples * 2 * au.spec.channels;
    TEST_ASSERT_EQUAL_INT(0, OpenDevice(&au, buf, name, 0));
    TEST_ASSERT_NOT_NULL(au.hidden);
    TEST_ASSERT_NOT_NULL(au.hidden->mixbuf);
    TEST_ASSERT_EQUAL_INT(s, au.hidden->mixlen);
    CloseDevice(&au);
}
#endif

static void PlayDevice(_THIS)
{
    do {
#if defined(MINI)
        MI_S32 ret = 0;
        MI_AUDIO_Frame_t frm = { 0 };
#endif

        if (!this) {
            err(SDL"invalid parameter(0x%x) in %s\n", this, __func__);
            break;
        }

#if defined(MINI)
        frm.eBitwidth = myaudio.mi.get_attr.eBitwidth;
        frm.eSoundmode = myaudio.mi.get_attr.eSoundmode;
        frm.u32Len = this->hidden->mixlen;
        frm.apVirAddr[0] = this->hidden->mixbuf;
        frm.apVirAddr[1] = NULL;
        do {
            ret = MI_AO_SendFrame(
                myaudio.mi.dev,
                myaudio.mi.channel,
                &frm,
                1
            );

            usleep(
                ((myaudio.mi.set_attr.u32PtNumPerFrm * 1000) /
                myaudio.mi.set_attr.eSamplerate - 10) * 1000
            );
        }
        while (ret == MI_AO_ERR_NOBUF);
#endif
    } while (0);
}

#if defined(UT)
TEST(sdl2_audio_miyoo, PlayDevice)
{
    PlayDevice(NULL);
    TEST_PASS();
}
#endif

static uint8_t *GetDeviceBuf(_THIS)
{
    if (!this) {
        err(SDL"invalid parameter(0x%x) in %s\n", this, __func__);
        return NULL;
    }

    return (this->hidden->mixbuf);
}

#if defined(UT)
TEST(sdl2_audio_miyoo, GetDeviceBuf)
{
    TEST_ASSERT_NULL(GetDeviceBuf(NULL));
}
#endif

static int AudioInit(SDL_AudioDriverImpl *impl)
{
    if (impl == NULL) {
        err(SDL"invalid parameter(0x%x) in %s\n", impl, __func__);
        return -1;
    }

    impl->OpenDevice = OpenDevice;
    impl->PlayDevice = PlayDevice;
    impl->CloseDevice = CloseDevice;
    impl->GetDeviceBuf = GetDeviceBuf;
    impl->OnlyHasDefaultOutputDevice = 1;
    return 1;
}

AudioBootStrap MiyooAudio_bootstrap = {
    "Miyoo",
    "Miyoo Audio Driver",
    AudioInit,
    0
};

#if defined(UT)
TEST(sdl2_audio_miyoo, AudioInit)
{
    SDL_AudioDriverImpl t = { 0 };

    TEST_ASSERT_EQUAL_INT(-1, AudioInit(NULL));

    TEST_ASSERT_EQUAL_INT(1, AudioInit(&t));
    TEST_ASSERT_EQUAL_INT(1, t.OnlyHasDefaultOutputDevice);
}
#endif

#if defined(UT)
TEST_GROUP_RUNNER(sdl2_audio_miyoo)
{
    RUN_TEST_CASE(sdl2_audio_miyoo, CloseDevice);
    RUN_TEST_CASE(sdl2_audio_miyoo, OpenDevice);
    RUN_TEST_CASE(sdl2_audio_miyoo, PlayDevice);
    RUN_TEST_CASE(sdl2_audio_miyoo, GetDeviceBuf);
    RUN_TEST_CASE(sdl2_audio_miyoo, AudioInit);
}
#endif

