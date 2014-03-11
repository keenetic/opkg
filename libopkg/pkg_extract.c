/* pkg_extract.c - the opkg package management system

   Carl D. Worth

   Copyright (C) 2001 University of Southern California

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
*/

#include <archive.h>
#include <archive_entry.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include "opkg_message.h"
#include "file_util.h"
#include "pkg_extract.h"
#include "sprintf_alloc.h"
#include "xfuncs.h"

struct inner_data {
	/* Pointer to the original archive file we're extracting from. */
	struct archive *	outer;

	/* Buffer for extracted data. */
	void *			buffer;
};

static ssize_t
inner_read(struct archive *a, void *client_data, const void **buff)
{
	struct inner_data * data = (struct inner_data *)client_data;

	*buff = data->buffer;
	return archive_read_data(data->outer, data->buffer, EXTRACT_BUFFER_LEN);
}

static int
inner_close(struct archive *inner, void *client_data)
{
	struct inner_data * data = (struct inner_data *)client_data;

	archive_read_free(data->outer);
	free(data->buffer);
	free(data);

	return ARCHIVE_OK;
}

static int
copy_to_stream(struct archive * a, FILE * stream)
{
	void * buffer;
	size_t sz_in, sz_out;

	buffer = xmalloc(EXTRACT_BUFFER_LEN);

	while (1) {
		/* Read data into buffer. */
		sz_in = archive_read_data(a, buffer, EXTRACT_BUFFER_LEN);
		if (sz_in == ARCHIVE_FATAL || sz_in == ARCHIVE_WARN) {
			opkg_msg(ERROR, "Failed to read data from archive: %s\n", archive_error_string(a));
			free(buffer);
			return -1;
		}
		else if (sz_in == ARCHIVE_RETRY)
			continue;
		else if (sz_in == 0) {
			/* We've reached the end of the file. */
			free(buffer);
			return 0;
		}

		/* Now write data to the output stream. */
		sz_out = fwrite(buffer, 1, sz_in, stream);
		if (sz_out < sz_in) {
			/* An error occurred during writing. */
			opkg_msg(ERROR, "Failed to write data to stream: %s\n", strerror(errno));
			free(buffer);
			return -1;
		}
	}
}

/* Join left and right path components without intervening '/' as left may end
 * with a prefix to be applied to the names of extracted files.
 *
 * The right path component is stripped of leading '/' or './' components. If
 * the right path component is simply '.' after stripping, it should not be
 * created.
 *
 * Returns the joined path, or NULL if the resulting path should not be created.
 */
static char * join_paths(const char * left, const char * right)
{
	char * path;

	/* Skip leading '/' or './' in right-hand path if present. */
	while (right[0] == '.' && right[1] == '/')
		right += 2;
	while (right[0] == '/')
		right++;

	/* Don't create '.' directory. */
	if (right[0] == '.' && right[1] == '\0')
		return NULL;

	/* Don't extract empty paths. */
	if (right[0] == '\0')
		return NULL;

	if  (!left)
		return xstrdup(right);

	sprintf_alloc(&path, "%s%s", left, right);
	return path;
}

/* Transform the destination path of the given entry.
 *
 * Returns 0 on success, 1 where the file does not need to be extracted and <0
 * on error.
 */
static int transform_dest_path(struct archive_entry * entry, const char * dest)
{
	char * path;
	const char * filename;

	filename = archive_entry_pathname(entry);

	path = join_paths(dest, filename);
	if (!path)
		return 1;

	archive_entry_set_pathname(entry, path);
	free(path);

	return 0;
}

/* Transform all path components of the given entry. This includes hardlink and
 * symlink paths as well as the destination path.
 *
 * Returns 0 on success, 1 where the file does not need to be extracted and <0
 * on error.
 */
static int
transform_all_paths(struct archive_entry * entry, const char * dest)
{
	char * path;
	const char * filename;
	int r;

	r = transform_dest_path(entry, dest);
	if (r)
		return r;

	/* Next transform hardlink and symlink destinations. */
	filename = archive_entry_hardlink(entry);
	if (filename) {
		/* Apply the same transform to the hardlink path as was applied
		 * to the destination path.
		 */
		path = join_paths(dest, filename);
		if (!path) {
			opkg_msg(ERROR, "Not extracting '%s': Hardlink to nowhere.\n", archive_entry_pathname(entry));
			return 1;
		}

		archive_entry_set_hardlink(entry, path);
		free(path);
	}

	/* Currently no transform to perform for symlinks. */

	return 0;
}

