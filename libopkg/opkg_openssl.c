/* vi: set expandtab sw=4 sts=4: */
/* opkg_openssl.c - the opkg package management system

    Copyright (C) 2001 University of Southern California
    Copyright (C) 2008 OpenMoko Inc
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

#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/objects.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/hmac.h>

#include "opkg_openssl.h"

void
openssl_init(void)
{
    static int init = 0;

    if(!init){
	OPENSSL_config(NULL);
        OpenSSL_add_all_algorithms();
        ERR_load_crypto_strings();
        init = 1;
    }
}
