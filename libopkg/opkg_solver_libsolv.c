/* vi: set expandtab sw=4 sts=4: */
/* opkg_solver_libsolv.c - handle package dependency solving with
   calls to libsolv.

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

#include <stdlib.h>

#include <solv/pool.h>
#include <solv/poolarch.h>
#include <solv/queue.h>
#include <solv/repo.h>
#include <solv/solver.h>
#include <solv/solverdebug.h>

#include "opkg_solver_libsolv.h"
#include "opkg_install.h"
#include "opkg_remove.h"
#include "opkg_message.h"
#include "pkg_vec.h"
#include "pkg_hash.h"
#include "xfuncs.h"

#define INITIAL_ARCH_LIST_SIZE 4

/* Priority values (the high priority 99 was taken from libsolv's
   examples/solv.c) These values are aribtrary values such that
   1 < PRIORITY_PREFERRED < PRIORITY_MARKED_FOR_INSTALL so that
   packages marked to be installed have higher priority than packages
   marked as preferred which have higher priority ofer other pakcages */
#define PRIORITY_PREFERRED 90
#define PRIORITY_MARKED_FOR_INSTALL 99

static int compare_arch_priorities(const void *p1, const void *p2)
{
    int priority1 = ((arch_data_t *)p1)->priority;
    int priority2 = ((arch_data_t *)p2)->priority;

    if (priority1 < priority2)
        return -1;
    else if (priority1 == priority2)
        return 0;
    else
        return 1;
}

static void libsolv_solver_set_arch_policy(libsolv_solver_t *libsolv_solver)
{
    int arch_list_size = INITIAL_ARCH_LIST_SIZE;
    int arch_count = 0;
    char *arch_policy;

    arch_data_t *archs = xcalloc(arch_list_size, sizeof(arch_data_t));

    nv_pair_list_elt_t *arch_info;

    list_for_each_entry(arch_info, &opkg_config->arch_list.head, node) {
        if (arch_count > arch_list_size)
            archs = xrealloc(archs, arch_list_size *= 2);

        archs[arch_count].arch = ((nv_pair_t *)(arch_info->data))->name;
        archs[arch_count].priority = atoi(((nv_pair_t *)
                                           (arch_info->data))->value);
        arch_count++;
    }

    /* if there are no architectures, set the policy to accept packages with
       architectures noarch and all */
    if (!arch_count) {
        arch_policy = "all=noarch";
        goto SET_ARCH_POLICY_AND_EXIT;
    }

    qsort(archs, arch_count, sizeof(arch_data_t), compare_arch_priorities);

    int j = 0;
    arch_policy = archs[j].arch;
    int prev_priority = archs[j].priority;

    /* libsolv's arch policy is a string of archs seperated by : < or =
       depending on priorities.
       A=B means arch A and arch B have equal priority.
       A:B means A is better than B and if a package with A is available,
           packages with arch B are not considered
       A>B means A is better than B, but a package of arch B may be installed
           if its version is better than a package of arch A

       TODO: this should not be necessary as ':' should work in both cases if
       SOLVER_FLAG_NO_INFARCHCHECK works as documented and does not check whether
       packages are of inferior architectures see libsolv issue #98 */
    char *better_arch_delimiter = opkg_config->prefer_arch_to_version ? ":" : ">";

    for (j = 1; j < arch_count; j++) {
        int priority = archs[j].priority;
        char *arch_delimiter = (prev_priority == priority) ?
                           "=" : better_arch_delimiter;
        arch_policy = pool_tmpjoin(libsolv_solver->pool, archs[j].arch,
                                   arch_delimiter, arch_policy);
        prev_priority = archs[j].priority;
    }

 SET_ARCH_POLICY_AND_EXIT:
    free(archs);
    opkg_msg(DEBUG2, "libsolv arch policy: %s\n", arch_policy);
    pool_setarchpolicy(libsolv_solver->pool, arch_policy);
}

/* This transforms a compound depend [e.g. A (=3.0)|C (>=2.0) ]
   into an id usable by libsolv. */
