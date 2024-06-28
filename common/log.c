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

#define _GNU_SOURCE
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>

#if defined(UT)
#include "unity_fixture.h"
#endif

#include "log.h"
#include "hook.h"

static int debug_level = LOG_LEVEL_DEFAULT;

int set_debug_level(int newlevel)
{
    debug_level = newlevel;
    info(COM"set debug level as %d\n", debug_level);

    return debug_level;
}

int write_log_to_file(int level, const char *msg, const char *fmt, ...)
{
    if (level < debug_level) {
        return -1;
    }

    FILE *file = fopen(LOG_FILE_NAME, "a+"); 
    if (NULL == file) {
        return -1;
    }

    va_list va = {0};
    time_t rawtime = {0};
    struct tm *timeinfo = {0};
    
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    
    fprintf(file, "[%04d/%02d/%02d-%02d:%02d:%02d]",
        timeinfo->tm_year + 1900,
        timeinfo->tm_mday,
        timeinfo->tm_mon + 1,
        timeinfo->tm_hour,
        timeinfo->tm_min,
        timeinfo->tm_sec);


    va_start(va, fmt);
    fprintf(file, "%s", msg);
    vfprintf(file, fmt, va);
    va_end(va);
    fclose(file);

    return 0;
}

#if defined(UT)
TEST_GROUP(common_log);

TEST_SETUP(common_log)
{
}

TEST_TEAR_DOWN(common_log)
{
}

TEST(common_log, set_debug_level)
{
    TEST_ASSERT_EQUAL_INT(LOG_LEVEL_ERR, set_debug_level(LOG_LEVEL_ERR));
    TEST_ASSERT_EQUAL_INT(LOG_LEVEL_DEBUG, set_debug_level(LOG_LEVEL_DEBUG));
}

TEST(common_log, write_log_to_file)
{
    TEST_ASSERT_EQUAL_INT(LOG_LEVEL_ERR, set_debug_level(LOG_LEVEL_ERR));
    TEST_ASSERT_EQUAL_INT(0, write_log_to_file(LOG_LEVEL_ERR, INFO, COM"run test with error level\n"));
    TEST_ASSERT_EQUAL_INT(-1, write_log_to_file(LOG_LEVEL_DEBUG, INFO, COM"run test with debug level\n"));

    TEST_ASSERT_EQUAL_INT(LOG_LEVEL_DEBUG, set_debug_level(LOG_LEVEL_DEBUG));
    TEST_ASSERT_EQUAL_INT(0, write_log_to_file(LOG_LEVEL_ERR, INFO, COM"run test with error level\n"));
    TEST_ASSERT_EQUAL_INT(0, write_log_to_file(LOG_LEVEL_DEBUG, INFO, COM"run test with debug level\n"));
}

TEST_GROUP_RUNNER(common_log)
{
    RUN_TEST_CASE(common_log, set_debug_level);
    RUN_TEST_CASE(common_log, write_log_to_file);
}
#endif

