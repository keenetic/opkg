/* opkg_verify.h - the opkg package management system

   Copyright (C) 2014 Paul Barker

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
*/

#ifndef OPKG_VERIFY_H
#define OPKG_VERIFY_H

#ifdef __cplusplus
extern "C" {
#endif

int opkg_verify_file (char *text_file, char *sig_file);

#ifdef __cplusplus
}
#endif
#endif /* OPKG_VERIFY_H */
