/**
 * @copyright Copyright (c) 2022, Alibaba Group Holding Limited
 */

#ifndef PLATFORM_H
#define PLATFORM_H

#include <errno.h>

/**
 * @brief get system last errno
 *
 * @return int
 */
static int get_last_sys_errno() {
    int err = 0;
    err = errno;
    return err;
}

/**
 * @brief init platform env if necessary
 *
 */
static void xqc_platform_init_env() {
    int result = 0;
}

static void set_last_sys_errno(int err) {
    errno = err;
}

#endif
