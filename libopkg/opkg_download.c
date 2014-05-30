/* vi: set expandtab sw=4 sts=4: */
/* opkg_download.c - the opkg package management system

   Carl D. Worth

   Copyright (C) 2001 University of Southern California
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
#include <libgen.h>
#include <ctype.h>
#include <sys/stat.h>
#include <malloc.h>
#include <stdlib.h>

#include "opkg_download.h"
#include "opkg_message.h"
#include "opkg_verify.h"

#include "sprintf_alloc.h"
#include "xsystem.h"
#include "file_util.h"
#include "opkg_defines.h"
#include "xfuncs.h"

#ifdef HAVE_CURL
#include <curl/curl.h>
#endif

#ifdef HAVE_OPENSSL
#include "opkg_openssl.h"
#endif

static int
str_starts_with(const char *str, const char *prefix)
{
    return (strncmp(str, prefix, strlen(prefix)) == 0);
}

#ifdef HAVE_CURL
/*
 * Make curl an instance variable so we don't have to instanciate it
 * each time
 */
static CURL *curl = NULL;
static CURL *opkg_curl_init(curl_progress_func cb, void *data);

size_t
dummy_write(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    return size * nmemb;
}

/** \brief header_write: curl callback that extracts HTTP ETag header
 *
 * \param ptr complete HTTP header line
 * \param size size of each data element
 * \param nmemb number of data elements
 * \param userdata pointer to data for storing ETag header value
 * \return number of processed bytes
 *
 */
size_t
header_write(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    char prefix[5];
    int i;
    for (i = 0; (i < 5) && (i < size * nmemb); ++i)
        prefix[i] = tolower(ptr[i]);
    if (str_starts_with(prefix, "etag:")) {
        char** out = userdata;
        char* start = strchr(ptr, '"') + 1;
        char* end = strrchr(ptr, '"');
        if (end > start)
            *out = strndup(start, end - start);
    }
    return size * nmemb;
}

#ifdef HAVE_SSLCURL
/*
 * Creates a newly allocated string with the same content as str, but
 * the first occurence of "token" is replaced with "replacement".
 * Returns a pointer to the newly created string's first byte.
 * If the substring "token" is not present in the string "str",
 * or if token is the empty string, it returns a newly allocated string
 * that is identical to "str".
 */
static char *
replace_token_in_str(const char *str, const char *token, const char *replacement)
{
    /*
     * There's nothing to replace, just clone the string to be consistent with
     * the fact that the user gets a newly allocated string back
     */
    if (!token || *token == '\0')
	return xstrdup(str);

    char *found_token = strstr(str, token);
    if (!found_token)
	/*
	 * There's nothing to replace, just clone the string to be consistent
	 * with the fact that the user gets a newly allocated string back
	 */
	return xstrdup(str);

    size_t str_len          = strlen(str);
    size_t replacement_len  = strlen(replacement);

    size_t token_len        = strlen(token);
    unsigned int token_idx  = found_token - str;

    size_t replaced_str_len = str_len - (strlen(token) - strlen(replacement));
    char *replaced_str      = xmalloc(replaced_str_len * sizeof(char));

    /* first copy the string part *before* the substring to replace */
    strncpy(replaced_str, str, token_idx);
    /* then copy the replacement at the same position than the token to replace
     */
    strncpy(replaced_str + token_idx, replacement, strlen(replacement));
    /* finally complete the string with characters following the token to
     * replace
     */
    strncpy(replaced_str + token_idx + replacement_len,
	    str + token_idx + token_len,
	    strlen(str + token_idx + token_len));

    replaced_str[replaced_str_len] = '\0';

    return replaced_str;
}

#if defined(HAVE_PATHFINDER) && defined(HAVE_OPENSSL)
static CURLcode
curl_ssl_ctx_function(CURL * curl, void * sslctx, void * parm)
{
  SSL_CTX * ctx = (SSL_CTX *) sslctx;
  SSL_CTX_set_cert_verify_callback(ctx, pathfinder_verify_callback, parm);

  return CURLE_OK ;
}
#endif /* HAVE_PATHFINDER && HAVE_OPENSSL */
#endif /* HAVE_SSLCURL */

/** \brief create_file_stamp: creates stamp for file
 *
 * \param file_name absolute file name
 * \param stamp stamp data for file
 * \return 0 if success, -1 if error occurs
 *
 */
