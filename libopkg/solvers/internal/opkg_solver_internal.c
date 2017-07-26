/* vi: set expandtab sw=4 sts=4: */
/* opkg_solver_internal.c - the opkg package management system

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
#include <xfuncs.h>
#include <stdlib.h>

#include "opkg_message.h"
#include "pkg.h"
#include "pkg_depends_internal.h"
#include "opkg_solver_internal.h"

/* adds the list of providers of the package being replaced */
static void pkg_get_provider_replacees(pkg_t *pkg,
                                       abstract_pkg_vec_t *provided_by,
                                       pkg_vec_t *replacees)
{
    int i, j;

    for (i = 0; i < provided_by->len; i++) {
        abstract_pkg_t *ap = provided_by->pkgs[i];
        if (!ap->pkgs)
            continue;
        for (j = 0; j < ap->pkgs->len; j++) {
            pkg_t *replacee = ap->pkgs->pkgs[j];
            pkg_t *old = pkg_hash_fetch_installed_by_name(pkg->name);
            /* skip pkg if installed: it  will be removed during upgrade
             * issue 8913 */
            if (old != replacee) {
                int installed = (replacee->state_status == SS_INSTALLED)
                        || (replacee->state_status == SS_UNPACKED);
                if (installed) {
                    if (!pkg_vec_contains(replacees, replacee))
                        pkg_vec_insert(replacees, replacee);
                }
            }
        }
    }
}

/*
 * Get packages which were auto_installed due to a dependency by old_pkg,
 * which are no longer a dependency in the new (upgraded) pkg.
 */
static void pkg_get_orphan_dependents(pkg_t *pkg, pkg_t *old_pkg, pkg_vec_t *orphans)
{
    int i, j, k, l, found;
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
                         "now orphaned, will remove\n", p->name);
                pkg_vec_insert(orphans, p);
            } else
                opkg_msg(INFO,
                         "%s was autoinstalled and is " "still required by %d "
                         "installed packages.\n", p->name, n_deps);

        }
    }
}


/*
 * Returns number of the number of packages depending on the packages provided by this package.
 * Every package implicitly provides itself.
 */
int pkg_has_installed_dependents(pkg_t *pkg, abstract_pkg_t ***pdependents)
{
    int nprovides = pkg->provides_count;
    abstract_pkg_t **provides = pkg->provides;
    unsigned int n_installed_dependents = 0;
    unsigned int n_deps;
    int i, j;
    for (i = 0; i < nprovides; i++) {
        abstract_pkg_t *providee = provides[i];
        abstract_pkg_t *dep_ab_pkg;

        n_deps = providee->depended_upon_by->len;
        for (j = 0; j < n_deps; j++) {
            dep_ab_pkg = providee->depended_upon_by->pkgs[j];
            int dep_installed = (dep_ab_pkg->state_status == SS_INSTALLED)
                    || (dep_ab_pkg->state_status == SS_UNPACKED);
            if (dep_installed)
                n_installed_dependents++;
        }
    }
    /* if caller requested the set of installed dependents */
    if (pdependents) {
        int p = 0;
        abstract_pkg_t **dependents =
            xcalloc((n_installed_dependents + 1), sizeof(abstract_pkg_t *));

        *pdependents = dependents;
        for (i = 0; i < nprovides; i++) {
            abstract_pkg_t *providee = provides[i];
            abstract_pkg_t *dep_ab_pkg;

            n_deps = providee->depended_upon_by->len;
            for (j = 0; j < n_deps; j++) {
                dep_ab_pkg = providee->depended_upon_by->pkgs[j];
                int installed_not_marked =
                        (dep_ab_pkg->state_status == SS_INSTALLED
                         && !(dep_ab_pkg->state_flag & SF_MARKED));
                if (installed_not_marked) {
                    dependents[p++] = dep_ab_pkg;
                    dep_ab_pkg->state_flag |= SF_MARKED;
                }
            }
        }
        dependents[p] = NULL;
        /* now clear the marks */
        for (i = 0; i < p; i++) {
            abstract_pkg_t *dep_ab_pkg = dependents[i];
            dep_ab_pkg->state_flag &= ~SF_MARKED;
        }
    }
    return n_installed_dependents;
}

