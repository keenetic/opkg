/* vi: set expandtab sw=4 sts=4: */
/* pkg_depends_internal.c - the opkg package management system

   Copyright (C) 2016 National Instruments Corporation

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
*/

#include "xfuncs.h"
#include "pkg.h"
#include "opkg_message.h"
#include "pkg_depends.h"

#include <stdlib.h>

static char **merge_unresolved(char **oldstuff, char **newstuff)
{
    int oldlen = 0, newlen = 0;
    char **result;
    int i, j;

    if (!newstuff)
        return oldstuff;

    while (oldstuff && oldstuff[oldlen])
        oldlen++;
    while (newstuff && newstuff[newlen])
        newlen++;

    result = xrealloc(oldstuff, sizeof(char *) * (oldlen + newlen + 1));

    for (i = oldlen, j = 0; i < (oldlen + newlen); i++, j++)
        *(result + i) = *(newstuff + j);

    *(result + i) = NULL;

    return result;
}

/*
 * a kinda kludgy way to back out depends str from two different arrays (reg'l'r 'n pre)
 * this is null terminated, no count is carried around
 */
static char **add_unresolved_dep(pkg_t *pkg, char **the_lost, int ref_ndx)
{
    int count;
    char **resized;

    count = 0;
    while (the_lost && the_lost[count])
        count++;

    count++;                    /* need one to hold the null */
    resized = xrealloc(the_lost, sizeof(char *) * (count + 1));
    resized[count - 1] = pkg_depend_str(pkg, ref_ndx);
    resized[count] = NULL;

    return resized;
}

static int pkg_installed_and_constraint_satisfied(pkg_t *pkg, void *cdata)
{
    depend_t *depend = (depend_t *) cdata;
    return ((pkg->state_status == SS_INSTALLED || pkg->state_status == SS_UNPACKED)
        && version_constraints_satisfied(depend, pkg));
}