int
create_file_stamp(const char *file_name, char *stamp)
{
    FILE * file;
    char *file_path;

    sprintf_alloc(&file_path, "%s.@stamp", file_name);
    file = fopen(file_path, "wb");
    if (file == NULL) {
        opkg_msg(ERROR, "Failed to open file %s\n", file_path);
        free(file_path);
        return -1;
    }
    fwrite(stamp, strlen(stamp), 1, file);
    fclose(file);
    free(file_path);
    return 0;
}

#define STAMP_BUF_SIZE 10
/** \brief check_file_stamp: compares provided stamp with existing file stamp
 *
 * \param file_name absolute file name
 * \param stamp stamp data to compare with existing file stamp
 * \return 0 if both stamps are equal or -1 otherwise
 *
 */
int
check_file_stamp(const char *file_name, char *stamp)
{
    FILE * file;
    char stamp_buf[STAMP_BUF_SIZE];
    char *file_path;
    int size;
    int diff = 0;

    sprintf_alloc(&file_path, "%s.@stamp", file_name);
    if (!file_exists(file_path)) {
        free(file_path);
        return -1;
    }
    file = fopen(file_path, "rb");
    if (file == NULL) {
        opkg_msg(ERROR, "Failed to open file %s\n", file_path);
        free(file_path);
        return -1;
    }
    while ((size = fread(stamp_buf, 1, STAMP_BUF_SIZE, file)) && *stamp) {
        if (((size < STAMP_BUF_SIZE) &&
             (size != strlen(stamp))) ||
            ((size == STAMP_BUF_SIZE) &&
             (strlen(stamp) < STAMP_BUF_SIZE)) ||
            memcmp(stamp_buf, stamp, size)) {
                diff = 1;
                break;
        }
        stamp += STAMP_BUF_SIZE;
    }
    fclose(file);
    free(file_path);
    return diff;
}

/** \brief opkg_validate_cached_file: check if file exists in cache
 *
 * \param src absolute URI of remote file
 * \param cache_location absolute name of cached file
 * \return 0 if file exists in cache and is completely downloaded.
 *         1 if file needs further downloading.
 *         -1 if error occurs.
 */
int
opkg_validate_cached_file(const char *src,
		const char *cache_location)
{
    CURLcode res;
    FILE * file;
    long resume_from = 0;
    char* etag = NULL;
    double src_size = -1;

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &dummy_write);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, &header_write);
    curl_easy_setopt(curl, CURLOPT_WRITEHEADER, &etag);
    curl_easy_setopt(curl, CURLOPT_HEADER, 1);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1); // remove body

    res = curl_easy_perform(curl);
    if (res) {
	long error_code;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &error_code);
	opkg_msg(ERROR, "Failed to download %s headers: %s.\n",
	    src, curl_easy_strerror(res));
	return -1;
    }
    curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &src_size);

    if (!file_exists(cache_location) ||
	!etag ||
	check_file_stamp(cache_location, etag)) {
	    unlink(cache_location);
	    if (etag && create_file_stamp(cache_location, etag))
		opkg_msg(ERROR, "Failed to create stamp for %s.\n",
		    cache_location);
    }
    free(etag);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, NULL);
    curl_easy_setopt(curl, CURLOPT_WRITEHEADER, NULL);
    curl_easy_setopt(curl, CURLOPT_HEADER, 0);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 0);

    file = fopen(cache_location, "ab");
    if (!file) {
        opkg_msg(ERROR, "Failed to open cache file %s\n",
            cache_location);
        return -1;
    }
    fseek(file, 0, SEEK_END);
    resume_from = ftell(file);
    fclose (file);

    if (resume_from < src_size)
	curl_easy_setopt(curl, CURLOPT_RESUME_FROM, resume_from);
    else
        return 0;

    return 1;
}

/* Download using curl backend. */
static int
opkg_download_backend(const char *src, const char *dest,
	curl_progress_func cb, void *data, int use_cache)
{
    CURLcode res;
    FILE * file;
    int ret;

    curl = opkg_curl_init (cb, data);
    if (!curl)
        return -1;

    curl_easy_setopt(curl, CURLOPT_URL, src);

#ifdef HAVE_SSLCURL
    if (opkg_config->ftp_explicit_ssl)
    {
        /*
         * This is what enables explicit FTP SSL mode on curl's side As per
         * the official documentation at
         * http://curl.haxx.se/libcurl/c/curl_easy_setopt.html : "This
         * option was known as CURLOPT_FTP_SSL up to 7.16.4, and the
         * constants were known as CURLFTPSSL_*"
         */
        curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);

        /*
	 * If a URL with the ftps:// scheme is passed to curl, then it
         * considers it's implicit mode. Thus, we need to fix it before
         * invoking curl.
         */
	char *fixed_src = replace_token_in_str(src, "ftps://", "ftp://");
        curl_easy_setopt(curl, CURLOPT_URL, fixed_src);
        free(fixed_src);
    }
