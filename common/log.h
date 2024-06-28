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

#ifndef __LOG_H__
#define __LOG_H__

    #define LOG_LEVEL_ERR       3
    #define LOG_LEVEL_WARN      2
    #define LOG_LEVEL_INFO      1
    #define LOG_LEVEL_DEBUG     0

    #define LOG_FILE_NAME       "miyoo_drastic_log.txt"
    #define LOG_LEVEL_DEFAULT   LOG_LEVEL_ERR

    #define SND     "[SND]"
    #define DTR     "[DTR]"
    #define COM     "[COM]"
    #define SDL     "[SDL]"

    #define ERR     "[ERR]"
    #define WARN    "[WARN]"
    #define INFO    "[INFO]"
    #define DEBUG   "[DEBUG]"

    #define err(...)    write_log_to_file(LOG_LEVEL_ERR, ERR, __VA_ARGS__)
    #define warn(...)   write_log_to_file(LOG_LEVEL_WARN, WARN, __VA_ARGS__)
    #define info(...)   write_log_to_file(LOG_LEVEL_INFO, INFO, __VA_ARGS__)
    #define debug(...)  write_log_to_file(LOG_LEVEL_DEBUG, DEBUG, __VA_ARGS__)

    int set_debug_level(int newlevel);
    int write_log_to_file(int level, const char *msg, const char *fmt, ...);

#endif