/* returns ndependencies or negative error value */
int pkg_hash_fetch_unsatisfied_dependencies(pkg_t *pkg,
                                            pkg_vec_t *unsatisfied,
                                            char ***unresolved)
{
    pkg_t *satisfier_entry_pkg;
    int i, j;
    unsigned int k;
    int count, found;
    char **the_lost;
    abstract_pkg_t *ab_pkg;

    /*
     * this is a setup to check for redundant/cyclic dependency checks,
     * which are marked at the abstract_pkg level
     */
    ab_pkg = pkg->parent;
    if (!ab_pkg) {
        opkg_msg(ERROR, "Internal error, with pkg %s.\n", pkg->name);
        *unresolved = NULL;
        return 0;
    }
    if (ab_pkg->dependencies_checked) { /* avoid duplicate or cyclic checks */
        opkg_msg(DEBUG2, "Already checked dependencies for '%s'.\n",
                 ab_pkg->name);
        *unresolved = NULL;
        return 0;
    } else {
        opkg_msg(DEBUG2, "Checking dependencies for '%s'.\n", ab_pkg->name);
        ab_pkg->dependencies_checked = 1;       /* mark it for subsequent visits */
    }
    count = pkg->pre_depends_count + pkg->depends_count + pkg->recommends_count
        + pkg->suggests_count;
    if (!count) {
        *unresolved = NULL;
        return 0;
    }

    the_lost = NULL;

    /* foreach dependency */
    for (i = 0; i < count; i++) {
        compound_depend_t *compound_depend = &pkg->depends[i];
        depend_t **possible_satisfiers = compound_depend->possibilities;;
        found = 0;
        satisfier_entry_pkg = NULL;

        if (compound_depend->type == GREEDY_DEPEND) {
            /* foreach possible satisfier */
            for (j = 0; j < compound_depend->possibility_count; j++) {
                /* foreach provided_by, which includes the abstract_pkg itself */
                abstract_pkg_t *abpkg = possible_satisfiers[j]->pkg;
                abstract_pkg_vec_t *ab_provider_vec = abpkg->provided_by;
                int nposs = ab_provider_vec->len;
                abstract_pkg_t **ab_providers = ab_provider_vec->pkgs;
                int l;
                for (l = 0; l < nposs; l++) {
                    pkg_vec_t *test_vec = ab_providers[l]->pkgs;
                    /* if no depends on this one, try the first package that
                     * Provides this one */
                    if (!test_vec) {
                        /* no pkg_vec hooked up to the abstract_pkg! (need another feed?) */
                        continue;
                    }

                    /* cruise this possiblity's pkg_vec looking for an installed version */
                    for (k = 0; k < test_vec->len; k++) {
                        pkg_t *pkg_scout = test_vec->pkgs[k];
                        /* not installed, and not already known about? */
                        int wanted = (pkg_scout->state_want != SW_INSTALL)
                                && !pkg_scout->parent->dependencies_checked
                                && !is_pkg_in_pkg_vec(unsatisfied, pkg_scout);
                        if (wanted) {
                            char **newstuff = NULL;
                            int rc;
                            pkg_vec_t *tmp_vec = pkg_vec_alloc();
                            /* check for not-already-installed dependencies */
                            rc = pkg_hash_fetch_unsatisfied_dependencies(pkg_scout,
                                    tmp_vec, &newstuff);
                            if (newstuff == NULL) {
                                int m;
                                int ok = 1;
                                for (m = 0; m < rc; m++) {
                                    pkg_t *p = tmp_vec->pkgs[m];
                                    if (p->state_want == SW_INSTALL)
                                        continue;
                                    opkg_msg(DEBUG,
                                             "Not installing %s due"
                                             " to requirement for %s.\n",
                                             pkg_scout->name, p->name);
                                    ok = 0;
                                    break;
                                }
                                pkg_vec_free(tmp_vec);
                                if (ok) {
                                    /* mark this one for installation */
                                    opkg_msg(NOTICE,
                                             "Adding satisfier for greedy"
                                             " dependence %s.\n",
                                             pkg_scout->name);
                                    pkg_vec_insert(unsatisfied, pkg_scout);
                                }
                            } else {
                                opkg_msg(DEBUG,
                                         "Not installing %s due to "
                                         "broken depends.\n", pkg_scout->name);
                                free(newstuff);
                            }
                        }
                    }
                }
            }

            continue;
        }

        /* foreach possible satisfier, look for installed package  */
        for (j = 0; j < compound_depend->possibility_count; j++) {
            /* foreach provided_by, which includes the abstract_pkg itself */
            depend_t *dependence_to_satisfy = possible_satisfiers[j];
            abstract_pkg_t *satisfying_apkg = possible_satisfiers[j]->pkg;
            pkg_t *satisfying_pkg = pkg_hash_fetch_best_installation_candidate(
                    satisfying_apkg,
                    pkg_installed_and_constraint_satisfied,
                    dependence_to_satisfy,
                    0,
                    1);
            opkg_msg(DEBUG, "satisfying_pkg=%p\n", satisfying_pkg);
            if (satisfying_pkg != NULL) {
                found = 1;
                break;
            }
        }
        /* if nothing installed matches, then look for uninstalled satisfier */
        if (!found) {
            /* foreach possible satisfier, look for installed package  */
            for (j = 0; j < compound_depend->possibility_count; j++) {
                /* foreach provided_by, which includes the abstract_pkg itself */
                depend_t *dependence_to_satisfy = possible_satisfiers[j];
                abstract_pkg_t *satisfying_apkg = possible_satisfiers[j]->pkg;
                pkg_t *satisfying_pkg = pkg_hash_fetch_best_installation_candidate(
                        satisfying_apkg,
                        pkg_constraint_satisfied,
                        dependence_to_satisfy,
                        0,
                        1);
                opkg_msg(DEBUG, "satisfying_pkg=%p\n", satisfying_pkg);
                if (!satisfying_pkg)
                    continue;

                /* user request overrides package recommendation */
                int ignore = (compound_depend->type == RECOMMEND
                            || compound_depend->type == SUGGEST)
                        && (satisfying_pkg->state_want == SW_DEINSTALL
                            || satisfying_pkg->state_want == SW_PURGE
                            || opkg_config->no_install_recommends
                            || str_list_contains(&opkg_config->ignore_recommends_list, satisfying_pkg->name));
                if (ignore) {
                    opkg_msg(NOTICE,
                             "%s: ignoring recommendation for "
                             "%s at user request\n", pkg->name,
                             satisfying_pkg->name);
                    continue;
                }

                /* Check for excluded packages */
                int exclude = str_list_contains(&opkg_config->exclude_list,
                                         satisfying_pkg->name);
                if (exclude) {
                    opkg_msg(NOTICE,
                             "%s: exclude required package %s at users request\n",
                             pkg->name, satisfying_pkg->name);
                    continue;
                }

                satisfier_entry_pkg = satisfying_pkg;
                break;
            }
        }

        /* we didn't find one, add something to the unsatisfied vector */
        if (!found) {
            if (!satisfier_entry_pkg) {
                /* failure to meet recommendations is not an error */
                int required = compound_depend->type != RECOMMEND
                    && compound_depend->type != SUGGEST;
                if (required)
                    the_lost = add_unresolved_dep(pkg, the_lost, i);
                else
                    opkg_msg(NOTICE, "%s: unsatisfied recommendation for %s\n",
                             pkg->name,
                             compound_depend->possibilities[0]->pkg->name);
            } else {
                if (compound_depend->type == SUGGEST) {
                    /* just mention it politely */
                    opkg_msg(NOTICE, "package %s suggests installing %s\n",
                             pkg->name, satisfier_entry_pkg->name);
                } else {
                    char **newstuff = NULL;

                    int not_seen_before = satisfier_entry_pkg != pkg
                            && !is_pkg_in_pkg_vec(unsatisfied,
                                    satisfier_entry_pkg);
                    if (not_seen_before) {
                        pkg_vec_insert(unsatisfied, satisfier_entry_pkg);
                        pkg_hash_fetch_unsatisfied_dependencies(satisfier_entry_pkg,
                                unsatisfied, &newstuff);
                        the_lost = merge_unresolved(the_lost, newstuff);
                        if (newstuff)
                            free(newstuff);
                    }
                }
            }
        }
    }
    *unresolved = the_lost;

    return unsatisfied->len;
}

