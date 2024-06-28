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

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include "../../SDL_internal.h"
#include "SDL_timer.h"
#include "SDL_audio.h"
#include "SDL_audio_miyoo.h"
#include "../SDL_audio_c.h"
#include "../SDL_audiodev_c.h"

#define MI_AUDIO_SAMPLE_PER_FRAME 768

#if defined(MINI) && !defined(UT)
#include "mi_sys.h"
#include "mi_common_datatype.h"
#include "mi_ao.h"

static MI_AUDIO_Attr_t stSetAttr;
static MI_AUDIO_Attr_t stGetAttr;
static MI_AO_CHN AoChn = 0;
static MI_AUDIO_DEV AoDevId = 0;
#endif

static void MMIYOO_CloseDevice(_THIS)
{
    SDL_free(this->hidden->mixbuf);
    SDL_free(this->hidden);
#if defined(MINI) && !defined(UT)
    MI_AO_DisableChn(AoDevId, AoChn);
    MI_AO_Disable(AoDevId);
#endif
}

static int MMIYOO_OpenDevice(_THIS, void *handle, const char *devname, int iscapture)
{
#if defined(MINI) && !defined(UT)
    MI_S32 miret = 0;
    MI_S32 s32SetVolumeDb = 0;
    MI_S32 s32GetVolumeDb = 0;
    MI_SYS_ChnPort_t stAoChn0OutputPort0;
#endif

    this->hidden = (struct SDL_PrivateAudioData *)SDL_malloc((sizeof * this->hidden));
    if(this->hidden == NULL) {
        return SDL_OutOfMemory();
    }
    SDL_zerop(this->hidden);

    this->hidden->mixlen = this->spec.samples * 2 * this->spec.channels;
    this->hidden->mixbuf = (Uint8 *) SDL_malloc(this->hidden->mixlen);
    if(this->hidden->mixbuf == NULL) {
        return SDL_OutOfMemory();
    }

#if defined(MINI) && !defined(UT)
    stSetAttr.eBitwidth = E_MI_AUDIO_BIT_WIDTH_16;
    stSetAttr.eWorkmode = E_MI_AUDIO_MODE_I2S_MASTER;
    stSetAttr.u32FrmNum = 6;
    stSetAttr.u32PtNumPerFrm = this->spec.samples;
    stSetAttr.u32ChnCnt = this->spec.channels;
    stSetAttr.eSoundmode = this->spec.channels == 2 ? E_MI_AUDIO_SOUND_MODE_STEREO : E_MI_AUDIO_SOUND_MODE_MONO;
    stSetAttr.eSamplerate = (MI_AUDIO_SampleRate_e)this->spec.freq;
    printf("%s, freq:%d sample:%d channels:%d\n", __func__, this->spec.freq, this->spec.samples, this->spec.channels);
    miret = MI_AO_SetPubAttr(AoDevId, &stSetAttr);
    if(miret != MI_SUCCESS) {
        printf("%s, failed to set PubAttr\n", __func__);
        return -1;
    }
    miret = MI_AO_GetPubAttr(AoDevId, &stGetAttr);
    if(miret != MI_SUCCESS) {
        printf("%s, failed to get PubAttr\n", __func__);
        return -1;
    }
    miret = MI_AO_Enable(AoDevId);
    if(miret != MI_SUCCESS) {
        printf("%s, failed to enable AO\n", __func__);
        return -1;
    }
    miret = MI_AO_EnableChn(AoDevId, AoChn);
    if(miret != MI_SUCCESS) {
        printf("%s, failed to enable Channel\n", __func__);
        return -1;
    }
    miret = MI_AO_SetVolume(AoDevId, s32SetVolumeDb);
    if(miret != MI_SUCCESS) {
        printf("%s, failed to set Volume\n", __func__);
        return -1;
    }
    MI_AO_GetVolume(AoDevId, &s32GetVolumeDb);
    stAoChn0OutputPort0.eModId = E_MI_MODULE_ID_AO;
    stAoChn0OutputPort0.u32DevId = AoDevId;
    stAoChn0OutputPort0.u32ChnId = AoChn;
    stAoChn0OutputPort0.u32PortId = 0;
    MI_SYS_SetChnOutputPortDepth(&stAoChn0OutputPort0, 12, 13);
#endif
    return 0;
}

static void MMIYOO_PlayDevice(_THIS)
{
#if defined(MINI) && !defined(UT)
    MI_AUDIO_Frame_t aoTestFrame;
    MI_S32 s32RetSendStatus = 0;

    aoTestFrame.eBitwidth = stGetAttr.eBitwidth;
    aoTestFrame.eSoundmode = stGetAttr.eSoundmode;
    aoTestFrame.u32Len = this->hidden->mixlen;
    aoTestFrame.apVirAddr[0] = this->hidden->mixbuf;
    aoTestFrame.apVirAddr[1] = NULL;
    do {
        s32RetSendStatus = MI_AO_SendFrame(AoDevId, AoChn, &aoTestFrame, 1);
        usleep(((stSetAttr.u32PtNumPerFrm * 1000) / stSetAttr.eSamplerate - 10) * 1000);
    }
    while(s32RetSendStatus == MI_AO_ERR_NOBUF);
#endif
}

static Uint8 *MMIYOO_GetDeviceBuf(_THIS)
{
    return (this->hidden->mixbuf);
}

static int MMIYOO_Init(SDL_AudioDriverImpl *impl)
{
    if (impl == NULL) {
        return -1;
    }

    impl->OpenDevice = MMIYOO_OpenDevice;
    impl->PlayDevice = MMIYOO_PlayDevice;
    impl->GetDeviceBuf = MMIYOO_GetDeviceBuf;
    impl->CloseDevice = MMIYOO_CloseDevice;
    impl->OnlyHasDefaultOutputDevice = 1;
    return 1;
}

AudioBootStrap MMIYOOAUDIO_bootstrap = {"MMIYOO", "MMIYOO AUDIO DRIVER", MMIYOO_Init, 0};

#ifdef UT
#include "unity_fixture.h"

TEST_GROUP(sdl2_audio_mmiyoo);

TEST_SETUP(sdl2_audio_mmiyoo)
{
}

TEST_TEAR_DOWN(sdl2_audio_mmiyoo)
{
}

TEST(sdl2_audio_mmiyoo, MMIYOO_Init)
{
    TEST_ASSERT_EQUAL(MMIYOO_Init(NULL), -1);
}

TEST_GROUP_RUNNER(sdl2_audio_mmiyoo)
{
    RUN_TEST_CASE(sdl2_audio_mmiyoo, MMIYOO_Init);
}
#endif

