/****************************************************************************
 *  Copyright (C) 2023 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ***************************************************************************/

#include <fcntl.h>
#include <pty.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "openpty.h"

static void disable_echo(int fd)
{
    struct termios echo;

    tcgetattr(fd, &echo);

    echo.c_lflag &= ~ECHO;

    tcsetattr(fd, TCSANOW, &echo);
}

int open_pty(int* master, char* name)
{
    char buf[64];
    int ret;
    /* Open the pseudo terminal master */
    ret = posix_openpt(O_RDWR);
    if (ret < 0)
        return ret;

    *master = ret;

    /* Configure the pseudo terminal master */

    ret = grantpt(*master);
    if (ret < 0)
        goto err;

    ret = unlockpt(*master);
    if (ret < 0)
        goto err;

    /* Open the pseudo terminal slave */

    ret = ptsname_r(*master, buf, sizeof(buf));
    if (ret < 0)
        goto err;

    if (name != NULL)
        strcpy(name, buf);

    disable_echo(*master);

    return 0;

err:
    close(*master);
    return ret;
}