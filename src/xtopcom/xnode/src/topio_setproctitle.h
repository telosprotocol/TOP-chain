// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once


#ifndef _TOPIO_SETPROCTITLE_H_INCLUDED_
#define _TOPIO_SETPROCTITLE_H_INCLUDED_

#define TOPIO_PROCTITLE_ERROR  -1
#define TOPIO_PROCTITLE_OK   0

int topio_init_setproctitle();
void topio_setproctitle(const char *title);

#endif /* _TOPIO_SETPROCTITLE_H_INCLUDED_ */
