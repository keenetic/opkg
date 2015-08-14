/* vi: set expandtab sw=4 sts=4: */
/* opkg_solver.c - handle package installation and removal actions
   using an external solver.

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
#include "opkg_solver.h"
#include "opkg_message.h"

#ifdef HAVE_SOLVER_LIBSOLV
#include "opkg_solver_libsolv.h"
#endif

int opkg_solver_install(int num_pkgs, char **pkg_names)
{
    int err = 0;

#ifdef HAVE_SOLVER_LIBSOLV
    err = opkg_solver_libsolv_perform_action(JOB_INSTALL, num_pkgs, pkg_names);
#else
    err = 1;
    opkg_msg(ERROR,"Current solver does not support install!\n");
#endif
    return err;
}

int opkg_solver_remove(int num_pkgs, char **pkg_names)
{
    int err = 0;

#ifdef HAVE_SOLVER_LIBSOLV
    err = opkg_solver_libsolv_perform_action(JOB_REMOVE, num_pkgs, pkg_names);
#else
    err = 1;
    opkg_msg(ERROR,"Current solver does not support remove!\n");
#endif
    return err;
}

int opkg_solver_upgrade(int num_pkgs, char **pkg_names)
{
    int err = 0;

#ifdef HAVE_SOLVER_LIBSOLV
    err = opkg_solver_libsolv_perform_action(JOB_UPGRADE, num_pkgs, pkg_names);
#else
    err = 1;
    opkg_msg(ERROR,"Current solver does not support upgrade!\n");
#endif
    return err;
}

int opkg_solver_distupgrade(int num_pkgs, char **pkg_names)
{
    int err = 0;

#ifdef HAVE_SOLVER_LIBSOLV
    err = opkg_solver_libsolv_perform_action(JOB_DISTUPGRADE, num_pkgs, pkg_names);
#else
    err = 1;
    opkg_msg(ERROR,"Current solver does not support dist-upgrade!\n");
#endif
    return err;
}