pkg_vec_t *pkg_hash_fetch_satisfied_dependencies(pkg_t *pkg)
{
    pkg_vec_t *satisfiers;
    int i, j;
    unsigned int k;
    int count;
    abstract_pkg_t *ab_pkg;

    satisfiers = pkg_vec_alloc();

    /*
     * this is a setup to check for redundant/cyclic dependency checks,
     * which are marked at the abstract_pkg level
     */
    ab_pkg = pkg->parent;
    if (!ab_pkg) {
        opkg_msg(ERROR, "Internal error, with pkg %s.\n", pkg->name);
        return satisfiers;
    }

    count = pkg->pre_depends_count + pkg->depends_count + pkg->recommends_count
            + pkg->suggests_count;
    if (!count)
        return satisfiers;

    /* foreach dependency */
    for (i = 0; i < count; i++) {
        compound_depend_t *compound_depend = &pkg->depends[i];
        depend_t **possible_satisfiers = compound_depend->possibilities;;

        int not_required = compound_depend->type == RECOMMEND
                || compound_depend->type == SUGGEST;
        if (not_required)
            continue;

        if (compound_depend->type == GREEDY_DEPEND) {
            /* foreach possible satisfier */
            for (j = 0; j < compound_depend->possibility_count; j++) {
                /* foreach provided_by, which includes the abstract_pkg itself */
                abstract_pkg_t *abpkg = possible_satisfiers[j]->pkg;
                abstract_pkg_vec_t *ab_provider_vec = abpkg->provided_by;
                int nposs = ab_provider_vec->len;
                abstract_pkg_t **ab_providers = ab_provider_vec->pkgs;
                int l;
                for (l = 0; l < nposs; l++) {
                    pkg_vec_t *test_vec = ab_providers[l]->pkgs;
                    /* if no depends on this one, try the first package that
                     * Provides this one */
                    if (!test_vec) {
                        /* no pkg_vec hooked up to the abstract_pkg! (need another feed?) */
                        continue;
                    }

                    /* cruise this possiblity's pkg_vec looking for an installed version */
                    for (k = 0; k < test_vec->len; k++) {
                        pkg_t *pkg_scout = test_vec->pkgs[k];
                        /* not installed, and not already known about? */
                        int not_seen_before = pkg_scout != pkg
                                && pkg_scout->state_want == SW_INSTALL;
                        if (not_seen_before)
                            pkg_vec_insert(satisfiers, pkg_scout);
                    }
                }
            }

            continue;
        }

        /* foreach possible satisfier, look for installed package  */
        for (j = 0; j < compound_depend->possibility_count; j++) {
            /* foreach provided_by, which includes the abstract_pkg itself */
            depend_t *dependence_to_satisfy = possible_satisfiers[j];
            abstract_pkg_t *satisfying_apkg = possible_satisfiers[j]->pkg;
            pkg_t *satisfying_pkg = pkg_hash_fetch_best_installation_candidate(satisfying_apkg,
                                                           pkg_constraint_satisfied,
                                                           dependence_to_satisfy,
                                                           1,
                                                           0);
            int need_to_insert = satisfying_pkg != NULL
                    && satisfying_pkg != pkg
                    && (satisfying_pkg->state_want == SW_INSTALL
                        || satisfying_pkg->state_want == SW_UNKNOWN);
            if (need_to_insert)
                pkg_vec_insert(satisfiers, satisfying_pkg);
        }
    }
    return satisfiers;
}

