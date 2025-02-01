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

#ifndef __SDL2_AUDIO_MIYOO_H__
#define __SDL2_AUDIO_MIYOO_H__

#include "../SDL_sysaudio.h"
#include "../../SDL_internal.h"

#if defined(MINI)
#include "mi_sys.h"
#include "mi_common_datatype.h"
#include "mi_ao.h"
#endif

#define _THIS SDL_AudioDevice *this

struct SDL_PrivateAudioData {
    int mixlen;
    int audio_fd;
    uint8_t *mixbuf;
};

typedef struct _miyoo_audio {
#if defined(MINI)
    struct {
        MI_AO_CHN channel;
        MI_AUDIO_DEV dev;
        MI_AUDIO_Attr_t set_attr;
        MI_AUDIO_Attr_t get_attr;
    } mi;
#endif
} miyoo_audio;

#endif