#endif /* HAVE_SSLCURL */

    if (use_cache) {
        ret = opkg_validate_cached_file(src, dest);
        if (ret <= 0)
            return ret;
    }
    else {
        unlink(dest);
    }

    file = fopen(dest, "ab");
    if (!file) {
        opkg_msg(ERROR, "Failed to open destination file %s\n",
            dest);
        return -1;
    }

    res = 0;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
    res = curl_easy_perform (curl);
    fclose (file);
    if (res) {
        long error_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &error_code);
        opkg_msg(ERROR, "Failed to download %s: %s.\n",
            src, curl_easy_strerror(res));
        return -1;
    }

    return 0;
}

void
opkg_download_cleanup(void)
{
    if(curl != NULL){
	curl_easy_cleanup (curl);
	curl = NULL;
    }
}
#else
/* Download using wget backend. */
static int
opkg_download_backend(const char *src, const char *dest,
	curl_progress_func cb, void *data, int use_cache)
{
    int res;
    const char *argv[8];
    int i = 0;

    /* Unused arguments. */
    (void) cb;
    (void) data;
    (void) use_cache;

    unlink(dest);

    argv[i++] = "wget";
    argv[i++] = "-q";
    if (opkg_config->http_proxy || opkg_config->ftp_proxy) {
	argv[i++] = "-Y";
	argv[i++] = "on";
    }
    argv[i++] = "-O";
    argv[i++] = dest;
    argv[i++] = src;
    argv[i++] = NULL;
    res = xsystem(argv);

    if (res) {
	opkg_msg(ERROR, "Failed to download %s, wget returned %d.\n", src, res);
	return -1;
    }

    return 0;
}

void
opkg_download_cleanup(void)
{
    /* Nothing to do. */
}
#endif

static int
opkg_download_set_env()
{
    int r;

    if (opkg_config->http_proxy) {
        opkg_msg(DEBUG, "Setting environment variable: http_proxy = %s.\n",
                opkg_config->http_proxy);
        r = setenv("http_proxy", opkg_config->http_proxy, 1);
        if (r != 0) {
            opkg_msg(ERROR, "Failed to set environment variable http_proxy");
            return r;
        }
    }
    if (opkg_config->https_proxy) {
        opkg_msg(DEBUG, "Setting environment variable: https_proxy = %s.\n",
                opkg_config->https_proxy);
        r = setenv("https_proxy", opkg_config->https_proxy, 1);
        if (r != 0) {
            opkg_msg(ERROR, "Failed to set environment variable https_proxy");
            return r;
        }
    }
    if (opkg_config->ftp_proxy) {
        opkg_msg(DEBUG, "Setting environment variable: ftp_proxy = %s.\n",
                opkg_config->ftp_proxy);
        r = setenv("ftp_proxy", opkg_config->ftp_proxy, 1);
        if (r != 0) {
            opkg_msg(ERROR, "Failed to set environment variable ftp_proxy");
            return r;
        }
    }
    if (opkg_config->no_proxy) {
        opkg_msg(DEBUG,"Setting environment variable: no_proxy = %s.\n",
                opkg_config->no_proxy);
        r = setenv("no_proxy", opkg_config->no_proxy, 1);
        if (r != 0) {
            opkg_msg(ERROR, "Failed to set environment variable no_proxy");
            return r;
        }
    }

    return 0;
}

static int
opkg_download_file(const char *src, const char *dest)
{
    if (!file_exists(src)) {
	opkg_msg(ERROR, "%s: No such file.\n", src);
	return -1;
    }

    /* Currently there is no attempt to check whether the destination file
     * already matches the source file. If doing so is worthwhile, it can be
     * added.
     */

    return file_copy(src, dest);
}

/** \brief opkg_download_internal: downloads file with existence check
 *
 * \param src absolute URI of file to download
 * \param dest destination path for downloaded file
 * \param cb callback for curl download progress
 * \param data data to pass to progress callback
 * \param use_cache 1 if file is downloaded into cache or 0 otherwise
 * \return 0 if success, -1 if error occurs
 *
 */