static Id dep2id(Pool *pool, compound_depend_t *dep)
{
    Id nameId = NULL;
    Id versionId = NULL;
    Id flagId = 0;
    Id previousId = NULL;
    Id dependId = NULL;

    int i;

    for (i = 0; i < dep->possibility_count; ++i) {
        depend_t *depend = dep->possibilities[i];

        nameId = pool_str2id(pool, depend->pkg->name, 1);
        if (depend->version) {
            versionId = pool_str2id(pool, depend->version, 1);

            switch (depend->constraint) {
            case EARLIER:
                flagId = REL_LT;
                break;
            case EARLIER_EQUAL:
                flagId = REL_LT | REL_EQ;
                break;
            case EQUAL:
                flagId = REL_EQ;
                break;
            case LATER:
                flagId = REL_GT;
                break;
            case LATER_EQUAL:
                flagId = REL_GT | REL_EQ;
                break;
            default:
                break;
            }
            dependId = pool_rel2id(pool, nameId, versionId, flagId, 1);
        } else {
            dependId = nameId;
        }

        if (previousId)
            dependId = pool_rel2id(pool, dependId, previousId, REL_OR, 1);

        previousId = dependId;
    }
    return dependId;
}

static void pkg2solvable(pkg_t *pkg, Solvable *solvable_out)
{
    if (!solvable_out) {
        opkg_msg(ERROR, "Solvable undefined!\n");
        return;
    }

    int i;
    Id dependId;
    Repo *repo = solvable_out->repo;
    Pool *pool = repo->pool;

    char *version = pkg_version_str_alloc(pkg);
    Id versionId = pool_str2id(pool, version, 1);
    free(version);

    /* set the solvable version */
    solvable_out->evr = versionId;

    /* set the solvable name */
    Id nameId = pool_str2id(pool, pkg->name, 1);
    solvable_out->name = nameId;

    /* set the architecture of the solvable */
    solvable_out->arch = pool_str2id(pool, pkg->architecture, 1);

    /* set the solvable's dependencies (depends, recommends, suggests) */
    if (pkg->depends && !opkg_config->nodeps) {
        unsigned int deps_count = pkg->depends_count + pkg->pre_depends_count
            + pkg->recommends_count + pkg->suggests_count;

        for (i = 0; i < deps_count; ++i) {
            compound_depend_t dep = pkg->depends[i];
            dependId = dep2id(pool, &dep);
            switch (dep.type) {
            /* pre-depends is not supported by opkg. However, the
               field is parsed in pkg_parse. This will handle the
               pre-depends case if a package control happens to have it*/
            case PREDEPEND:
                opkg_msg(DEBUG2, "%s pre-depends %s\n", pkg->name,
                         pool_dep2str(pool, dependId));
                solvable_out->requires = repo_addid_dep(repo,
                         solvable_out->requires, dependId,
                         SOLVABLE_PREREQMARKER);
            case DEPEND:
                opkg_msg(DEBUG2, "%s depends %s\n", pkg->name,
                         pool_dep2str(pool, dependId));
                solvable_out->requires = repo_addid_dep(repo,
                         solvable_out->requires, dependId,
                         -SOLVABLE_PREREQMARKER);
                break;
            case RECOMMEND:
                opkg_msg(DEBUG2, "%s recommends %s\n", pkg->name,
                         pool_dep2str(pool, dependId));
                solvable_out->recommends = repo_addid_dep(repo,
                         solvable_out->recommends, dependId, 0);
                break;
            case SUGGEST:
                opkg_msg(DEBUG2, "%s suggests %s\n", pkg->name,
                         pool_dep2str(pool, dependId));
                solvable_out->suggests = repo_addid_dep(repo,
                         solvable_out->suggests, dependId, 0);
                break;
            default:
                break;
            }
        }
    }

    /* set solvable conflicts */
    if (pkg->conflicts) {
        for (i = 0; i < pkg->conflicts_count; i++) {
            compound_depend_t dep = pkg->conflicts[i];

            dependId = dep2id(pool, &dep);
            opkg_msg(DEBUG2, "%s conflicts %s\n", pkg->name,
                     pool_dep2str(pool, dependId));
            solvable_out->conflicts = repo_addid_dep(repo,
                     solvable_out->conflicts, dependId, 0);
        }
    }

    /* set solvable provides */
    if (pkg->provides_count) {
        for (i = 0; i < pkg->provides_count; i++) {
            abstract_pkg_t *provide = pkg->provides[i];

            /* we will handle the case where a package provides itself
               separately */
            if (strcmp(pkg->name, provide->name) != 0) {
                opkg_msg(DEBUG2, "%s provides %s\n", pkg->name, provide->name);
                Id provideId = pool_str2id(pool, provide->name, 1);
                solvable_out->provides = repo_addid_dep(repo,
                    solvable_out->provides, provideId, 0);
            }
        }
    }
    /* every package provides itself in its own version */
    solvable_out->provides = repo_addid_dep(repo, solvable_out->provides,
        pool_rel2id(pool, nameId, versionId, REL_EQ, 1),
        0);

    /* set solvable obsoletes. Obsoletes is libsolv's equivalent of Replaces*/
    if (pkg->replaces_count) {
        for (i = 0; i < pkg->replaces_count; i++) {
            abstract_pkg_t *replaces = pkg->replaces[i];

            opkg_msg(DEBUG2, "%s replaces %s\n", pkg->name, replaces->name);
            Id replacesId = pool_str2id(pool, replaces->name, 1);
            solvable_out->obsoletes = repo_addid_dep(repo,
                solvable_out->obsoletes, replacesId, 0);
        }
    }
}

