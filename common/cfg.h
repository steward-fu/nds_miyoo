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

#ifndef __CFG_H__
#define __CFG_H__

#if defined(A30)
#define JSON_SYS_FOLDER     "/config"
#endif

#if defined(MINI)
#define JSON_SYS_FOLDER     "/appconfigs"
#endif

#if defined(UT)
#define JSON_SYS_FOLDER     "."
#endif

#define JSON_SYS_FILE       "system.json"
#define JSON_SYS_PATH       JSON_SYS_FOLDER "/" JSON_SYS_FILE
#define JSON_SYS_VOLUME     "vol"

#define CFG_PATH            "miyoo/settings.pb"
#define MAX_PATH            128
#define MAX_MALLOC_SIZE     4096

#define DEF_CFG_VERSION                         "20250101"
#define DEF_CFG_LANGUAGE                        "en_US"
#define DEF_CFG_MENU_BG                         "menu/640/1/bg.png"
#define DEF_CFG_PEN_IMAGE                       "pen/4_lb.png"
#define DEF_CFG_FONT_PATH                       "font/simsun.ttf"
#define DEF_CFG_STATE_FOLDER                    ""
#define DEF_CFG_BORDER_IMAGE                    "border/640x480/1/16.png"
#define DEF_CFG_FAST_FORWARD                    6
#define DEF_CFG_HALF_VOLUME                     false
#define DEF_CFG_LOW_BATTERY_CLOSE               true
#define DEF_CFG_DISPLAY_LAYOUT                  16
#define DEF_CFG_DISPLAY_ALT_LAYOUT              2
#define DEF_CFG_DISPLAY_SMALL_ALPHA             3
#define DEF_CFG_DISPLAY_SMALL_BORDER            3
#define DEF_CFG_DISPLAY_SMALL_POSITION          3
#define DEF_CFG_CPU_FREQ_MIN                    300
#define DEF_CFG_CPU_FREQ_MAX                    3000
#define DEF_CFG_CPU_CORE_MIN                    1
#define DEF_CFG_CPU_CORE_MAX                    4
#define DEF_CFG_PEN_SCREEN0                     true
#define DEF_CFG_PEN_SPEED_X                     30000
#define DEF_CFG_PEN_SPEED_Y                     35000
#define DEF_CFG_MENU_SHOW_CURSOR                true
#define DEF_CFG_AUTOSAVE_ENABLE                 true
#define DEF_CFG_AUTOSAVE_SLOT                   10
#define DEF_CFG_KEYPAD_ROTATE                   0
#define DEF_CFG_KEYPAD_HOTKEY                   0
#define DEF_CFG_KEYPAD_SWAP_L1_L2               0
#define DEF_CFG_KEYPAD_SWAP_R1_R2               0
#define DEF_CFG_JOYSTICK_MODE                   0
#define DEF_CFG_JOYSTICK_DEAD_ZONE              65
#define DEF_CFG_JOYSTICK_REMAP_LEFT_TOP         0
#define DEF_CFG_JOYSTICK_REMAP_LEFT_DOWN        1
#define DEF_CFG_JOYSTICK_REMAP_LEFT_LEFT        2
#define DEF_CFG_JOYSTICK_REMAP_LEFT_RIGHT       3
#define DEF_CFG_JOYSTICK_REMAP_RIGHT_TOP        0
#define DEF_CFG_JOYSTICK_REMAP_RIGHT_DOWN       1
#define DEF_CFG_JOYSTICK_REMAP_RIGHT_LEFT       2
#define DEF_CFG_JOYSTICK_REMAP_RIGHT_RIGHT      3

int init_config_settings(void);
int load_config_settings(void);
int reset_config_settings(void);
int update_config_settings(void);
int get_system_volume(void);
int set_system_volume(int vol);
int get_cfg_half_volume(void);
int get_cfg_autosave_enable(void);
int get_cfg_autosave_slot(void);

#endif

