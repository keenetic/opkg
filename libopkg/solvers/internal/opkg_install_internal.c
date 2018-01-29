/* vi: set expandtab sw=4 sts=4: */
/* opkg_solver_internal.h - the opkg package management system

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

#include "opkg_install_internal.h"
#include "opkg_solver_internal.h"
#include "pkg_depends.h"
#include "opkg_install.h"
#include "opkg_remove.h"
#include "opkg_message.h"
#include "opkg_utils.h"
#include "pkg_hash.h"
#include "pkg.h"

#include <stdlib.h>

#include "xfuncs.h"

static int pkg_remove_installed(pkg_vec_t *pkgs_to_remove)
{
    int i;
    int pkgs_to_remove_count = pkgs_to_remove->len;
    for (i = 0; i < pkgs_to_remove_count; i++) {
        pkg_t *pkg = pkgs_to_remove->pkgs[i];
        int err;
        pkg->state_flag |= SF_REPLACE;     /* flag it so remove won't complain */
        err = opkg_remove_pkg(pkg);
        if (err)
            return err;
    }
    return 0;
}

/* to unwind the removal: make sure they are installed */
static int pkg_remove_installed_replacees_unwind(pkg_vec_t *replacees)
{
    int i, err;
    int replaces_count = replacees->len;
    for (i = 0; i < replaces_count; i++) {
        pkg_t *replacee = replacees->pkgs[i];
        if (replacee->state_status != SS_INSTALLED) {
            opkg_msg(DEBUG2, "Calling opkg_install_pkg.\n");
            err = opkg_install_pkg(replacee);
            if (err)
                return err;
        }
    }
    return err;
}