static void populate_installed_repo(libsolv_solver_t *libsolv_solver)
{
    int i;

    pkg_vec_t *installed_pkgs = pkg_vec_alloc();

    pkg_hash_fetch_all_installed(installed_pkgs);

    for (i = 0; i < installed_pkgs->len; i++) {
        pkg_t *pkg = installed_pkgs->pkgs[i];

        char *version = pkg_version_str_alloc(pkg);

        opkg_message(DEBUG2, "Installed package: %s - %s\n",
                     pkg->name, version);

        free(version);

        /* add a new solvable to the installed packages repo */
        Id solvable_id = repo_add_solvable(libsolv_solver->repo_installed);
        Solvable *solvable = pool_id2solvable(libsolv_solver->pool,
                                              solvable_id);

        /* set solvable attributes */
        pkg2solvable(pkg, solvable);

        /* if the package is not autoinstalled, mark it as user installed */
        if (!pkg->auto_installed)
            queue_push2(&libsolv_solver->solver_jobs, SOLVER_SOLVABLE
                        | SOLVER_USERINSTALLED, solvable_id);

        /* if the package is held, mark it as locked */
        if (pkg->state_flag & SF_HOLD)
            queue_push2(&libsolv_solver->solver_jobs, SOLVER_SOLVABLE
                        | SOLVER_LOCK, solvable_id);

        /* if the --force-depends option is specified, make dependencies weak */
        if (opkg_config->force_depends)
            queue_push2(&libsolv_solver->solver_jobs, SOLVER_SOLVABLE
                        | SOLVER_WEAKENDEPS, solvable_id);
    }

    pkg_vec_free(installed_pkgs);
}