/*checking for conflicts !in replaces
  If a packages conflicts with another but is also replacing it, I should not
  consider it a really conflicts
  returns 0 if conflicts <> replaces or 1 if conflicts == replaces
*/
static int is_pkg_a_replaces(pkg_t *pkg_scout, pkg_t *pkg)
{
    int i;
    int replaces_count = pkg->replaces_count;
    struct compound_depend *replaces;

    if (pkg->replaces_count == 0)       /* No replaces, it's surely a conflict */
        return 0;

    replaces = pkg->replaces;

    for (i = 0; i < replaces_count; i++) {
        /* Replaces field doesn't support or'ed conditions */
        if (version_constraints_satisfied(replaces->possibilities[0], pkg_scout)) {  /* Found */
            opkg_msg(DEBUG2, "Seems I've found a replace %s %s\n",
                     pkg_scout->name, replaces[i].possibilities[0]->pkg->name);
            return 1;
        }
    }
    return 0;
}

int is_pkg_a_provides(const pkg_t *pkg_scout, const pkg_t *pkg, int strict)
{
    int i;

    for (i = 0; i < pkg->provides_count; i++) {
        if (strcmp(pkg_scout->name, pkg->provides[i]->name) == 0) {     /* Found */
            opkg_msg(DEBUG2, "Seems I've found a provide %s %s\n",
                     pkg_scout->name, pkg->provides[i]->name);
            return 1;
        } else if (strict) {
            /* if a pkg_scout only Provides one virtual package, then also check if
             * the pkg_scout Provides is provided by pkg. This check is needed when
	     * looking to satisy a dependency. For satisfied dependencies, we want to
	     * stick with a pkg name check
	     * */
            if ((pkg_scout->provides_count == 2)
                && strcmp(pkg_scout->provides[1]->name, pkg->provides[i]->name) == 0) { /* Found */
                opkg_msg(DEBUG2, "Seems I've found a provide %s %s\n",
                         pkg_scout->name, pkg->provides[i]->name);
                return 1;
            }
        }
    }

    return 0;
}