/* Read a header from the given archive object and handle all possible outcomes.
 * The returned pointer is managed by libarchive and should not be freed by the
 * caller.
 *
 * If the caller needs to know whether EOF was hit without an error, they can
 * pass an eof pointer which will be set to 1 in this case or 0 otherwise.
 */
static struct archive_entry *
read_header(struct archive * ar, int * eof)
{
	struct archive_entry * entry;
	int r;
	int retries = 0;

	if (eof)
		*eof = 0;

retry:
	r = archive_read_next_header(ar, &entry);
	switch (r) {
		case ARCHIVE_OK:
			break;

		case ARCHIVE_WARN:
			opkg_msg(NOTICE, "Warning when reading ar archive header: %s\n",
					archive_error_string(ar));
			break;

		case ARCHIVE_EOF:
			if (eof)
				*eof = 1;
			return NULL;

		case ARCHIVE_RETRY:
			opkg_msg(ERROR, "Failed to read archive header: %s\n",
					archive_error_string(ar));
			if (retries++ < 3)
				goto retry;
			else
				return NULL;

		default:
			opkg_msg(ERROR, "Failed to read archive header: %s\n",
					archive_error_string(ar));
			return NULL;
	}

	return entry;
}

/* Extact a single file from an open archive, writing data to an open stream.
 * Returns 0 on success or <0 on error.
 */
static int
extract_file_to_stream(struct archive * a, const char * name, FILE * stream)
{
	struct archive_entry * entry;
	const char * path;

	while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
		/* Cleanup the path of the entry incase it starts with './' or
		 * other prefixes.
		 *
		 * We ignore the return value of transform_dest_path as bad
		 * paths simply won't match the requested path name.
		 */
		transform_dest_path(entry, NULL);

		path = archive_entry_pathname(entry);
		if (strcmp(path, name) == 0)
			/* We found the requested file. */
			return copy_to_stream(a, stream);
		else
			archive_read_data_skip(a);
	}

	/* If we get here, we didn't find the listed file. */
	opkg_msg(ERROR, "Could not find the file '%s' in archive.\n", name);
	return -1;
}

/* Extact the paths of files contained in an open archive, writing data to an
 * open stream.  Returns 0 on success or <0 on error.
 */
static int
extract_paths_to_stream(struct archive * a, FILE * stream)
{
	struct archive_entry * entry;
	int r;
	const char * path;

	while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
		path = archive_entry_pathname(entry);
		r = fprintf(stream, "%s\n", path);
		if (r <= 0)
			/* Read failed */
			return -1;

		archive_read_data_skip(a);
	}

	return 0;
}

/* Extract all files in an archive to the filesystem under the path given by
 * dest. Returns 0 on success or <0 on error.
 */