static void populate_available_repos(libsolv_solver_t *libsolv_solver)
{
    int i;
    Solvable *solvable;
    Id solvable_id;

    pkg_vec_t *available_pkgs = pkg_vec_alloc();

    pkg_hash_fetch_available(available_pkgs);

    for (i = 0; i < available_pkgs->len; i++) {
        pkg_t *pkg = available_pkgs->pkgs[i];

        /* if the package is marked as excluded, skip it */
        if (str_list_contains(&opkg_config->exclude_list, pkg->name))
            continue;

        /* if the package is installed or unpacked, skip it */
        if (pkg->state_status == SS_INSTALLED ||
            pkg->state_status == SS_UNPACKED)
            continue;

        /* if the package is marked as held, skip it */
        if (pkg->state_flag & SF_HOLD)
            continue;

        char *version = pkg_version_str_alloc(pkg);

        /* if the package is to be installed, create a solvable in
           repo_to_install and create a job to install it */
        if (pkg->state_want == SW_INSTALL) {
            opkg_message(DEBUG2, "Package marked for install: %s - %s\n",
                         pkg->name, version);
            solvable_id = repo_add_solvable(libsolv_solver->repo_to_install);
            queue_push2(&libsolv_solver->solver_jobs, SOLVER_SOLVABLE
                        | SOLVER_INSTALL | SOLVER_SETEVR, solvable_id);
        }
        /* if the package is preferred, create a solvable in repo_preferred */
        else if (pkg->state_flag & SF_PREFER) {
            opkg_message(DEBUG2, "Preferred package: %s - %s\n",
                         pkg->name, version);
            solvable_id = repo_add_solvable(libsolv_solver->repo_preferred);
        }
        /* otherwise, create a solvable in repo_available */
        else {
            opkg_message(DEBUG2, "Available package: %s - %s\n",
                         pkg->name, version);
            solvable_id = repo_add_solvable(libsolv_solver->repo_available);
        }

        free(version);

        /* set solvable attributes using the package */
        solvable = pool_id2solvable(libsolv_solver->pool, solvable_id);
        pkg2solvable(pkg, solvable);

        /* if the --force-depends option is specified make dependencies weak */
        if (opkg_config->force_depends)
            queue_push2(&libsolv_solver->solver_jobs, SOLVER_SOLVABLE
                        | SOLVER_WEAKENDEPS, solvable_id);
    }

    pkg_vec_free(available_pkgs);
}

static void libsolv_solver_init(libsolv_solver_t *libsolv_solver)
{
    /* initialize the solver job queue */
    queue_init(&libsolv_solver->solver_jobs);

    /* create the solvable pool and repos */
    libsolv_solver->pool = pool_create();
    libsolv_solver->repo_installed = repo_create(libsolv_solver->pool,
                                                 "@System");
    libsolv_solver->repo_available = repo_create(libsolv_solver->pool,
                                                 "@Available");
    libsolv_solver->repo_preferred = repo_create(libsolv_solver->pool,
                                                 "@Preferred");
    libsolv_solver->repo_to_install = repo_create(libsolv_solver->pool,
                                                  "@To_Install");

    /* set the repo priorities */
    libsolv_solver->repo_preferred->priority = PRIORITY_PREFERRED;
    libsolv_solver->repo_to_install->priority = PRIORITY_MARKED_FOR_INSTALL;

    /* set the architecture policy for the solver pool */
    libsolv_solver_set_arch_policy(libsolv_solver);

    /* set libsolv pool flags to match provides behavior of opkg.
       Obsoletes is libsolv's equivalent of replaces */
    pool_set_flag(libsolv_solver->pool, POOL_FLAG_OBSOLETEUSESPROVIDES, 1);
    pool_set_flag(libsolv_solver->pool,
                  POOL_FLAG_IMPLICITOBSOLETEUSESPROVIDES, 1);

    /* read in repo of installed packages */
    populate_installed_repo(libsolv_solver);

    /* set the installed repo */
    pool_set_installed(libsolv_solver->pool, libsolv_solver->repo_installed);

    /* read in available packages */
    populate_available_repos(libsolv_solver);

    /* create index of what each package provides */
    pool_createwhatprovides(libsolv_solver->pool);

    /* create the solver with the solver pool */
    libsolv_solver->solver = solver_create(libsolv_solver->pool);

    /* set libsolv solver flags to match behavoir of opkg options */
    if (opkg_config->force_removal_of_dependent_packages)
        solver_set_flag(libsolv_solver->solver,
                        SOLVER_FLAG_ALLOW_UNINSTALL, 1);
    if (opkg_config->force_downgrade)
        solver_set_flag(libsolv_solver->solver,
                        SOLVER_FLAG_ALLOW_DOWNGRADE, 1);
    if (opkg_config->no_install_recommends)
        solver_set_flag(libsolv_solver->solver,
                        SOLVER_FLAG_IGNORE_RECOMMENDED, 1);
    if (!opkg_config->prefer_arch_to_version) {
        solver_set_flag(libsolv_solver->solver,
                        SOLVER_FLAG_ALLOW_ARCHCHANGE, 1);
        solver_set_flag(libsolv_solver->solver,
                        SOLVER_FLAG_NO_INFARCHCHECK, 1);
    }
}

