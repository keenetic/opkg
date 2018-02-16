/* vi: set expandtab sw=4 sts=4: */
/* opkg.c - the opkg  package management system

   Thomas Wood <thomas@openedhand.com>

   Copyright (C) 2008 OpenMoko Inc

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

#include <stdio.h>
#include <unistd.h>
#include <fnmatch.h>
#include <stdlib.h>

#include "opkg.h"
#include "opkg_conf.h"
#include "opkg_install.h"
#include "opkg_configure.h"
#include "opkg_download.h"
#include "opkg_remove.h"
#include "solvers/internal/opkg_upgrade_internal.h"
#include "opkg_verify.h"
#include "pkg_parse.h"
#include "solvers/internal/pkg_depends_internal.h"
#include "sprintf_alloc.h"
#include "file_util.h"
#include "xfuncs.h"

#define opkg_assert(expr) if (!(expr)) { \
    printf ("opkg: file %s: line %d (%s): Assertation '%s' failed",\
            __FILE__, __LINE__, __PRETTY_FUNCTION__, # expr); abort (); }

/** Private Functions ***/

static void progress(opkg_progress_data_t * d, int p,
                     opkg_progress_callback_t callback, void *user_data)
{
    opkg_assert(d);

    d->percentage = p;

    if (callback)
        callback(d, user_data);
}

static int opkg_configure_packages(char *pkg_name)
{
    pkg_vec_t *all;
    unsigned int i;
    pkg_t *pkg;
    int r, err = 0;

    all = pkg_vec_alloc();
    pkg_hash_fetch_available(all);

    for (i = 0; i < all->len; i++) {
        pkg = all->pkgs[i];

        if (pkg_name && fnmatch(pkg_name, pkg->name, 0))
            continue;

        if (pkg->state_status == SS_UNPACKED) {
            r = opkg_configure(pkg);
            if (r == 0) {
                pkg->state_status = SS_INSTALLED;
                pkg->parent->state_status = SS_INSTALLED;
                pkg->state_flag &= ~SF_PREFER;
            } else {
                if (!err)
                    err = r;
            }
        }
    }

    pkg_vec_free(all);
    return err;
}

struct _curl_cb_data {
    opkg_progress_callback_t cb;
    opkg_progress_data_t *progress_data;
    void *user_data;
    int start_range;
    int finish_range;
};

static int curl_progress_cb(struct _curl_cb_data *cb_data, double t,    /* dltotal */
                            double d,   /* dlnow */
                            double ultotal, double ulnow)
{
    int p = (t) ? d * 100 / t : 0;
    static int prev = -1;
    int progress = 0;

    /* prevent the same value being sent twice (can occur due to rounding) */
    if (p == prev)
        return 0;
    prev = p;

    if (t < 1)
        return 0;

    progress = cb_data->start_range +
        (d / t * ((cb_data->finish_range - cb_data->start_range)));
    cb_data->progress_data->percentage = progress;

    (cb_data->cb) (cb_data->progress_data, cb_data->user_data);

    return 0;
}

static opkg_conf_t saved_conf;
/*** Public API ***/

int opkg_new()
{
    int r;
    saved_conf = *opkg_config;

    r = opkg_conf_init();
    if (r != 0)
        goto err0;

    r = opkg_conf_load();
    if (r != 0)
        goto err0;

    r = pkg_hash_load_feeds();
    if (r != 0)
        goto err1;

    r = pkg_hash_load_status_files();
    if (r != 0)
        goto err1;

    return 0;

 err1:
    pkg_hash_deinit();
 err0:
    opkg_conf_deinit();
    return -1;
}

void opkg_free(void)
{
    opkg_download_cleanup();
    opkg_conf_deinit();
}

int opkg_re_read_config_files(void)
{
    opkg_free();
    *opkg_config = saved_conf;
    return opkg_new();
}

int opkg_get_option(char *option, void *value)
{
    opkg_assert(option != NULL);
    opkg_assert(value != NULL);

    return opkg_conf_get_option(option, value);
}

void opkg_set_option(char *option, void *value)
{
    opkg_assert(option != NULL);
    opkg_assert(value != NULL);

    /* Set option, overwriting any previously set value. */
    opkg_conf_set_option(option, value, 1);
}

/**
 * @brief libopkg API: Install package
 * @param package_name The name of package in which is going to install
 * @param progress_callback The callback function that report the status to caller.
 */