static int
extract_all(struct archive * a, const char * dest, int flags)
{
	struct archive * disk;
	struct archive_entry * entry;
	int r;
	const char * path;
	const char * hardlink;
	const char * symlink;
	int retries;
	int eof;

	disk = archive_write_disk_new();
	if (!disk) {
		opkg_msg(ERROR, "Failed to create disk archive object.\n");
		return -1;
	}

	r = archive_write_disk_set_options(disk, flags);
	if (r == ARCHIVE_WARN)
		opkg_msg(NOTICE, "Warning when setting disk options: %s\n",
				archive_error_string(disk));
	else if (r != ARCHIVE_OK) {
		opkg_msg(ERROR, "Failed to set disk options: %s\n",
				archive_error_string(disk));
		goto err_cleanup;
	}

	r = archive_write_disk_set_standard_lookup(disk);
	if (r == ARCHIVE_WARN)
		opkg_msg(NOTICE, "Warning when setting user/group lookup functions: %s\n",
				archive_error_string(disk));
	else if (r != ARCHIVE_OK) {
		opkg_msg(ERROR, "Failed to set user/group lookup functions: %s\n",
				archive_error_string(disk));
		goto err_cleanup;
	}

	while (1) {
		entry = read_header(a, &eof);
		if (eof)
			break;
		if (!entry) {
			r = -1;
			goto err_cleanup;
		}

		r = transform_all_paths(entry, dest);
		if (r == 1)
			continue;
		if (r < 0) {
			opkg_msg(ERROR, "Failed to transform path.\n");
			goto err_cleanup;
		}

		/* Print destination paths at DEBUG level. */
		path = archive_entry_pathname(entry);
		hardlink = archive_entry_hardlink(entry);
		symlink = archive_entry_symlink(entry);
		if (hardlink)
			opkg_msg(DEBUG, "Extracting '%s', hardlink to '%s'.\n", path, hardlink);
		else if (symlink)
			opkg_msg(DEBUG, "Extracting '%s', symlink to '%s'.\n", path, symlink);
		else
			opkg_msg(DEBUG, "Extracting '%s'.\n", path);

		retries = 0;
retry:
		r = archive_read_extract2(a, entry, disk);
		switch (r) {
			case ARCHIVE_OK:
				break;

			case ARCHIVE_WARN:
				opkg_msg(NOTICE, "Warning when extracting archive entry: %s\n",
						archive_error_string(a));
				break;

			case ARCHIVE_RETRY:
				opkg_msg(ERROR, "Failed to extract archive entry '%s': %s\n",
						archive_entry_pathname(entry),
						archive_error_string(a));
				if (retries++ < 3) {
					opkg_msg(NOTICE, "Retrying...\n");
					goto retry;
				}
				else
					return -1;

			default:
				opkg_msg(ERROR, "Failed to extract archive entry '%s': %s\n",
						archive_entry_pathname(entry),
						archive_error_string(a));
				return -1;
		}
	}

	r = ARCHIVE_OK;
err_cleanup:
	archive_write_free(disk);
	return (r == ARCHIVE_OK) ? 0 : -1;
}

/* Open an outer archive with the given filename. */
static struct archive *
open_outer(const char * filename)
{
	struct archive * outer;
	int r;

	outer = archive_read_new();
	if (!outer) {
		opkg_msg(ERROR, "Failed to create outer archive object.\n");
		return NULL;
	}

	/* Outer package is in 'ar' format, uncompressed. */
	r = archive_read_support_format_ar(outer);
	if (r != ARCHIVE_OK) {
		opkg_msg(ERROR, "Ar format not supported: %s\n",
				archive_error_string(outer));
		goto err_cleanup;
	}

	r = archive_read_open_filename(outer, filename, EXTRACT_BUFFER_LEN);
	if (r != ARCHIVE_OK) {
		opkg_msg(ERROR, "Failed to open package '%s': %s\n",
				filename, archive_error_string(outer));
		goto err_cleanup;
	}

	return outer;

err_cleanup:
	archive_read_free(outer);
	return NULL;
}

/* Open an inner archive at the current position within the given outer archive. */
static struct archive *
open_inner(struct archive * outer)
{
	struct archive * inner;
	struct inner_data * data;
	int r;

	inner = archive_read_new();
	if (!inner) {
		opkg_msg(ERROR, "Failed to create inner archive object.\n");
		return NULL;
	}

	data = (struct inner_data *)xmalloc(sizeof(struct inner_data));
	data->buffer = xmalloc(EXTRACT_BUFFER_LEN);
	data->outer = outer;

	/* Inner package is in 'tar' format, gzip compressed. */
	r = archive_read_support_filter_gzip(inner);
	if (r == ARCHIVE_WARN) {
		/* libarchive returns ARCHIVE_WARN if the filter is provided by
		 * an external program.
		 */
		opkg_msg(INFO, "Gzip support provided by external program.\n");
	} else if (r != ARCHIVE_OK) {
		opkg_msg(ERROR, "Gzip format not supported.\n");
		goto err_cleanup;
	}

	r = archive_read_support_format_tar(inner);
	if (r != ARCHIVE_OK) {
		opkg_msg(ERROR, "Tar format not supported: %s\n",
				archive_error_string(outer));
		goto err_cleanup;
	}

	r = archive_read_open(inner, data, NULL, inner_read, inner_close);
	if (r != ARCHIVE_OK) {
		opkg_msg(ERROR, "Failed to open inner archive: %s\n",
				archive_error_string(inner));
		goto err_cleanup;
	}

	return inner;

err_cleanup:
	archive_read_free(inner);
	free(data->buffer);
	free(data);
	return NULL;
}

/* Locate an inner archive with the given name in the given outer archive.
 * Returns 0 if the item was found, <0 otherwise.
 */
