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
#include "opkg_install.h"
#include "opkg_remove.h"
#include "opkg_message.h"
#include "pkg_hash.h"
#include "pkg.h"

#include <stdlib.h>

/*
 * Remove packages which were auto_installed due to a dependency by old_pkg,
 * which are no longer a dependency in the new (upgraded) pkg.
 */
int pkg_remove_orphan_dependent(pkg_t *pkg, pkg_t *old_pkg)
{
    int i, j, k, l, found, r, err = 0;
    int n_deps;
    pkg_t *p;
    struct compound_depend *cd0, *cd1;
    abstract_pkg_t **dependents;

    int count0 = old_pkg->pre_depends_count + old_pkg->depends_count
        + old_pkg->recommends_count + old_pkg->suggests_count;
    int count1 = pkg->pre_depends_count + pkg->depends_count
        + pkg->recommends_count + pkg->suggests_count;

    for (i = 0; i < count0; i++) {
        cd0 = &old_pkg->depends[i];
        if (cd0->type != DEPEND && cd0->type != RECOMMEND)
            continue;
        for (j = 0; j < cd0->possibility_count; j++) {
            found = 0;

            for (k = 0; k < count1; k++) {
                cd1 = &pkg->depends[k];
                if (cd1->type != DEPEND && cd1->type != RECOMMEND)
                    continue;
                for (l = 0; l < cd1->possibility_count; l++) {
                    if (cd0->possibilities[j]->pkg == cd1->possibilities[l]->pkg) {
                        found = 1;
                        break;
                    }
                }
                if (found)
                    break;
            }

            if (found)
                continue;

            /*
             * old_pkg has a dependency that pkg does not.
             */
            p = pkg_hash_fetch_installed_by_name(cd0->possibilities[j]->pkg->
                                                 name);

            if (!p)
                continue;

            if (!p->auto_installed)
                continue;

            n_deps = pkg_has_installed_dependents(p, &dependents);
            n_deps--;           /* don't count old_pkg */

            if (n_deps == 0) {
                opkg_msg(NOTICE,
                         "%s was autoinstalled and is "
                         "now orphaned, removing.\n", p->name);

                /* p has one installed dependency (old_pkg),
                 * which we need to ignore during removal. */
                p->state_flag |= SF_REPLACE;

                r = opkg_remove_pkg(p);
                if (!err)
                    err = r;
            } else
                opkg_msg(INFO,
                         "%s was autoinstalled and is " "still required by %d "
                         "installed packages.\n", p->name, n_deps);

        }
    }

    return err;
}

int pkg_remove_installed_replacees(pkg_vec_t *replacees)
{
    int i;
    int replaces_count = replacees->len;
    for (i = 0; i < replaces_count; i++) {
        pkg_t *replacee = replacees->pkgs[i];
        int err;
        replacee->state_flag |= SF_REPLACE;     /* flag it so remove won't complain */
        err = opkg_remove_pkg(replacee);
        if (err)
            return err;
    }
    return 0;
}

/* to unwind the removal: make sure they are installed */
int pkg_remove_installed_replacees_unwind(pkg_vec_t *replacees)
{
    int i, err;
    int replaces_count = replacees->len;
    for (i = 0; i < replaces_count; i++) {
        pkg_t *replacee = replacees->pkgs[i];
        if (replacee->state_status != SS_INSTALLED) {
            opkg_msg(DEBUG2, "Calling opkg_install_pkg.\n");
            err = opkg_install_pkg(replacee, 0);
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
    char *old_version, *new_version;

    old = pkg_hash_fetch_installed_by_name(pkg_name);
    if (old)
        opkg_msg(DEBUG2, "Old versions from pkg_hash_fetch %s.\n",
                 old->version);

    new = pkg_hash_fetch_best_installation_candidate_by_name(pkg_name);
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

int opkg_install_by_name(const char *pkg_name)
{
    pkg_t *pkg;
    int r;

    r = opkg_prepare_install_by_name(pkg_name, &pkg);
    if (r <= 0)
        return r;

    opkg_msg(DEBUG2, "Calling opkg_install_pkg for %s %s.\n", pkg->name,
             pkg->version);
    return opkg_install_pkg(pkg, 0);
}

int opkg_install_multiple_by_name(str_list_t *pkg_names)
{
    str_list_elt_t *pn;
    const char *name;
    pkg_t *pkg;
    int r;
    int errors = 0;
    unsigned int i;
    pkg_vec_t *pkgs_to_install = pkg_vec_alloc();

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

    /* Now install all packages. */
    for (i = 0; i < pkgs_to_install->len; i++) {
        pkg = pkgs_to_install->pkgs[i];

        opkg_msg(DEBUG2, "Calling opkg_install_pkg for %s %s.\n", pkg->name,
                 pkg->version);
        r = opkg_install_pkg(pkg, 0);
        if (r < 0)
            errors++;
    }

    pkg_vec_free(pkgs_to_install);
    if (errors)
        return -1;

    return 0;
}