static int opkg_prepare_install_by_name(const char *pkg_name, pkg_t **pkg)
{
    int cmp;
    pkg_t *old, *new;
    char *old_version, *new_version, *version, *name;
    version_constraint_t constraint;

    strip_pkg_name_and_version(pkg_name, &name, &version, &constraint);
    abstract_pkg_t *ab_pkg = abstract_pkg_fetch_by_name(name);

    old = pkg_hash_fetch_installed_by_name(name);
    if (old)
        opkg_msg(DEBUG2, "Old versions from pkg_hash_fetch %s.\n",
                 old->version);

    /* A constrained version of a package was requested. The syntax to request a particular
     * version is "opkg install  <PKG_NAME>=<VERSION> */
    if (version) {
        depend_t *dependence_to_satisfy = xmalloc(sizeof(depend_t));
        dependence_to_satisfy->constraint = constraint;
        dependence_to_satisfy->version = version;
        dependence_to_satisfy->pkg = ab_pkg;

        new = pkg_hash_fetch_best_installation_candidate(
                ab_pkg,
                pkg_constraint_satisfied,
                dependence_to_satisfy,
                0,
                1);

        free(dependence_to_satisfy);
    } else {
        new = pkg_hash_fetch_best_installation_candidate_by_name(name);
    }

    free(name);
    free(version);

    if (new == NULL) {
        opkg_msg(NOTICE, "Unknown package '%s'.\n", pkg_name);
        return -1;
    }

    opkg_msg(DEBUG2, "Versions from pkg_hash_fetch:");
    if (old)
        opkg_message(DEBUG2, " old %s ", old->version);
    opkg_message(DEBUG2, " new %s\n", new->version);

    new->state_flag |= SF_USER;
    if (old) {
        old_version = pkg_version_str_alloc(old);
        new_version = pkg_version_str_alloc(new);

        cmp = pkg_compare_versions(old, new);
        if ((opkg_config->force_downgrade == 1) && (cmp > 0)) {
            /* We've been asked to allow downgrade and version is precedent */
            opkg_msg(DEBUG, "Forcing downgrade\n");
            cmp = -1;           /* then we force opkg to downgrade */
            /* We need to use a value < 0 because in the 0 case we are asking to */
            /* reinstall, and some check could fail asking the "force-reinstall" option */
        }
        opkg_msg(DEBUG,
                 "Comparing visible versions of pkg %s:" "\n\t%s is installed "
                 "\n\t%s is available " "\n\t%d was comparison result\n",
                 pkg_name, old_version, new_version, cmp);
        if (cmp == 0) {
            opkg_msg(NOTICE, "Package %s (%s) installed in %s is up to date.\n",
                     old->name, old_version, old->dest->name);
            free(old_version);
            free(new_version);
            return 0;
        } else if (cmp > 0) {
            opkg_msg(NOTICE,
                     "Not downgrading package %s on %s from %s to %s.\n",
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
    }

    new->state_want = SW_INSTALL;

    *pkg = new;
    return 1;
}

int opkg_execute_install(pkg_t *pkg, pkg_vec_t *pkgs_to_install, pkg_vec_t *replacees, pkg_vec_t *orphans, int from_upgrade)
{
    int r, errors = 0;
    unsigned int i;
    pkg_t  *dependency, *old_pkg;

    /* Add top level package to pkgs_to_install vector */
    pkg_vec_insert(pkgs_to_install, pkg);

    /* Remove orphans */
    pkg_remove_installed(orphans);

    /* Remove replacees */
    r = pkg_remove_installed(replacees);
    if (r) {
        pkg_remove_installed_replacees_unwind(replacees);
        return -1;
    }

    /* Install packages */
    for (i = 0; i < pkgs_to_install->len; i++) {
        dependency = pkgs_to_install->pkgs[i];

        /* Skip installation if
         *  1) Package is already installed (A deps B & B depends on A. Both B and A are installed, so A's processing triggers an unecessary upgrade
         *  2) Package is unpacked, which means a circular dependency already installed it for us */
        if (dependency->state_status == SS_INSTALLED ||
                        /* Circular dependency has installed it for us */
                        dependency->state_status == SS_UNPACKED)
            continue;

        if (!opkg_config->download_only) {
            opkg_msg(NOTICE, "%s %s (%s) on %s.\n", from_upgrade ? "Upgrading" : "Installing",
                     dependency->name, dependency->version, dependency->dest->name);
        }

        /* Set all pkgs to auto_installed except the top level */
        if (dependency != pkg)
            dependency->auto_installed = 1;
        r = opkg_install_pkg(dependency);
        if (r < 0)
            errors++;
    }

    if (errors) {
        if (from_upgrade) {
            /* The installation failed so we need to reset the appropriate
             * state_want flags.
             */
            old_pkg = pkg_hash_fetch_installed_by_name(pkg->name);
            if (old_pkg)
                old_pkg->state_want = SW_INSTALL;
            pkg->state_want = SW_UNKNOWN;
        }
        return -1;
    }

    return 0;
}

int opkg_install_by_name(const char *pkg_name)
{
    pkg_t *pkg;
    pkg_vec_t *pkgs_to_install, *replacees, *orphans;
    int r;

    r = opkg_prepare_install_by_name(pkg_name, &pkg);
    if (r <= 0)
        return r;

    pkgs_to_install = pkg_vec_alloc();
    replacees = pkg_vec_alloc();
    orphans = pkg_vec_alloc();

    r = internal_solver_solv(SOLVER_TRANSACTION_INSTALL, pkg, pkgs_to_install, replacees, orphans);

    if (r < 0)
        pkg->state_want = SW_UNKNOWN;
    else if (r == 0)
        r = opkg_execute_install(pkg, pkgs_to_install, replacees, orphans, 0);

    pkg_vec_free(pkgs_to_install);
    pkg_vec_free(replacees);
    pkg_vec_free(orphans);

    return r;
}

int opkg_install_multiple_by_name(str_list_t *pkg_names)
{
    str_list_elt_t *pn;
    const char *name;
    pkg_t *pkg;
    int r;
    int errors = 0;
    unsigned int i;
    pkg_vec_t *pkgs_to_install, *deps_to_install, *replacees, *orphans;
    pkgs_to_install = pkg_vec_alloc();

    /* Prepare all packages first. */
    for (pn = str_list_first(pkg_names); pn; pn = str_list_next(pkg_names, pn)) {
        name = pn->data;
        r = opkg_prepare_install_by_name(name, &pkg);
        if (r < 0)
            errors++;
        if (r <= 0)
            continue;

        pkg_vec_insert(pkgs_to_install, pkg);
    }

    for (i = 0; i < pkgs_to_install->len; i++) {
        deps_to_install = pkg_vec_alloc();
        replacees = pkg_vec_alloc();
        orphans = pkg_vec_alloc();

        pkg = pkgs_to_install->pkgs[i];

        r = internal_solver_solv(SOLVER_TRANSACTION_INSTALL, pkg, deps_to_install, replacees, orphans);
        if (r < 0) {
            pkg->state_want = SW_UNKNOWN;
            errors++;
            goto cleanup;
        } else if (r > 0) {
            /* Provides already installed, skip */
            continue;
        }

        r = opkg_execute_install(pkg, deps_to_install, replacees, orphans, 0);
        if (r)
            errors++;
cleanup:
        pkg_vec_free(deps_to_install);
        pkg_vec_free(replacees);
        pkg_vec_free(orphans);
   }

    pkg_vec_free(pkgs_to_install);
    if (errors)
        return -1;

    return 0;
}
