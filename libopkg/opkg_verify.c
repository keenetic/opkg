/* vi: set expandtab sw=4 sts=4: */
/* opkg_verify.c - the opkg package management system

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

#include "config.h"

#include "opkg_verify.h"

#ifdef HAVE_GPGME
#include "opkg_gpg.h"
#endif

#ifdef HAVE_OPENSSL
#include "opkg_openssl.h"
#endif

int
opkg_verify_file (char *text_file, char *sig_file)
{
#if defined HAVE_GPGME
    return opkg_verify_gpg_signature(text_file, sig_file);
#elif defined HAVE_OPENSSL
    return opkg_verify_openssl_signature(text_file, sig_file);
#else
    /* mute `unused variable' warnings. */
    (void) sig_file;
    (void) text_file;
    return 0;
#endif
}