int opkg_get_dependent_pkgs(pkg_t *pkg, abstract_pkg_t **dependents, pkg_vec_t *dependent_pkgs)
{
    unsigned int i;
    unsigned int a;
    int count;
    abstract_pkg_t *ab_pkg;

    ab_pkg = pkg->parent;
    if (ab_pkg == NULL) {
        opkg_msg(ERROR, "Internal error: pkg %s isn't in hash table\n",
                 pkg->name);
        return -1;
    }

    if (dependents == NULL)
        return 0;

    /* here i am using the dependencies_checked */
    if (ab_pkg->dependencies_checked == 2)      /* variable to make out whether this package */
        return 0;               /* has already been encountered in the process */
    /* of marking packages for removal - Karthik */
    ab_pkg->dependencies_checked = 2;

    count = 1;

    for (i = 0; dependents[i] != NULL; i++) {
        abstract_pkg_t *dep_ab_pkg = dependents[i];

        if (dep_ab_pkg->dependencies_checked == 2)
            continue;
        if (dep_ab_pkg->state_status == SS_INSTALLED) {
            for (a = 0; a < dep_ab_pkg->pkgs->len; a++) {
                pkg_t *dep_pkg = dep_ab_pkg->pkgs->pkgs[a];
                if (dep_pkg->state_status == SS_INSTALLED) {
                    pkg_vec_insert(dependent_pkgs, dep_pkg);
                    count++;
                }
            }
        }
        /* 1 - to keep track of visited ab_pkgs when checking for possiblility
         * of a broken removal of pkgs.
         * 2 - to keep track of pkgs whose deps have been checked alrdy  -
         * Karthik */
    }

    return 0;
}

/*
* Find  packages that were autoinstalled and will be orphaned by the removal of pkg.
*/
int opkg_get_autoinstalled_pkgs(pkg_t *pkg, pkg_vec_t *dependent_pkgs)
{
    int i, j;
    int err = 0;
    int n_deps;
    pkg_t *p;
    struct compound_depend *cdep;

    int count = pkg->pre_depends_count + pkg->depends_count
        + pkg->recommends_count + pkg->suggests_count;

    for (i = 0; i < count; i++) {
        cdep = &pkg->depends[i];
        /* Only remove dependency types which would normally be autoinstalled,
         * namely the types PREDEPEND, DEPEND and RECOMMEND. We don't consider
         * SUGGEST which are not normally autoinstalled and we don't consider
         * CONFLICTS or GREEDY_DEPEND either.
         */
        int uninteresting = cdep->type != PREDEPEND
                && cdep->type != DEPEND
                && cdep->type != RECOMMEND;
        if (uninteresting)
            continue;
        for (j = 0; j < cdep->possibility_count; j++) {
            p = pkg_hash_fetch_installed_by_name(cdep->possibilities[j]->pkg->
                                                 name);

            /* If the package is not installed, this could have
             * been a circular dependency and the package has
             * already been removed.
             */
            if (!p)
                return -1;

            if (!p->auto_installed)
                continue;

            n_deps = pkg_has_installed_dependents(p, NULL);
            if (n_deps == 1) {
                opkg_msg(NOTICE,
                         "%s was autoinstalled and is "
                         "now orphaned, will remove.\n", p->name);
                pkg_vec_insert(dependent_pkgs, p);
            } else
                opkg_msg(INFO,
                         "%s was autoinstalled and is " "still required by %d "
                         "installed packages.\n", p->name, n_deps);
        }
    }

    return err;
}