libsolv_solver_t *libsolv_solver_new(void)
{
    libsolv_solver_t *libsolv_solver;

    libsolv_solver = xcalloc(1, sizeof(libsolv_solver_t));
    libsolv_solver_init(libsolv_solver);

    return libsolv_solver;
}

void libsolv_solver_add_job(libsolv_solver_t *libsolv_solver,
                            job_action_t action, char *pkg_name)
{
    Id what = 0;
    Id how = 0;

    what = pool_str2id(libsolv_solver->pool, pkg_name, 1);

    switch (action) {
    case JOB_INSTALL:
        how = SOLVER_INSTALL | SOLVER_SOLVABLE_NAME;
        break;
    case JOB_REMOVE:
        how = SOLVER_ERASE | SOLVER_SOLVABLE_NAME;
        break;
    case JOB_UPGRADE:
        if (pkg_name && strcmp(pkg_name, "") != 0) {
            how = SOLVER_UPDATE | SOLVER_SOLVABLE_NAME;
        } else {
            how = SOLVER_UPDATE | SOLVER_SOLVABLE_REPO;
            what = libsolv_solver->pool->installed->repoid;
        }
        break;
    case JOB_DISTUPGRADE:
        if (pkg_name && strcmp(pkg_name, "") != 0) {
            how = SOLVER_DISTUPGRADE | SOLVER_SOLVABLE_NAME;
        } else {
            how = SOLVER_DISTUPGRADE | SOLVER_SOLVABLE_ALL;
            queue_push2(&libsolv_solver->solver_jobs, SOLVER_DROP_ORPHANED | SOLVER_SOLVABLE_ALL, 0);
        }
        break;
    case JOB_NOOP:
    default:
        break;
    }

    if (opkg_config->autoremove)
        how |= SOLVER_CLEANDEPS;

    queue_push2(&libsolv_solver->solver_jobs, how, what);
}

int libsolv_solver_solve(libsolv_solver_t *libsolv_solver)
{
    int problem_count = solver_solve(libsolv_solver->solver,
                                     &libsolv_solver->solver_jobs);

    /* print out all problems and recommended solutions */
    if (problem_count) {
        opkg_message(NOTICE, "Solver encountered %d problem(s):\n", problem_count);

        int problem;
        /* problems start at 1, not 0 */
        for (problem = 1; problem <= problem_count; problem++) {
            opkg_message(NOTICE, "Problem %d/%d:\n", problem, problem_count);
            opkg_message(NOTICE, "  - %s\n",
                         solver_problem2str(libsolv_solver->solver, problem));
            opkg_message(NOTICE, "\n");

            int solution_count = solver_solution_count(libsolv_solver->solver,
                                                       problem);
            int solution;

            /* solutions also start from 1 */
            for (solution = 1; solution <= solution_count; solution++) {
                opkg_message(NOTICE, "Solution %d:\n", solution);
                solver_printsolution(libsolv_solver->solver, problem, solution);
                opkg_message(NOTICE, "\n");
            }
        }
    }

    return problem_count;
}

void libsolv_solver_free(libsolv_solver_t *libsolv_solver)
{
    solver_free(libsolv_solver->solver);
    queue_free(&libsolv_solver->solver_jobs);
    pool_free(libsolv_solver->pool);
    free(libsolv_solver);
}

