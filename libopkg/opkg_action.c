/* vi: set expandtab sw=4 sts=4: */
/* opkg_action.c - the opkg package management system

   Copyright (C) 2015 National Instruments Corp.

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
#include "opkg_action.h"

#include <fnmatch.h>
#include <signal.h>

#include "opkg_message.h"
#include "opkg_install.h"
#include "opkg_upgrade.h"
#include "opkg_remove.h"
#include "pkg.h"

int opkg_remove(int argc, char **argv)
{
    int i, err = 0;
    unsigned int a;
    pkg_t *pkg;
    pkg_t *pkg_to_remove;
    pkg_vec_t *available;
    int done = 0;

    available = pkg_vec_alloc();
    pkg_hash_fetch_all_installed(available);

    for (i = 0; i < argc; i++) {
        for (a = 0; a < available->len; a++) {
            pkg = available->pkgs[a];
            if (fnmatch(argv[i], pkg->name, 0))
                 continue;
            if (opkg_config->restrict_to_default_dest) {
                pkg_to_remove = pkg_hash_fetch_installed_by_name_dest(pkg->name,
                opkg_config->default_dest);
            } else {
                pkg_to_remove = pkg_hash_fetch_installed_by_name(pkg->name);
            }

            if (pkg_to_remove == NULL) {
                opkg_msg(ERROR, "Package %s is not installed.\n", pkg->name);
                continue;
            }
            if (pkg->state_status == SS_NOT_INSTALLED) {
                opkg_msg(ERROR, "Package %s not installed.\n", pkg->name);
                continue;
            }

            err = opkg_remove_pkg(pkg_to_remove);
            if (!err)
                done = 1;
        }
    }

    pkg_vec_free(available);

    if (done == 0)
        opkg_msg(NOTICE, "No packages removed.\n");

    return err;
}

int opkg_install(int argc, char **argv)
{
    int i;
    char *arg;
    int err = 0;
    str_list_t *pkg_names_to_install = NULL;
    int r;

    if (opkg_config->combine)
        pkg_names_to_install = str_list_alloc();

    for (i = 0; i < argc; i++) {
        arg = argv[i];
        if (opkg_config->combine) {
            str_list_append(pkg_names_to_install, arg);
        } else {
            r = opkg_install_by_name(arg);
            if (r != 0) {
                opkg_msg(ERROR, "Cannot install package %s.\n", arg);
                err = -1;
            }
        }
    }

    if (opkg_config->combine) {
        r = opkg_install_multiple_by_name(pkg_names_to_install);
        if (r != 0)
            err = -1;
        str_list_purge(pkg_names_to_install);
    }

    return err;
}

int opkg_upgrade(int argc, char **argv)
{
    int i;
    unsigned int j;
    pkg_t *pkg;
    int err = 0;
    pkg_vec_t *pkgs_to_upgrade = NULL;
    int r;

    if (argc) {
        if (opkg_config->combine)
            pkgs_to_upgrade = pkg_vec_alloc();

        for (i = 0; i < argc; i++) {
            if (opkg_config->restrict_to_default_dest) {
                pkg = pkg_hash_fetch_installed_by_name_dest(argv[i],
                    opkg_config->default_dest);
                if (pkg == NULL) {
                    opkg_msg(ERROR, "Package %s not installed in %s.\n",
                        argv[i], opkg_config->default_dest->name);
                    continue;
                }
            } else {
                pkg = pkg_hash_fetch_installed_by_name(argv[i]);
                if (pkg == NULL) {
                    opkg_msg(ERROR, "Package %s not installed.\n", argv[i]);
                    continue;
                }
            }
            if (opkg_config->combine) {
                pkg_vec_insert(pkgs_to_upgrade, pkg);
            } else {
                r = opkg_upgrade_pkg(pkg);
                if (r != 0)
                   err = -1;
            }
        }

        if (opkg_config->combine) {
            r = opkg_upgrade_multiple_pkgs(pkgs_to_upgrade);
            if (r != 0)
                err = -1;

            pkg_vec_free(pkgs_to_upgrade);
        }
    } else {
        pkg_vec_t *installed = pkg_vec_alloc();

        pkg_hash_fetch_all_installed(installed);

        if (opkg_config->combine) {
            err = opkg_upgrade_multiple_pkgs(installed);
        } else {
            for (j = 0; j < installed->len; j++) {
                pkg = installed->pkgs[j];
                r = opkg_upgrade_pkg(pkg);
                if (r != 0)
                    err = -1;
            }
        }
        pkg_vec_free(installed);
    }

    return err;
}