static int check_conflicts_for(pkg_t *pkg)
{
    unsigned int i;
    pkg_vec_t *conflicts;

    if (opkg_config->force_depends)
        return 0;

    conflicts = pkg_hash_fetch_conflicts(pkg);

    if (conflicts) {
        opkg_msg(ERROR, "The following packages conflict with %s:\n",
                 pkg->name);
        i = 0;
        while (i < conflicts->len)
            opkg_msg(ERROR, "\t%s", conflicts->pkgs[i++]->name);
        opkg_message(ERROR, "\n");
        pkg_vec_free(conflicts);
        return -1;
    }
    return 0;
}

/* returns number of installed replacees */
static int pkg_get_installed_replacees(pkg_t *pkg,
                                       pkg_vec_t *installed_replacees)
{
    struct compound_depend *replaces = pkg->replaces;
    int replaces_count = pkg->replaces_count;
    int i;
    unsigned int j;
    for (i = 0; i < replaces_count; i++) {
        /* Replaces field doesn't support or'ed conditionals */
        abstract_pkg_t *ab_pkg = replaces[i].possibilities[0]->pkg;

        /* If any package listed in the replacement field is a virtual (provided)
         * package, check to see if it conflicts with any abstract package that pkg
         * provides
         */
        if (ab_pkg->provided_by && pkg_conflicts_abstract(pkg, ab_pkg))
            pkg_get_provider_replacees(pkg, ab_pkg->provided_by,
                                       installed_replacees);
        pkg_vec_t *pkg_vec = ab_pkg->pkgs;
        if (pkg_vec) {
            for (j = 0; j < pkg_vec->len; j++) {
                pkg_t *replacee = pkg_vec->pkgs[j];
                if (!pkg_conflicts(pkg, replacee))
                    continue;
                if (replacee->state_status == SS_INSTALLED &&
                    version_constraints_satisfied(replaces[i].possibilities[0], pkg)) {
                    if (!pkg_vec_contains(installed_replacees, replacee))
                        pkg_vec_insert(installed_replacees, replacee);
                }
            }
        }
    }
    return installed_replacees->len;
}

/* compares versions of pkg and old_pkg, returns 0 if OK to proceed with
 * installation of pkg, 1 if the package is already up-to-date or -1 if the
 * install would be a downgrade. */
static int opkg_install_check_downgrade(pkg_t *pkg, pkg_t *old_pkg,
                                        int message)
{
    if (old_pkg) {
        int cmp = pkg_compare_versions(pkg, old_pkg);

        if (!opkg_config->download_only) {
            /* Print message. */
            char *old_version = pkg_version_str_alloc(old_pkg);
            char *new_version = pkg_version_str_alloc(pkg);
            const char *s;

            if (cmp < 0) {
                if (opkg_config->force_downgrade)
                    s = "Downgrading";
                else
                    s = "Not downgrading";
                opkg_msg(NOTICE, "%s %s from %s to %s on %s.\n", s, pkg->name,
                         old_version, new_version, old_pkg->dest->name);
            } else if (cmp == 0) {
                opkg_msg(NOTICE, "%s (%s) already installed on %s.\n",
                         pkg->name, new_version, old_pkg->dest->name);
            } else {
                /* Compare versions without force_reinstall flag to see if this
                 * is really an upgrade or a reinstall.
                 */
                int is_upgrade = pkg_compare_versions_no_reinstall(pkg, old_pkg);
                if (is_upgrade) {
                    s = "Upgrading";
                    opkg_msg(NOTICE, "%s %s from %s to %s on %s.\n", s,
                             pkg->name, old_version, new_version,
                             old_pkg->dest->name);
                } else {
                    s = "Reinstalling";
                    opkg_msg(NOTICE, "%s %s (%s) on %s.\n", s, pkg->name,
                             new_version, old_pkg->dest->name);
                }
            }

            free(old_version);
            free(new_version);
        }

        /* Do nothing if package already up-to-date. */
        if (cmp == 0)
            return 1;

        /* Do nothing if newer version is installed and we're not forcing a
         * downgrade.
         */
        if (cmp < 0 && !opkg_config->force_downgrade)
            return -1;

        /* Install is go... */
        pkg->dest = old_pkg->dest;
        return 0;
    }

    return 0;
}

