// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "topio_setproctitle.h"

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

#include <cstdint>

/*
 * To change the process title in Linux and Solaris we have to set argv[1]
 * to NULL and to copy the title to the same place where the argv[0] points to.
 * However, argv[0] may be too small to hold a new title.  Fortunately, Linux
 * and Solaris store argv[] and environ[] one after another.  So we should
 * ensure that is the continuous memory and then we allocate the new memory
 * for environ[] and copy it.  After this we could use the memory starting
 * from argv[0] for our process title.
 */

extern char **environ;
extern char** topio_os_argv;

static char *topio_os_argv_last;

int topio_init_setproctitle()
{
    char *p;
    int i = 0;
    uint32_t size = 0;

    for (i = 0; environ[i]; i++) {
        size += strlen(environ[i]) + 1;
    }

    p = (char*)malloc(size);
    if (p == NULL) {
        return TOPIO_PROCTITLE_ERROR;
    }

    topio_os_argv_last = topio_os_argv[0];

    for (i = 0; topio_os_argv[i]; i++) {
        if (topio_os_argv_last == topio_os_argv[i]) {
            topio_os_argv_last = topio_os_argv[i] + strlen(topio_os_argv[i]) + 1;
        }
    }

    for (i = 0; environ[i]; i++) {
        if (topio_os_argv_last == environ[i]) {

            size = strlen(environ[i]) + 1;
            topio_os_argv_last = environ[i] + size;

            strncpy(p, environ[i], size);
            environ[i] = (char *) p;
            p += size;
        }
    }

    topio_os_argv_last--;

    return TOPIO_PROCTITLE_OK;
}


void topio_setproctitle(const char *title)
{
    topio_os_argv[1] = NULL;
    char new_title[1024];
    bzero(new_title, sizeof(new_title));
    sprintf(new_title, "%s%s", "topio: ", title);

    strncpy(topio_os_argv[0], new_title, topio_os_argv_last - topio_os_argv[0]);

#ifdef DEBUG
    printf("set title:%s\n", new_title);
#endif
}