int opkg_install_package(const char *package_url,
                         opkg_progress_callback_t progress_callback,
                         void *user_data)
{
    int err;
    opkg_progress_data_t pdata;
    pkg_t *old, *new;
    pkg_vec_t *deps, *all;
    unsigned int i;
    char **unresolved = NULL;
    char *package_name;

    opkg_assert(package_url != NULL);

    /* Pre-process the package name to handle remote URLs and paths to
     * ipk/opk files.
     */
    opkg_prepare_url_for_install(package_url, &package_name);

    /* ... */
    pkg_info_preinstall_check();

    /* check to ensure package is not already installed */
    old = pkg_hash_fetch_installed_by_name(package_name);
    if (old) {
        opkg_msg(ERROR, "Package %s is already installed\n", package_name);
        return -1;
    }

    new = pkg_hash_fetch_best_installation_candidate_by_name(package_name);
    if (!new) {
        opkg_msg(ERROR, "Couldn't find package %s\n", package_name);
        return -1;
    }

    new->state_flag |= SF_USER;

    pdata.action = -1;
    pdata.pkg = new;

    progress(&pdata, 0, progress_callback, user_data);

    /* find dependancies and download them */
    deps = pkg_vec_alloc();
    /* this function does not return the original package, so we insert it later */
    pkg_hash_fetch_unsatisfied_dependencies(new, deps, &unresolved);
    if (unresolved) {
        char **tmp = unresolved;
        opkg_msg(ERROR,
                 "Couldn't satisfy the following dependencies" " for %s:\n",
                 package_name);
        while (*tmp) {
            opkg_msg(ERROR, "\t%s", *tmp);
            free(*tmp);
            tmp++;
        }
        free(unresolved);
        pkg_vec_free(deps);
        opkg_message(ERROR, "\n");
        return -1;
    }

    /* insert the package we are installing so that we download it */
    pkg_vec_insert(deps, new);

    /* download package and dependencies */
    for (i = 0; i < deps->len; i++) {
        pkg_t *pkg;
        struct _curl_cb_data cb_data;
        char *url;

        pkg = deps->pkgs[i];
        if (pkg->local_filename)
            continue;

        pdata.pkg = pkg;
        pdata.action = OPKG_DOWNLOAD;

        if (pkg->src == NULL) {
            opkg_msg(ERROR,
                     "Package %s not available from any " "configured src\n",
                     package_name);
            return -1;
        }

        sprintf_alloc(&url, "%s/%s", pkg->src->value, pkg->filename);

        cb_data.cb = progress_callback;
        cb_data.progress_data = &pdata;
        cb_data.user_data = user_data;
        /* 75% of "install" progress is for downloading */
        cb_data.start_range = 75 * i / deps->len;
        cb_data.finish_range = 75 * (i + 1) / deps->len;

        pkg->local_filename = opkg_download_cache(url,
                                (curl_progress_func) curl_progress_cb,
                                &cb_data);
        free(url);

        if (pkg->local_filename == NULL) {
            pkg_vec_free(deps);
            return -1;
        }

    }
    pkg_vec_free(deps);

    /* clear depenacy checked marks, left by pkg_hash_fetch_unsatisfied_dependencies */
    all = pkg_vec_alloc();
    pkg_hash_fetch_available(all);
    for (i = 0; i < all->len; i++) {
        all->pkgs[i]->parent->dependencies_checked = 0;
    }
    pkg_vec_free(all);

    /* 75% of "install" progress is for downloading */
    pdata.pkg = new;
    pdata.action = OPKG_INSTALL;
    progress(&pdata, 75, progress_callback, user_data);

    /* unpack the package */
    err = opkg_install_pkg(new);

    if (err) {
        return -1;
    }

    progress(&pdata, 90, progress_callback, user_data);

    /* run configure scripts, etc. */
    err = opkg_configure_packages(NULL);
    if (err) {
        return -1;
    }

    /* write out status files and file lists */
    opkg_conf_write_status_files();
    pkg_write_changed_filelists();

    progress(&pdata, 100, progress_callback, user_data);
    return 0;
}

int opkg_remove_package(const char *package_name,
                        opkg_progress_callback_t progress_callback,
                        void *user_data)
{
    int err;
    pkg_t *pkg = NULL;
    pkg_t *pkg_to_remove;
    opkg_progress_data_t pdata;

    opkg_assert(package_name != NULL);

    pkg_info_preinstall_check();

    pkg = pkg_hash_fetch_installed_by_name(package_name);

    if (pkg == NULL || pkg->state_status == SS_NOT_INSTALLED) {
        opkg_msg(ERROR, "Package %s not installed\n", package_name);
        return -1;
    }

    pdata.action = OPKG_REMOVE;
    pdata.pkg = pkg;
    progress(&pdata, 0, progress_callback, user_data);

    if (opkg_config->restrict_to_default_dest) {
        pkg_to_remove = pkg_hash_fetch_installed_by_name_dest(pkg->name,
                                                              opkg_config->default_dest);
    } else {
        pkg_to_remove = pkg_hash_fetch_installed_by_name(pkg->name);
    }

    progress(&pdata, 75, progress_callback, user_data);

    err = opkg_remove_pkg(pkg_to_remove);

    /* write out status files and file lists */
    opkg_conf_write_status_files();
    pkg_write_changed_filelists();

    progress(&pdata, 100, progress_callback, user_data);
    return (err) ? -1 : 0;
}