static int
find_inner(struct archive * outer, const char * arname)
{
	const char * path;
	struct archive_entry * entry;

	while (1) {
		entry = read_header(outer, NULL);
		if (!entry)
			return -1;

		path = archive_entry_pathname(entry);
		if (strcmp(path, arname) == 0) {
			/* We found the requested file. */
			return 0;
		}
	}
}

/* Prepare to extract 'control.tar.gz' or 'data.tar.gz' from the outer package
 * archive, returning a `struct archive *` for the enclosed file. On error,
 * return NULL.
 */
static struct archive *
extract_outer(pkg_t * pkg, const char * arname)
{
	int r;
	struct archive * inner;
	struct archive * outer;

	outer = open_outer(pkg->local_filename);
	if (!outer)
		return NULL;

	r = find_inner(outer, arname);
	if (r < 0)
		goto err_cleanup;

	inner = open_inner(outer);
	if (!inner)
		goto err_cleanup;

	return inner;

err_cleanup:
	archive_read_free(outer);
	return NULL;
}

int
pkg_extract_control_file_to_stream(pkg_t *pkg, FILE *stream)
{
	int err = 0;
	struct archive * a = extract_outer(pkg, "control.tar.gz");
	if (!a) {
		opkg_msg(ERROR,
			 "Failed to extract control.tar.gz from package '%s'.\n",
			 pkg->local_filename);
		return -1;
	}

	err = extract_file_to_stream(a, "control", stream);
	archive_read_free(a);
	if (err < 0)
		opkg_msg(ERROR,
			 "Failed to extract control file from package '%s'.\n",
			 pkg->local_filename);

	return err;
}

int
pkg_extract_control_files_to_dir_with_prefix(pkg_t *pkg, const char *dir,
		const char *prefix)
{
	int err;
	int flags;
	char *dir_with_prefix;

	sprintf_alloc(&dir_with_prefix, "%s/%s", dir, prefix);

	struct archive * a = extract_outer(pkg, "control.tar.gz");
	if (!a) {
		free(dir_with_prefix);
		opkg_msg(ERROR,
			 "Failed to extract control.tar.gz from package '%s'.\n",
			 pkg->local_filename);
		return -1;
	}

	/* We don't want to set ownership and permissions of control files
	 * incase they're incorrect in the archive. They should instead be owned
	 * by the user who ran opkg (usually root) and given permissions
	 * according to the current umask.
	 */
	flags = 0;
	err = extract_all(a, dir_with_prefix, flags);
	archive_read_free(a);
	free(dir_with_prefix);
	if (err < 0)
		opkg_msg(ERROR,
			 "Failed to extract all control files from package '%s'.\n",
			 pkg->local_filename);

	return err;
}

int
pkg_extract_control_files_to_dir(pkg_t *pkg, const char *dir)
{
	return pkg_extract_control_files_to_dir_with_prefix(pkg, dir, "");
}


int
pkg_extract_data_files_to_dir(pkg_t *pkg, const char *dir)
{
	int err;
	int flags;

	struct archive * a = extract_outer(pkg, "data.tar.gz");
	if (!a) {
		opkg_msg(ERROR,
			 "Failed to extract data.tar.gz from package '%s'.\n",
			 pkg->local_filename);
		return -1;
	}

	/** Flags:
	 *
	 * TODO: Do we want to support ACLs, extended flags and extended
	 * attributes? (ARCHIVE_EXTRACT_ACL, ARCHIVE_EXTRACT_FFLAGS,
	 * ARCHIVE_EXTRACT_XATTR).
	 */
	flags = ARCHIVE_EXTRACT_OWNER | ARCHIVE_EXTRACT_PERM |
		ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_UNLINK;
	err = extract_all(a, dir, flags);
	archive_read_free(a);
	if (err < 0)
		opkg_msg(ERROR,
			 "Failed to extract data files from package '%s'.\n",
			 pkg->local_filename);

	return err;
}

int
pkg_extract_data_file_names_to_stream(pkg_t *pkg, FILE *stream)
{
	int err;

	struct archive * a = extract_outer(pkg, "data.tar.gz");
	if (!a) {
		opkg_msg(ERROR,
			 "Failed to extract data.tar.gz from package '%s'.\n",
			 pkg->local_filename);
		return -1;
	}

	err = extract_paths_to_stream(a, stream);
	archive_read_free(a);
	if (err < 0)
		opkg_msg(ERROR,
			 "Failed to extract data file names from package '%s'.\n",
			 pkg->local_filename);

	return err;
}