int
opkg_download_internal(const char *src, const char *dest,
	curl_progress_func cb, void *data, int use_cache)
{
    int ret;

    opkg_msg(NOTICE,"Downloading %s.\n", src);

    if (str_starts_with(src, "file:")) {
        const char *file_src = src + 5;

	return opkg_download_file(file_src, dest);
    }

    ret = opkg_download_set_env();
    if (ret != 0) {
	/* Error message already printed. */
	return ret;
    }

    return opkg_download_backend(src, dest, cb, data, use_cache);
}

/** \brief get_cache_location: generate cached file path
 *
 * \param src absolute URI of remote file to generate path for
 * \return generated file path
 *
 */
char *
get_cache_location(const char *src)
{
    char *cache_name = xstrdup(src);
    char *cache_location, *p;

    for (p = cache_name; *p; p++)
        if (*p == '/')
            *p = '_';

    sprintf_alloc(&cache_location, "%s/%s", opkg_config->cache_dir, cache_name);
    free(cache_name);
    return cache_location;
}

/** \brief opkg_download_cache: downloads file into cache
 *
 * \param src absolute URI of file to download
 * \param cb callback for curl download progress
 * \param data data to pass to progress callback
 * \return path of downloaded file in cache or NULL if error occurs
 *
 */
char *
opkg_download_cache(const char *src,
	curl_progress_func cb, void *data)
{
    char *cache_location;
    int err;

    cache_location = get_cache_location(src);
    err = opkg_download_internal(src, cache_location, cb, data, 1);
    if (err) {
        free(cache_location);
        cache_location = NULL;
    }
    return cache_location;
}

/** \brief opkg_download_direct: downloads file directly
 *
 * \param src absolute URI of file to download
 * \param dest destination path for downloaded file
 * \param cb callback for curl download progress
 * \param data data to pass to progress callback
 * \return 0 on success, <0 on failure
 *
 */
int
opkg_download_direct(const char *src, const char *dest,
	curl_progress_func cb, void *data)
{
    return opkg_download_internal(src, dest, cb, data, 0);
}

int
opkg_download(const char *src, const char *dest_file_name,
	curl_progress_func cb, void *data)
{
    int err = -1;

    if (!opkg_config->volatile_cache) {
        char *cache_location = opkg_download_cache(src, cb, data);
        if (cache_location) {
            err = file_copy(cache_location, dest_file_name);
            free(cache_location);
        }
    }
    else {
        err = opkg_download_direct(src, dest_file_name, NULL, NULL);
    }
    return err;
}

/** \brief opkg_download_pkg: download and verify a package
 *
 * \param pkg the package to download
 * \param dir destination directory or NULL to store package in cache
 * \return 0 if success, -1 if error occurs
 *
 */
int
opkg_download_pkg(pkg_t *pkg, const char *dir)
{
    char *url;
    int err = 0;

    if (pkg->src == NULL) {
	opkg_msg(ERROR, "Package %s is not available from any configured src.\n",
		pkg->name);
	return -1;
    }
    if (pkg->filename == NULL) {
	opkg_msg(ERROR, "Package %s does not have a valid filename field.\n",
		pkg->name);
	return -1;
    }

    sprintf_alloc(&url, "%s/%s", pkg->src->value, pkg->filename);
    if (!dir || !opkg_config->volatile_cache) {
        pkg->local_filename = get_cache_location(url);

        /* Check if valid package exists in cache */
        err = pkg_verify(pkg);
        if (!err)
            return 0;

        pkg->local_filename = opkg_download_cache(url, NULL, NULL);
        free(url);
        if (pkg->local_filename == NULL)
            return -1;

        /* Ensure downloaded package is valid. */
        err = pkg_verify(pkg);
        if (err)
            return err;
    }
    if (dir)
    {
        char* dest_file_name;
        sprintf_alloc(&dest_file_name, "%s/%s", dir, pkg->filename);
        if (opkg_config->volatile_cache) {
            err = opkg_download_direct(url, dest_file_name, NULL, NULL);
            free(url);
            if (err) {
                free(dest_file_name);
                return err;
            }

            /* This is a bit hackish
             * TODO: Clean this up! */
            pkg->local_filename = dest_file_name;
            err = pkg_verify(pkg);
            pkg->local_filename = NULL;
        }
        else {
            err = file_copy(pkg->local_filename, dest_file_name);
        }
        free(dest_file_name);
    }

    return err;
}

/*
 * Returns 1 if URL "url" is prefixed by a remote protocol, 0 otherwise.
 */