static int is_provides_installed(pkg_t *pkg, int strict)
{
    int i, ret = 0;
    pkg_vec_t *available_pkgs = pkg_vec_alloc();

    pkg_hash_fetch_all_installed(available_pkgs, INSTALLED_TOBE_INSTALLED);

    for (i = 0; i < available_pkgs->len; i++) {
        pkg_t *installed_pkg = available_pkgs->pkgs[i];
        /* Return true if the installed_pkg provides pkg, is not pkg, and is
         * not set to be removed (issue 121) */
        if (is_pkg_a_provides(pkg, installed_pkg, strict)
                && (strcmp(pkg->name, installed_pkg->name))
                && (installed_pkg->state_want == SW_INSTALL)) {
            ret = 1;
            break;
        }
    }
    pkg_vec_free(available_pkgs);
    return ret;
}

static int calculate_dependencies_for(pkg_t *pkg, pkg_vec_t *pkgs_to_install, pkg_vec_t *replacees, pkg_vec_t *orphans)
{
    unsigned int i;
    int err = 0;
    pkg_vec_t *depends = pkg_vec_alloc();
    pkg_t *dep, *old_pkg;
    char **tmp, **unresolved = NULL;
    int ndepends;

    ndepends =
        pkg_hash_fetch_unsatisfied_dependencies(pkg, depends, &unresolved);

    if (unresolved && !opkg_config->force_depends) {
        opkg_msg(ERROR, "Cannot satisfy the following dependencies for %s:\n",
                 pkg->name);
        tmp = unresolved;
        while (*unresolved) {
            opkg_message(ERROR, "\t%s", *unresolved);
            free(*unresolved);
            unresolved++;
        }
        free(tmp);
        opkg_message(ERROR, "\n");
        opkg_msg(INFO,
            "This could mean that your package list is out of date or that the packages\n"
            "mentioned above do not yet exist (try 'opkg update'). To proceed in spite\n"
            "of this problem try again with the '-force-depends' option.\n");
        err = -1;
        goto cleanup;
    }

    if (ndepends <= 0) {
        pkg_vec_free(depends);
        depends = pkg_hash_fetch_satisfied_dependencies(pkg);

        for (i = 0; i < depends->len; i++) {
            dep = depends->pkgs[i];
            /* The package was uninstalled when we started, but another
             * dep earlier in this loop may have depended on it and pulled
             * it in, so check first. */
            if (is_pkg_in_pkg_vec(dep->wanted_by, pkg)) {
                opkg_msg(NOTICE, "Breaking circular dependency on %s for %s.\n",
                         pkg->name, dep->name);
                continue;
            }
            int needs_install = (dep->state_status != SS_INSTALLED)
                    && (dep->state_status != SS_UNPACKED)
                    && !is_pkg_in_pkg_vec(pkgs_to_install, dep);
            if (needs_install) {
                if (is_provides_installed(dep, 1))
                    continue;
                if (dep->dest == NULL)
                    dep->dest = opkg_config->default_dest;
                if (check_conflicts_for(dep)) {
                    err = -1;
                    goto cleanup;
                }
                pkg_vec_insert(dep->wanted_by, pkg);
                if (calculate_dependencies_for(dep, pkgs_to_install, replacees, orphans)) {
                    err = -1;
                    goto cleanup;
                }
                pkg_vec_insert(pkgs_to_install, dep);

                if (opkg_config->download_only)
                    continue;

                old_pkg = pkg_hash_fetch_installed_by_name(dep->name);

                pkg_get_installed_replacees(dep, replacees);
                /* DPKG_INCOMPATIBILITY:
                 * For upgrades, dpkg and apt-get will not remove orphaned dependents.
                 * Apt-get will instead tell the user to use apt-get autoremove to remove
                 * the autoinstalled orphaned package if it is no longer needed */
                if (old_pkg)
                    pkg_get_orphan_dependents(dep, old_pkg, orphans);
            }
        }

        goto cleanup;
    }

    /* Mark packages as to-be-installed */
    for (i = 0; i < depends->len; i++) {
        /* Dependencies should be installed the same place as pkg */
        if (depends->pkgs[i]->dest == NULL) {
            depends->pkgs[i]->dest = pkg->dest;
        }
        depends->pkgs[i]->state_want = SW_INSTALL;
    }

    for (i = 0; i < depends->len; i++) {
        dep = depends->pkgs[i];
        /* The package was uninstalled when we started, but another
         * dep earlier in this loop may have depended on it and pulled
         * it in, so check first. */
        if (!is_pkg_in_pkg_vec(dep->wanted_by, pkg))
            pkg_vec_insert(dep->wanted_by, pkg);
        int needs_install = (dep->state_status != SS_INSTALLED)
                && (dep->state_status != SS_UNPACKED)
                && !is_pkg_in_pkg_vec(pkgs_to_install, dep);
        if (needs_install) {
            if (is_provides_installed(dep, 0))
                continue;
            if (dep->dest == NULL)
                dep->dest = opkg_config->default_dest;
            if (check_conflicts_for(dep)) {
                err = -1;
                goto cleanup;
            }
            if (calculate_dependencies_for(dep, pkgs_to_install, replacees, orphans)) {
                err = -1;
                goto cleanup;
            }
            pkg_vec_insert(pkgs_to_install, dep);

            if (opkg_config->download_only)
                continue;

            old_pkg = pkg_hash_fetch_installed_by_name(dep->name);

            pkg_get_installed_replacees(dep, replacees);
            if (old_pkg)
                pkg_get_orphan_dependents(dep, old_pkg, replacees);
        }
    }

cleanup:
    pkg_vec_free(depends);

    return err;
}

