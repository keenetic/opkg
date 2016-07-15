/* vi: set expandtab sw=4 sts=4: */
/* opkg_upgrade.c - the opkg package management system

   Carl D. Worth
   Copyright (C) 2001 University of Southern California

   Copyright (C) 2003 Daniele Nicolodi <daniele@grinta.net>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
*/

#include <stdio.h>
#include <stdlib.h>

#include "opkg_upgrade_internal.h"
#include "opkg_install_internal.h"
#include "opkg_solver_internal.h"
#include "opkg_message.h"
#include "xfuncs.h"

static int opkg_prepare_upgrade_pkg(pkg_t * old, pkg_t ** pkg)
{
    pkg_t *new;
    int cmp;
    char *old_version, *new_version;

    if (old->state_flag & SF_HOLD) {
        opkg_msg(NOTICE,
                 "Not upgrading package %s which is marked "
                 "hold (flags=%#x).\n", old->name, old->state_flag);
        return 0;
    }

    if (old->state_flag & SF_REPLACE) {
        opkg_msg(NOTICE,
                 "Not upgrading package %s which is marked "
                 "replace (flags=%#x).\n", old->name, old->state_flag);
        return 0;
    }

    new = pkg_hash_fetch_best_installation_candidate_by_name(old->name);
    if (new == NULL) {
        old_version = pkg_version_str_alloc(old);
        opkg_msg(NOTICE,
                 "Assuming locally installed package %s (%s) "
                 "is up to date.\n", old->name, old_version);
        free(old_version);
        return 0;
    }

    old_version = pkg_version_str_alloc(old);
    new_version = pkg_version_str_alloc(new);

    cmp = pkg_compare_versions(old, new);
    opkg_msg(DEBUG,
             "Comparing visible versions of pkg %s:" "\n\t%s is installed "
             "\n\t%s is available " "\n\t%d was comparison result\n", old->name,
             old_version, new_version, cmp);
    if (cmp == 0) {
        opkg_msg(INFO, "Package %s (%s) installed in %s is up to date.\n",
                 old->name, old_version, old->dest->name);
        free(old_version);
        free(new_version);
        return 0;
    } else if (cmp > 0) {
        opkg_msg(NOTICE, "Not downgrading package %s on %s from %s to %s.\n",
                 old->name, old->dest->name, old_version, new_version);
        free(old_version);
        free(new_version);
        return 0;
    } else if (cmp < 0) {
        new->dest = old->dest;
        old->state_want = SW_DEINSTALL;
    }

    free(old_version);
    free(new_version);

    new->state_flag = old->state_flag;
    new->state_want = SW_INSTALL;
    /* maintain the "Auto-Installed: yes" flag */
    new->auto_installed = old->auto_installed;

    *pkg = new;
    return 1;
}

int opkg_upgrade_pkg(pkg_t * old)
{
    pkg_t *new;
    pkg_vec_t *pkgs_to_install, *replacees, *orphans;
    int r;

    r = opkg_prepare_upgrade_pkg(old, &new);
    if (r <= 0)
        return r;

    pkgs_to_install = pkg_vec_alloc();
    replacees = pkg_vec_alloc();
    orphans = pkg_vec_alloc();

    r = internal_solver_solv(SOLVER_TRANSACTION_UPGRADE, new, pkgs_to_install, replacees, orphans);
    if (r < 0)
        goto cleanup;

    r = opkg_execute_install(new, pkgs_to_install, replacees, orphans, 1);

cleanup:
    pkg_vec_free(pkgs_to_install);
    pkg_vec_free(replacees);
    pkg_vec_free(orphans);

    return r;
}

int opkg_upgrade_multiple_pkgs(pkg_vec_t * pkgs_to_upgrade)
{
    int r;
    unsigned int i;
    pkg_t *pkg, *new;
    pkg_vec_t  *upgrade_pkgs, *pkgs_to_install, *replacees, *orphans;
    int errors = 0;

    upgrade_pkgs = pkg_vec_alloc();

    /* Prepare all packages. */
    for (i = 0; i < pkgs_to_upgrade->len; i++) {
        pkg = pkgs_to_upgrade->pkgs[i];

        r = opkg_prepare_upgrade_pkg(pkg, &new);
        if (r < 0)
            errors++;
        if (r <= 0)
            continue;

        pkg_vec_insert(upgrade_pkgs, new);
    }

    for (i = 0; i < upgrade_pkgs->len; i++) {
        pkgs_to_install = pkg_vec_alloc();
        replacees = pkg_vec_alloc();
        orphans = pkg_vec_alloc();

        new = upgrade_pkgs->pkgs[i];

        r = internal_solver_solv(SOLVER_TRANSACTION_UPGRADE, new, pkgs_to_install, replacees, orphans);
        if (r < 0) {
            errors++;
            goto cleanup;
        }

        r = opkg_execute_install(new, pkgs_to_install, replacees, orphans, 1);

        if (r < 0)
            errors++;

cleanup:
        pkg_vec_free(pkgs_to_install);
        pkg_vec_free(replacees);
        pkg_vec_free(orphans);
    }

    pkg_vec_free(upgrade_pkgs);

    if (errors > 0)
        return -1;

    return 0;
}

static void pkg_hash_check_installed_pkg_helper(const char *pkg_name,
                                                void *entry, void *data)
{
    struct list_head *head = data;
    abstract_pkg_t *ab_pkg = (abstract_pkg_t *) entry;
    pkg_vec_t *pkg_vec = ab_pkg->pkgs;
    unsigned int j;

    if (!pkg_vec)
        return;

    for (j = 0; j < pkg_vec->len; j++) {
        pkg_t *pkg = pkg_vec->pkgs[j];
        int is_installed = pkg->state_status == SS_INSTALLED
                || pkg->state_status == SS_UNPACKED;
        if (is_installed)
            list_add(&pkg->list, head);
    }
}

void prepare_upgrade_list(struct list_head *head)
{
    pkg_t *old, *safe;
    LIST_HEAD(all);

    /* ensure all data is valid */
    pkg_info_preinstall_check();

    hash_table_foreach(&opkg_config->pkg_hash,
                       pkg_hash_check_installed_pkg_helper, &all);

    list_for_each_entry_safe(old, safe, &all, list) {
        pkg_t *new;

        new = pkg_hash_fetch_best_installation_candidate_by_name(old->name);

        if (new == NULL)
            continue;

        if (pkg_compare_versions(old, new) < 0)
            list_move(&old->list, head);
    }
}