static void __pkg_hash_fetch_conflicts(pkg_t *pkg,
                                       pkg_vec_t *installed_conflicts)
{
    pkg_vec_t *test_vec;
    compound_depend_t *conflicts;
    depend_t **possible_satisfiers;
    depend_t *possible_satisfier;
    int j;
    unsigned int i, k;
    unsigned int count;
    pkg_t **pkg_scouts;
    pkg_t *pkg_scout;

    conflicts = pkg->conflicts;
    if (!conflicts) {
        return;
    }

    count = pkg->conflicts_count;

    /* foreach conflict */
    for (i = 0; i < count; i++) {

        possible_satisfiers = conflicts->possibilities;

        /* foreach possible satisfier */
        for (j = 0; j < conflicts->possibility_count; j++) {
            possible_satisfier = possible_satisfiers[j];
            if (!possible_satisfier) {
                opkg_msg(ERROR, "Internal error: possible_satisfier=NULL\n");
                continue;
            }
            if (!possible_satisfier->pkg) {
                opkg_msg(ERROR,
                         "Internal error: possible_satisfier->pkg=NULL\n");
                continue;
            }
            test_vec = possible_satisfier->pkg->pkgs;
            if (test_vec) {
                /* pkg_vec found, it is an actual package conflict
                 * cruise this possiblity's pkg_vec looking for an installed version */
                pkg_scouts = test_vec->pkgs;
                for (k = 0; k < test_vec->len; k++) {
                    pkg_scout = pkg_scouts[k];
                    if (!pkg_scout) {
                        opkg_msg(ERROR, "Internal error: pkg_scout=NULL\n");
                        continue;
                    }
                    int is_new_conflict =
                            (pkg_scout->state_status == SS_INSTALLED
                                || pkg_scout->state_want == SW_INSTALL)
                            && version_constraints_satisfied(possible_satisfier,
                                                             pkg_scout)
                            && !is_pkg_a_replaces(pkg_scout, pkg)
                            && !is_pkg_in_pkg_vec(installed_conflicts,
                                                  pkg_scout);
                    if (is_new_conflict) {
                        pkg_vec_insert(installed_conflicts, pkg_scout);
                    }
                }
            }
        }
        conflicts++;
    }
}

static void __pkg_hash_fetch_conflictees(pkg_t *pkg,
                                         pkg_vec_t *installed_conflicts)
{
    int i;

    pkg_vec_t *available_pkgs = pkg_vec_alloc();
    pkg_hash_fetch_all_installed(available_pkgs, INSTALLED);

    for (i = 0; i < available_pkgs->len; i++) {
        pkg_t *cpkg = available_pkgs->pkgs[i];
        int is_new_conflict = pkg_conflicts(cpkg, pkg)
                && strcmp(cpkg->name, pkg->name)
                && !is_pkg_in_pkg_vec(installed_conflicts, cpkg)
                && !pkg_replaces(pkg, cpkg);
        if (is_new_conflict)
            pkg_vec_insert(installed_conflicts, cpkg);
    }

    pkg_vec_free(available_pkgs);
}

pkg_vec_t *pkg_hash_fetch_conflicts(pkg_t *pkg)
{
    pkg_vec_t *installed_conflicts;
    abstract_pkg_t *ab_pkg;

    /*
     * this is a setup to check for redundant/cyclic dependency checks,
     * which are marked at the abstract_pkg level
     */
    ab_pkg = pkg->parent;
    if (!ab_pkg) {
        opkg_msg(ERROR, "Internal error: %s not in hash table\n", pkg->name);
        return (pkg_vec_t *) NULL;
    }

    installed_conflicts = pkg_vec_alloc();

    __pkg_hash_fetch_conflicts(pkg, installed_conflicts);
    __pkg_hash_fetch_conflictees(pkg, installed_conflicts);

    if (installed_conflicts->len)
        return installed_conflicts;
    pkg_vec_free(installed_conflicts);
    return (pkg_vec_t *) NULL;
}