static int
url_has_remote_protocol(const char* url)
{
  static const char* remote_protos[] = {
    "http://",
    "ftp://",
    "https://",
    "ftps://"
  };
  static const size_t nb_remote_protos =  sizeof(remote_protos) /
                                          sizeof(char*);

  unsigned int i = 0;
  for (i = 0; i < nb_remote_protos; ++i)
  {
    if (str_starts_with(url, remote_protos[i]))
      return 1;
  }

  return 0;
}

static int
opkg_prepare_file_for_install(const char * path, char **namep)
{
    int r;
    pkg_t * pkg = pkg_new();

    r = pkg_init_from_file(pkg, path);
    if (r)
        return r;

    opkg_msg(DEBUG2, "Package %s provided by file '%s'.\n",
            pkg->name, pkg->local_filename);
    pkg->provided_by_hand = 1;

    pkg->dest = opkg_config->default_dest;
    pkg->state_want = SW_INSTALL;
    pkg->state_flag |= SF_PREFER;

    if (opkg_config->force_reinstall)
        pkg->force_reinstall = 1;
    else {
        /* Disallow a reinstall of the same package version if force_reinstall
         * is not set. This is because in this case orphaned files may be left
         * behind.
         */
        pkg_t * old_pkg = pkg_hash_fetch_installed_by_name(pkg->name);
        if (old_pkg && (pkg_compare_versions(pkg, old_pkg) == 0)) {
            char * version = pkg_version_str_alloc(old_pkg);
            opkg_msg(ERROR, "Refusing to load file '%s' as it matches the installed version of %s (%s).\n",
                    path, old_pkg->name, version);
            free(version);
            pkg_deinit(pkg);
            free(pkg);
            return -1;
        }
    }

    hash_insert_pkg(pkg, 1);

    if (namep)
        *namep = pkg->name;
    return 0;
}

/* Prepare a given URL for installation. We use a few simple heuristics to
 * determine whether this is a remote URL, file name or abstract package name.
 */
int
opkg_prepare_url_for_install(const char *url, char **namep)
{
    int r;

    /* First heuristic: Maybe it's a remote URL. */
    if (url_has_remote_protocol(url)) {
        char *cache_location;

        cache_location = opkg_download_cache(url, NULL, NULL);
        if (!cache_location)
            return -1;

        r = opkg_prepare_file_for_install(cache_location, namep);
        free(cache_location);
        return r;
    }

    /* Second heuristic: Maybe it's a package name.
     *
     * We check this before files incase an existing file incidentally shares a
     * name with an available package.
     */
    if (abstract_pkg_fetch_by_name(url)) {
        if (opkg_config->force_reinstall) {
            /* Reload the given package from its package file into a new package
             * object. This new object can then be marked as force_reinstall and
             * the reinstall should go ahead like an upgrade.
             */
            pkg_t * pkg;
            pkg = pkg_hash_fetch_best_installation_candidate_by_name(url);
            if (!pkg) {
                opkg_msg(ERROR, "Unknown package %s, cannot force reinstall.\n", url);
                return -1;
            }
            r = opkg_download_pkg(pkg, NULL);
            if (r)
                return r;

            return opkg_prepare_file_for_install(pkg->local_filename, namep);
        }

        /* Nothing special to do. */
        return 0;
    }

    /* Third heuristic: Maybe it's a file. */
    if (file_exists(url))
        return opkg_prepare_file_for_install(url, namep);

    /* Can't find anything matching the requested URL. */
    opkg_msg(ERROR, "Couldn't find anything to satisfy '%s'.\n", url);
    return -1;
}