int opkg_upgrade_package(const char *package_name,
                         opkg_progress_callback_t progress_callback,
                         void *user_data)
{
    int err;
    pkg_t *pkg;
    opkg_progress_data_t pdata;

    opkg_assert(package_name != NULL);

    pkg_info_preinstall_check();

    if (opkg_config->restrict_to_default_dest) {
        pkg = pkg_hash_fetch_installed_by_name_dest(package_name,
                                                    opkg_config->default_dest);
    } else {
        pkg = pkg_hash_fetch_installed_by_name(package_name);
    }

    if (!pkg) {
        opkg_msg(ERROR, "Package %s not installed\n", package_name);
        return -1;
    }

    pdata.action = OPKG_INSTALL;
    pdata.pkg = pkg;
    progress(&pdata, 0, progress_callback, user_data);

    err = opkg_upgrade_pkg(pkg);
    if (err) {
        return -1;
    }
    progress(&pdata, 75, progress_callback, user_data);

    err = opkg_configure_packages(NULL);
    if (err) {
        return -1;
    }

    /* write out status files and file lists */
    opkg_conf_write_status_files();
    pkg_write_changed_filelists();

    progress(&pdata, 100, progress_callback, user_data);
    return 0;
}

int opkg_upgrade_all(opkg_progress_callback_t progress_callback,
                     void *user_data)
{
    pkg_vec_t *installed;
    int err = 0;
    unsigned int i;
    pkg_t *pkg;
    opkg_progress_data_t pdata;

    pdata.action = OPKG_INSTALL;
    pdata.pkg = NULL;

    progress(&pdata, 0, progress_callback, user_data);

    installed = pkg_vec_alloc();
    pkg_info_preinstall_check();

    pkg_hash_fetch_all_installed(installed, INSTALLED);
    for (i = 0; i < installed->len; i++) {
        pkg = installed->pkgs[i];

        pdata.pkg = pkg;
        progress(&pdata, 99 * i / installed->len, progress_callback, user_data);

        err += opkg_upgrade_pkg(pkg);
    }
    pkg_vec_free(installed);

    if (err)
        return 1;

    err = opkg_configure_packages(NULL);
    if (err)
        return 1;

    /* write out status files and file lists */
    opkg_conf_write_status_files();
    pkg_write_changed_filelists();

    pdata.pkg = NULL;
    progress(&pdata, 100, progress_callback, user_data);
    return 0;
}

int opkg_update_package_lists(opkg_progress_callback_t progress_callback,
                              void *user_data)
{
    char *tmp;
    int err, result = 0;
    pkg_src_list_elt_t *iter;
    pkg_src_t *src;
    int sources_list_count, sources_done;
    opkg_progress_data_t pdata;
    char *dtemp;

    pdata.action = OPKG_DOWNLOAD;
    pdata.pkg = NULL;
    progress(&pdata, 0, progress_callback, user_data);

    if (!file_is_dir(opkg_config->lists_dir)) {
        if (file_exists(opkg_config->lists_dir)) {
            opkg_msg(ERROR, "%s is not a directory\n", opkg_config->lists_dir);
            return 1;
        }

        err = file_mkdir_hier(opkg_config->lists_dir, 0755);
        if (err) {
            opkg_msg(ERROR, "Couldn't create lists_dir %s\n",
                     opkg_config->lists_dir);
            return 1;
        }
    }

    sprintf_alloc(&tmp, "%s/update-XXXXXX", opkg_config->tmp_dir);
    dtemp = mkdtemp(tmp);
    if (dtemp == NULL) {
        opkg_perror(ERROR, "Coundn't create temporary directory %s", tmp);
        free(tmp);
        return 1;
    }

    /* count the number of sources so we can give some progress updates */
    sources_list_count = 0;
    sources_done = 0;
    list_for_each_entry(iter, &opkg_config->pkg_src_list.head, node) {
        sources_list_count++;
    }

    list_for_each_entry(iter, &opkg_config->pkg_src_list.head, node) {
        src = (pkg_src_t *) iter->data;

        err = pkg_src_update(src);
        if (err)
            result = -1;

        sources_done++;
        progress(&pdata, 100 * sources_done / sources_list_count,
                 progress_callback, user_data);
    }

    rmdir(tmp);
    free(tmp);

    /* Now re-read the package lists to update package hash tables. */
    opkg_re_read_config_files();

    return result;
}