int internal_solver_solv(typeId  transactionType, pkg_t *pkg, pkg_vec_t *pkgs_to_install, pkg_vec_t *replacees, pkg_vec_t *orphans)
{
    int err = 0;
    int from_upgrade = (transactionType == SOLVER_TRANSACTION_UPGRADE);
    pkg_t *old_pkg = NULL;

    /* TODO: Solving for removal is not currently done in this function, it is done on
     *       opkg_solver_remove. Logic should be migrated here.
     */
    if (transactionType == SOLVER_TRANSACTION_ERASE) {
        opkg_msg(ERROR, "Internal error: internal_solver_solv doesn't support SOLVER_TRANSACTION_ERASE");
        return -1;
    }

    if (pkg->dest == NULL) {
        pkg->dest = opkg_config->default_dest;
    }

    /* pkg already installed case */
    if (pkg->state_status == SS_INSTALLED && opkg_config->nodeps == 0) {
        err = calculate_dependencies_for(pkg, pkgs_to_install, replacees, orphans);
        if (err)
            return -1;

        opkg_msg(NOTICE, "Package %s is already installed on %s.\n", pkg->name,
                 pkg->dest->name);
        return 0;
    }

    old_pkg = pkg_hash_fetch_installed_by_name(pkg->name);

    err = opkg_install_check_downgrade(pkg, old_pkg, from_upgrade);
    if (err < 0)
        return -1;
    else if ((err == 1) && !from_upgrade)
        return 0;

    err = check_conflicts_for(pkg);
    if (err)
        return -1;

    /* solve install operation for pkg */
    if (opkg_config->nodeps == 0) {
        err = calculate_dependencies_for(pkg, pkgs_to_install, replacees, orphans);
        if (err)
            return -1;
    }

    if (opkg_config->download_only)
        return 0;

    /* add orphans for top level pkg */
    if (old_pkg)
        pkg_get_orphan_dependents(pkg, old_pkg, orphans);

    /* add top level pkg replacees to replacees vector */
    pkg_get_installed_replacees(pkg, replacees);

    return 0;
}

char *opkg_solver_version_alloc(void)
{
    return NULL;
}
