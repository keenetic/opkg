/* vi: set expandtab sw=4 sts=4: */
/* opkg_message.c - the opkg package management system

   Copyright (C) 2009 Ubiq Technologies <graham.gower@gmail.com>
   Copyright (C) 2003 Daniele Nicolodi <daniele@grinta.net>

   SPDX-License-Identifier: GPL-2.0-or-later

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
*/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#include "opkg_conf.h"
#include "opkg_message.h"
#include "xfuncs.h"

void opkg_message(message_level_t level, const char *fmt, ...)
{
    va_list ap;

    if (opkg_config->verbosity < (int)level)
        return;

    if (opkg_config->opkg_vmessage) {
        /* Pass the message to libopkg users. */
        va_start(ap, fmt);
        opkg_config->opkg_vmessage(level, fmt, ap);
        va_end(ap);
        return;
    }

    va_start(ap, fmt);

    if (level == ERROR) {
#define MSG_LEN 4096
        char msg[MSG_LEN];
        int ret;
        ret = vsnprintf(msg, MSG_LEN, fmt, ap);
        if (ret < 0) {
            fprintf(stderr,
                    "%s: encountered an output or encoding"
                    " error during vsnprintf.\n", __FUNCTION__);
            va_end(ap);
            exit(EXIT_FAILURE);
        }
        if (ret >= MSG_LEN) {
            fprintf(stderr, "%s: Message truncated.\n", __FUNCTION__);
        }
        fprintf(stderr, " * %s", msg);
    } else {
        int ret;
        ret = vprintf(fmt, ap);
        if (ret < 0) {
            fprintf(stderr,
                    "%s: encountered an output or encoding"
                    " error during vprintf.\n", __FUNCTION__);
            exit(EXIT_FAILURE);
        }
    }

    va_end(ap);
}
