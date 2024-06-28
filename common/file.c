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
#include <json-c/json.h>
#include <sys/stat.h>

#if defined(UT)
#include "unity_fixture.h"
#endif

#include "log.h"
#include "cfg.h"
#include "snd.h"

extern char home_path[MAX_PATH];

#if defined(UT)
TEST_GROUP(common_file);

TEST_SETUP(common_file)
{
}

TEST_TEAR_DOWN(common_file)
{
}
#endif

int check_resource_file_exist_and_copy(char *dst, int dst_size, const char *fpath)
{
    struct stat st = { 0 };
    char buf[MAX_PATH] = { 0 };

    if (!dst || (dst_size < 0) || !fpath) {
        err(COM"invalid parameter in %s\n", __func__);
        return -1;
    }

    if (snprintf(buf, sizeof(buf), "%s/%s/%s", home_path, JSON_CFG_FOLDER, fpath) < 0) {
        err(COM"failed to format string in %s\n", __func__);
        return -1;
    }

    if (strlen(buf) > dst_size) {
        err(COM"the length of path is too large in %s\n", __func__);
        return -1;
    }

    if (stat(buf, &st) || !S_ISREG(st.st_mode)) {
        err(COM"invalid resource path(\"%s\") in %s\n", buf, __func__);
        return -1;
    }

    strncpy(dst, fpath, dst_size);
    return 0;
}

#if defined(UT)
TEST(common_file, check_resource_file_exist_and_copy)
{
    char buf[MAX_PATH] = { 0 };

    TEST_ASSERT_EQUAL_INT(-1, check_resource_file_exist_and_copy(NULL, 0, NULL));
    TEST_ASSERT_EQUAL_INT(-1, check_resource_file_exist_and_copy(buf, 0, "/NOT_EXIST_PATH"));
    TEST_ASSERT_EQUAL_INT(-1, check_resource_file_exist_and_copy(buf, sizeof(buf), "/NOT_EXIST_PATH"));

    TEST_ASSERT_EQUAL_INT(0, check_resource_file_exist_and_copy(buf, sizeof(buf), JSON_CFG_FILE));
    TEST_ASSERT_EQUAL_STRING(JSON_CFG_FILE, buf);
}
#endif

int check_absolute_folder_exist_and_copy(char *dst, int dst_size, const char *path)
{
    struct stat st = { 0 };

    if (!dst || (dst_size < 0) || !path) {
        err(COM"invalid parameter in %s\n", __func__);
        return -1;
    }

    if (strlen(path) > dst_size) {
        err(COM"the length of path is too large in %s\n", __func__);
        return -1;
    }

    if (stat(path, &st) || !S_ISDIR(st.st_mode)) {
        err(COM"invalid folder path(\"%s\") in %s\n", path, __func__);
        return -1;
    }

    strncpy(dst, path, dst_size);
    return 0;
}

#if defined(UT)
TEST(common_file, check_absolute_folder_exist_and_copy)
{
    char buf[MAX_PATH] = { 0 };

    TEST_ASSERT_EQUAL_INT(-1, check_absolute_folder_exist_and_copy(NULL, 0, NULL));
    TEST_ASSERT_EQUAL_INT(-1, check_absolute_folder_exist_and_copy(buf, 0, "/tmp"));
    TEST_ASSERT_EQUAL_INT(-1, check_absolute_folder_exist_and_copy(buf, sizeof(buf), "/NOT_EXIST_PATH"));

    TEST_ASSERT_EQUAL_INT(0, check_absolute_folder_exist_and_copy(buf, sizeof(buf), "/tmp"));
    TEST_ASSERT_EQUAL_STRING("/tmp", buf);
}
#endif

#if defined(UT)
TEST_GROUP_RUNNER(common_file)
{
    RUN_TEST_CASE(common_file, check_absolute_folder_exist_and_copy);
    RUN_TEST_CASE(common_file, check_resource_file_exist_and_copy);
}
#endif

