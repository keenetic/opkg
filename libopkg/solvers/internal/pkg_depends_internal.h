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

#ifndef PKG_DEPENDS_INTERNAL_H
#define PKG_DEPENDS_INTERNAL_H

#include "pkg.h"

#ifdef __cplusplus
extern "C" {
#endif

int pkg_hash_fetch_unsatisfied_dependencies(pkg_t *pkg,
                                            pkg_vec_t *depends,
                                            char ***unresolved);
pkg_vec_t *pkg_hash_fetch_satisfied_dependencies(pkg_t *pkg);
pkg_vec_t *pkg_hash_fetch_conflicts(pkg_t *pkg);
int is_pkg_a_provides(const pkg_t *pkg_scout, const pkg_t *pkg,
                                            int strict);

#ifdef __cplusplus
}
#endif
#endif    /* PKG_DEPENDS_INTERNAL_H */
