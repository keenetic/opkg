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

#include "pkg.h"

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