#ifdef HAVE_CURL
static CURL *
opkg_curl_init(curl_progress_func cb, void *data)
{

    if(curl == NULL){
	curl = curl_easy_init();

#ifdef HAVE_SSLCURL
#ifdef HAVE_OPENSSL
	openssl_init();
#endif /* HAVE_OPENSSL */

	if (opkg_config->ssl_engine) {

	    /* use crypto engine */
	    if (curl_easy_setopt(curl, CURLOPT_SSLENGINE, opkg_config->ssl_engine) != CURLE_OK){
		opkg_msg(ERROR, "Can't set crypto engine '%s'.\n",
			opkg_config->ssl_engine);

		opkg_download_cleanup();
		return NULL;
	    }
	    /* set the crypto engine as default */
	    if (curl_easy_setopt(curl, CURLOPT_SSLENGINE_DEFAULT, 1L) != CURLE_OK){
		opkg_msg(ERROR, "Can't set crypto engine '%s' as default.\n",
			opkg_config->ssl_engine);

		opkg_download_cleanup();
		return NULL;
	    }
	}

	/* cert & key can only be in PEM case in the same file */
	if(opkg_config->ssl_key_passwd){
	    if (curl_easy_setopt(curl, CURLOPT_SSLKEYPASSWD, opkg_config->ssl_key_passwd) != CURLE_OK)
	    {
	        opkg_msg(DEBUG, "Failed to set key password.\n");
	    }
	}

	/* sets the client certificate and its type */
	if(opkg_config->ssl_cert_type){
	    if (curl_easy_setopt(curl, CURLOPT_SSLCERTTYPE, opkg_config->ssl_cert_type) != CURLE_OK)
	    {
	        opkg_msg(DEBUG, "Failed to set certificate format.\n");
	    }
	}
	/* SSL cert name isn't mandatory */
	if(opkg_config->ssl_cert){
	        curl_easy_setopt(curl, CURLOPT_SSLCERT, opkg_config->ssl_cert);
	}

	/* sets the client key and its type */
	if(opkg_config->ssl_key_type){
	    if (curl_easy_setopt(curl, CURLOPT_SSLKEYTYPE, opkg_config->ssl_key_type) != CURLE_OK)
	    {
	        opkg_msg(DEBUG, "Failed to set key format.\n");
	    }
	}
	if(opkg_config->ssl_key){
	    if (curl_easy_setopt(curl, CURLOPT_SSLKEY, opkg_config->ssl_key) != CURLE_OK)
	    {
	        opkg_msg(DEBUG, "Failed to set key.\n");
	    }
	}

	/* Should we verify the peer certificate ? */
	if(opkg_config->ssl_dont_verify_peer){
	    /*
	     * CURLOPT_SSL_VERIFYPEER default is nonzero (curl => 7.10)
	     */
	    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
	}else{
#if defined(HAVE_PATHFINDER) && defined(HAVE_OPENSSL)
	    if(opkg_config->check_x509_path){
    	        if (curl_easy_setopt(curl, CURLOPT_SSL_CTX_FUNCTION, curl_ssl_ctx_function) != CURLE_OK){
		    opkg_msg(DEBUG, "Failed to set ssl path verification callback.\n");
		}else{
		    curl_easy_setopt(curl, CURLOPT_SSL_CTX_DATA, NULL);
		}
	    }
#endif
	}

	/* certification authority file and/or path */
	if(opkg_config->ssl_ca_file){
	    curl_easy_setopt(curl, CURLOPT_CAINFO, opkg_config->ssl_ca_file);
	}
	if(opkg_config->ssl_ca_path){
	    curl_easy_setopt(curl, CURLOPT_CAPATH, opkg_config->ssl_ca_path);
	}
#endif

	if (opkg_config->connect_timeout_ms > 0) {
	    long timeout_ms = opkg_config->connect_timeout_ms;
	    curl_easy_setopt (curl, CURLOPT_CONNECTTIMEOUT_MS, timeout_ms);
	}

	if (opkg_config->transfer_timeout_ms > 0) {
	    long timeout_ms = opkg_config->transfer_timeout_ms;
	    curl_easy_setopt (curl, CURLOPT_TIMEOUT_MS, timeout_ms);
	}

	if (opkg_config->follow_location)
	    curl_easy_setopt (curl, CURLOPT_FOLLOWLOCATION, 1);

	curl_easy_setopt (curl, CURLOPT_FAILONERROR, 1);
	if (opkg_config->http_proxy || opkg_config->ftp_proxy || opkg_config->https_proxy)
	{
	    curl_easy_setopt(curl, CURLOPT_PROXYUSERNAME, opkg_config->proxy_user);
	    curl_easy_setopt(curl, CURLOPT_PROXYPASSWORD, opkg_config->proxy_passwd);
	    curl_easy_setopt(curl, CURLOPT_PROXYAUTH, CURLAUTH_ANY);
	}
	if (opkg_config->http_auth)
	{
	    curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
	    curl_easy_setopt(curl, CURLOPT_USERPWD, opkg_config->http_auth);
	}
    }

    curl_easy_setopt (curl, CURLOPT_NOPROGRESS, (cb == NULL));
    if (cb)
    {
	curl_easy_setopt (curl, CURLOPT_PROGRESSDATA, data);
	curl_easy_setopt (curl, CURLOPT_PROGRESSFUNCTION, cb);
    }

    return curl;

}
#endif