static int pkg_compare_names_and_version(const void *a0, const void *b0)
{
    const pkg_t *a = *(const pkg_t **)a0;
    const pkg_t *b = *(const pkg_t **)b0;
    int ret;

    ret = strcmp(a->name, b->name);

    if (ret == 0)
        ret = pkg_compare_versions(a, b);

    return ret;
}

int opkg_list_packages(opkg_package_callback_t callback, void *user_data)
{
    pkg_vec_t *all;
    unsigned int i;

    opkg_assert(callback);

    all = pkg_vec_alloc();
    pkg_hash_fetch_available(all);

    pkg_vec_sort(all, pkg_compare_names_and_version);

    for (i = 0; i < all->len; i++) {
        pkg_t *pkg;

        pkg = all->pkgs[i];

        callback(pkg, user_data);
    }

    pkg_vec_free(all);

    return 0;
}

int opkg_list_upgradable_packages(opkg_package_callback_t callback,
                                  void *user_data)
{
    LIST_HEAD(head);

    pkg_t *old = NULL, *new = NULL;

    opkg_assert(callback);

    /* ensure all data is valid */
    pkg_info_preinstall_check();

    prepare_upgrade_list(&head);
    list_for_each_entry(old, &head, list) {
        new = pkg_hash_fetch_best_installation_candidate_by_name(old->name);
        if (new == NULL)
            continue;
        callback(new, user_data);
    }
    return 0;
}

pkg_t *opkg_find_package(const char *name, const char *ver, const char *arch,
                         const char *repo)
{
    pkg_t *pkg = NULL;
    pkg_vec_t *all;
    unsigned int i;

    /* We expect to be given a name to search for, all other arguments are
     * optional.
     */
    opkg_assert(name);

    all = pkg_vec_alloc();
    pkg_hash_fetch_available(all);
    for (i = 0; i < all->len; i++) {
        char *pkgv;

        pkg = all->pkgs[i];

        /* check name */
        /* We can assume all packages have a name and we always want to
         * check the name.
         */
        if (strcmp(pkg->name, name))
            continue;

        /* check version */
        /* We can assume all packages have a version but we might not
         * have been given a version as an argument if we don't care
         * what it is.
         */
        pkgv = pkg_version_str_alloc(pkg);
        if (ver && strcmp(pkgv, ver)) {
            free(pkgv);
            continue;
        }
        free(pkgv);

        /* check architecture */
        if (arch && pkg->architecture && strcmp(pkg->architecture, arch))
            continue;

        /* check repository */
        if (repo && pkg->src && pkg->src->name && strcmp(pkg->src->name, repo))
            continue;

        /* match found */
        pkg_vec_free(all);
        return pkg;
    }

    /* no match found */
    pkg_vec_free(all);
    return NULL;
}

/**
 * @brief Check the accessibility of repositories.
 * @return return how many repositories cannot access. 0 means all okay.
 */
int opkg_repository_accessibility_check(void)
{
    pkg_src_list_elt_t *iter;
    str_list_elt_t *iter1;
    str_list_t *src;
    int repositories = 0;
    int ret = 0;
    char *repo_ptr;
    char *stmp;
    char *host, *end;

    src = str_list_alloc();

    list_for_each_entry(iter, &opkg_config->pkg_src_list.head, node) {
        host = strstr(((pkg_src_t *) iter->data)->value, "://") + 3;
        end = index(host, '/');
        if (strstr(((pkg_src_t *) iter->data)->value, "://") && end)
            stmp = xstrndup(((pkg_src_t *) iter->data)->value,
                            end - ((pkg_src_t *) iter->data)->value);
        else
            stmp = xstrdup(((pkg_src_t *) iter->data)->value);

        for (iter1 = str_list_first(src); iter1;
                iter1 = str_list_next(src, iter1)) {
            if (strstr(iter1->data, stmp))
                break;
        }
        if (iter1)
            continue;

        sprintf_alloc(&repo_ptr, "%s/index.html", stmp);
        free(stmp);

        str_list_append(src, repo_ptr);
        free(repo_ptr);
        repositories++;
    }

    while (repositories > 0) {
        char *cache_location;
        iter1 = str_list_pop(src);
        repositories--;

        cache_location = opkg_download_cache(iter1->data, NULL, NULL);
        if (cache_location) {
            free(cache_location);
            ret++;
        }
        str_list_elt_deinit(iter1);
    }

    free(src);

    return ret;
}

int opkg_compare_versions(const char *ver1, const char *ver2)
{
    pkg_t *pkg1, *pkg2;

    pkg1 = pkg_new();
    pkg2 = pkg_new();

    parse_version(pkg1, ver1);
    parse_version(pkg2, ver2);

    return pkg_compare_versions(pkg1, pkg2);
}