int libsolv_solver_execute_transaction(libsolv_solver_t *libsolv_solver)
{
    int i, ret = 0, err = 0;
    Transaction *transaction;

    transaction = solver_create_transaction(libsolv_solver->solver);

    if (!transaction->steps.count) {
        opkg_message(NOTICE, "No packages installed or removed.\n");
    } else {
        /* order the transaction so dependencies are handled first */
        transaction_order(transaction, 0);

        for (i = 0; i < transaction->steps.count; i++) {
            Id stepId = transaction->steps.elements[i];
            Solvable *solvable = pool_id2solvable(libsolv_solver->pool, stepId);
            Id typeId = transaction_type(transaction, stepId,
                    SOLVER_TRANSACTION_SHOW_ACTIVE |
                    SOLVER_TRANSACTION_CHANGE_IS_REINSTALL);

            const char *pkg_name = pool_id2str(libsolv_solver->pool, solvable->name);
            const char *evr = pool_id2str(libsolv_solver->pool, solvable->evr);

            pkg_t *pkg = pkg_hash_fetch_by_name_version(pkg_name, evr);
            pkg_t *old = 0;

            Id decision_rule;

            switch (typeId) {
            case SOLVER_TRANSACTION_ERASE:
                opkg_message(NOTICE, "Removing pkg %s - %s\n", pkg_name, evr);
                ret = opkg_remove_pkg(pkg);
                break;
            case SOLVER_TRANSACTION_DOWNGRADE:
            case SOLVER_TRANSACTION_REINSTALL:
            case SOLVER_TRANSACTION_INSTALL:
            case SOLVER_TRANSACTION_MULTIINSTALL:
                 solver_describe_decision(libsolv_solver->solver, stepId,
                                          &decision_rule);

                /* If a package is not explicitly installed by a job,
                mark it as autoinstalled. */
                if (solver_ruleclass(libsolv_solver->solver, decision_rule)
                                    != SOLVER_RULE_JOB)
                    pkg->auto_installed = 1;

                opkg_message(NOTICE, "Installing pkg %s - %s\n", pkg_name, evr);
                ret = opkg_install_pkg(pkg, 0);
                break;
            case SOLVER_TRANSACTION_UPGRADE:
                old = pkg_hash_fetch_installed_by_name(pkg_name);

                /* if an old version was found set the new package's
                   autoinstalled status to that of the old package. */
                if (old) {
                    char *old_version = pkg_version_str_alloc(old);

                    pkg->auto_installed = old->auto_installed;
                    opkg_message(NOTICE, "Upgrading pkg %s from %s to %s\n",
                                 pkg_name, old_version, evr);

                    free(old_version);
                } else {
                    opkg_message(NOTICE, "Upgrading pkg %s to %s\n",
                                 pkg_name, evr);
                }

                ret = opkg_install_pkg(pkg, 1);
                break;
            case SOLVER_TRANSACTION_IGNORE:
            default:
                ret = 0;
                break;
            }
            if (ret)
                err = -1;
        }
    }

    transaction_free(transaction);
    return err;
}

int opkg_solver_libsolv_perform_action(job_action_t action,
                                       int num_pkgs, char **pkg_names)
{
    int i, err;

    libsolv_solver_t *solver = libsolv_solver_new();

    if (num_pkgs == 0) {
        if (action == JOB_UPGRADE || action == JOB_DISTUPGRADE) {
            libsolv_solver_add_job(solver, action, 0);
        } else {
            opkg_msg(ERROR, "No packages specified for install or remove!\n");
            err = -1;
            goto CLEANUP;
        }
    }

    for (i = 0; i < num_pkgs; i++)
        libsolv_solver_add_job(solver, action, pkg_names[i]);

    err = libsolv_solver_solve(solver);
    if (err)
        goto CLEANUP;

    err = libsolv_solver_execute_transaction(solver);

 CLEANUP:
    libsolv_solver_free(solver);
    return err;
}
