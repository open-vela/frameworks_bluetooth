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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/un.h>

#if defined(CONFIG_NET_SOCKOPTS)
void setSocketBuf(int fd, int option)
{
    int buff_size = 0;
    socklen_t socklen = sizeof(int);
    assert(getsockopt(fd, SOL_SOCKET, option, &buff_size, &socklen) == OK);
    if (buff_size >= CONFIG_BLUETOOTH_SOCKET_BUF_SIZE)
        return;

    buff_size = CONFIG_BLUETOOTH_SOCKET_BUF_SIZE;
    assert(setsockopt(fd, SOL_SOCKET, option, &buff_size, sizeof(buff_size)) == OK);
    assert(getsockopt(fd, SOL_SOCKET, option, &buff_size, &socklen) == OK);
    assert(buff_size >= CONFIG_BLUETOOTH_SOCKET_BUF_SIZE);
}
#endif