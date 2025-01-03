#include <stdio.h>
#include <stdlib.h>

#include "unity_fixture.h"

static void runAllTests(void)
{
    RUN_TEST_GROUP(common_log);
    RUN_TEST_GROUP(common_cfg);
    RUN_TEST_GROUP(alsa_snd);
    RUN_TEST_GROUP(detour_hook);
    RUN_TEST_GROUP(detour_drastic);
    RUN_TEST_GROUP(sdl2_audio_miyoo);
    RUN_TEST_GROUP(sdl2_render_miyoo);
    RUN_TEST_GROUP(sdl2_joystick_miyoo);
//    RUN_TEST_GROUP(sdl2_video_miyoo);
//    RUN_TEST_GROUP(sdl2_event_miyoo);
}

int main(int argc, const char **argv)
{
    printf("\n");
    return UnityMain(argc, argv, runAllTests);
}

