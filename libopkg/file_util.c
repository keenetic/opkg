/* file_util.c - convenience routines for common stat operations

   Copyright (C) 2009 Ubiq Technologies <graham.gower@gmail.com>

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

#include "config.h"

#include <archive.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <utime.h>
#include <fcntl.h>
#include <malloc.h>
#include <string.h>

#include "opkg_message.h"
#include "sprintf_alloc.h"
#include "file_util.h"
#include "md5.h"
#include "libbb/libbb.h"

#if defined HAVE_SHA256
#include "sha256.h"
#endif

int
file_exists(const char *file_name)
{
	struct stat st;

	if (stat(file_name, &st) == -1)
		return 0;

	return 1;
}

int
file_is_dir(const char *file_name)
{
	struct stat st;

	if (stat(file_name, &st) == -1)
		return 0;

	return S_ISDIR(st.st_mode);
}

/* read a single line from a file, stopping at a newline or EOF.
   If a newline is read, it will appear in the resulting string.
   Return value is a malloc'ed char * which should be freed at
   some point by the caller.

   Return value is NULL if the file is at EOF when called.
*/
char *
file_read_line_alloc(FILE *fp)
{
	char buf[BUFSIZ];
	unsigned int buf_len;
	char *line = NULL;
	unsigned int line_size = 0;
	int got_nl = 0;

	buf[0] = '\0';

	while (fgets(buf, BUFSIZ, fp)) {
		buf_len = strlen(buf);
		if (buf[buf_len - 1] == '\n') {
			buf_len--;
			buf[buf_len] = '\0';
			got_nl = 1;
		}
		if (line) {
			line_size += buf_len;
			line = xrealloc(line, line_size+1);
			strncat(line, buf, line_size);
		} else {
			line_size = buf_len + 1;
			line = xstrdup(buf);
		}
		if (got_nl)
			break;
	}

	return line;
}

int
file_move(const char *src, const char *dest)
{
	int err;

	err = rename(src, dest);
	if (err == -1) {
		if (errno == EXDEV) {
			/* src & dest live on different file systems */
			err = file_copy(src, dest);
			if (err == 0)
				unlink(src);
		} else {
			opkg_perror(ERROR, "Failed to rename %s to %s",
				src, dest);
		}
	}

	return err;
}

static int
copy_file_data(FILE *src_file, FILE *dst_file)
{
	size_t nread, nwritten;
	char buffer[BUFSIZ];

	while (1) {
		nread = fread (buffer, 1, BUFSIZ, src_file);

		if (nread != BUFSIZ && ferror (src_file)) {
			opkg_perror(ERROR, "read");
			return -1;
		}

		/* Check for EOF. */
		if (nread == 0)
			return 0;

		nwritten = fwrite (buffer, 1, nread, dst_file);

		if (nwritten != nread) {
			if (ferror (dst_file))
				opkg_perror(ERROR, "write");
			else
				opkg_msg(ERROR, "Unable to write all data.\n");
			return -1;
		}
	}
}

int
file_copy(const char *src, const char *dest)
{
	struct stat src_stat;
	struct stat dest_stat;
	int dest_exists = 1;
	int status = 0;

	if (stat(src, &src_stat) < 0) {
		opkg_perror(ERROR, "%s", src);
		return -1;
	}

	if (stat(dest, &dest_stat) < 0) {
		if (errno != ENOENT) {
			opkg_perror(ERROR, "unable to stat `%s'", dest);
			return -1;
		}
		dest_exists = 0;
	}

	if (dest_exists && src_stat.st_rdev == dest_stat.st_rdev &&
			src_stat.st_ino == dest_stat.st_ino) {
		opkg_msg(ERROR, "`%s' and `%s' are the same file.\n", src, dest);
		return -1;
	}

	if (S_ISREG(src_stat.st_mode)) {
		FILE *sfp, *dfp;
		struct utimbuf times;

		if (dest_exists) {
			dfp = fopen(dest, "w");
			if (dfp == NULL) {
				if (unlink(dest) < 0) {
					opkg_perror(ERROR, "unable to remove `%s'", dest);
					return -1;
				}
			}
		} else {
			int fd;

			fd = open(dest, O_WRONLY|O_CREAT, src_stat.st_mode);
			if (fd < 0) {
				opkg_perror(ERROR, "unable to open `%s'", dest);
				return -1;
			}
			dfp = fdopen(fd, "w");
			if (dfp == NULL) {
				if (fd >= 0)
					close(fd);
				opkg_perror(ERROR, "unable to open `%s'", dest);
				return -1;
			}
		}

		sfp = fopen(src, "r");
		if (sfp) {
			if (copy_file_data(sfp, dfp) < 0)
				status = -1;

			if (fclose(sfp) < 0) {
				opkg_perror(ERROR, "unable to close `%s'", src);
				status = -1;
			}
		} else {
			opkg_perror(ERROR, "unable to open `%s'", src);
			status = -1;
		}

		if (fclose(dfp) < 0) {
			opkg_perror(ERROR, "unable to close `%s'", dest);
			status = -1;
		}

		times.actime = src_stat.st_atime;
		times.modtime = src_stat.st_mtime;
		if (utime(dest, &times) < 0)
			opkg_perror(ERROR, "unable to preserve times of `%s'", dest);
		if (chown(dest, src_stat.st_uid, src_stat.st_gid) < 0) {
			src_stat.st_mode &= ~(S_ISUID | S_ISGID);
			opkg_perror(ERROR, "unable to preserve ownership of `%s'", dest);
		}
		if (chmod(dest, src_stat.st_mode) < 0)
			opkg_perror(ERROR, "unable to preserve permissions of `%s'", dest);

		return status;
	} else if (S_ISBLK(src_stat.st_mode) || S_ISCHR(src_stat.st_mode) ||
			S_ISSOCK(src_stat.st_mode)) {
		if (mknod(dest, src_stat.st_mode, src_stat.st_rdev) < 0) {
			opkg_perror(ERROR, "unable to create `%s'", dest);
			return -1;
		}
	} else if (S_ISFIFO(src_stat.st_mode)) {
		if (mkfifo(dest, src_stat.st_mode) < 0) {
			opkg_perror(ERROR, "cannot create fifo `%s'", dest);
			return -1;
		}
	} else if (S_ISDIR(src_stat.st_mode)) {
		opkg_msg(ERROR, "%s: omitting directory.\n", src);
		return -1;
	}

	opkg_msg(ERROR, "internal error: unrecognized file type.\n");
	return -1;
}

int
file_mkdir_hier(const char *path, long mode)
{
	struct stat st;

	if (stat (path, &st) < 0 && errno == ENOENT) {
		int status;
		char *parent;

		parent = xdirname(path);
		status = file_mkdir_hier(parent, mode | 0300);
		free(parent);

		if (status < 0)
			return -1;

		if (mkdir (path, 0777) < 0) {
			opkg_perror(ERROR, "Cannot create directory `%s'", path);
			return -1;
		}

		if (mode != -1 && chmod (path, mode) < 0) {
			opkg_perror(ERROR, "Cannot set permissions of directory `%s'", path);
			return -1;
		}
	}

	return 0;
}

char *file_md5sum_alloc(const char *file_name)
{
    static const int md5sum_bin_len = 16;
    static const int md5sum_hex_len = 32;

    static const unsigned char bin2hex[16] = {
	'0', '1', '2', '3',
	'4', '5', '6', '7',
	'8', '9', 'a', 'b',
	'c', 'd', 'e', 'f'
    };

    int i, err;
    FILE *file;
    char *md5sum_hex;
    unsigned char md5sum_bin[md5sum_bin_len];

    md5sum_hex = xcalloc(1, md5sum_hex_len + 1);

    file = fopen(file_name, "r");
    if (file == NULL) {
	opkg_perror(ERROR, "Failed to open file %s", file_name);
	free(md5sum_hex);
	return NULL;
    }

    err = md5_stream(file, md5sum_bin);
    if (err) {
	opkg_msg(ERROR, "Could't compute md5sum for %s.\n", file_name);
	fclose(file);
	free(md5sum_hex);
	return NULL;
    }

    fclose(file);

    for (i=0; i < md5sum_bin_len; i++) {
	md5sum_hex[i*2] = bin2hex[md5sum_bin[i] >> 4];
	md5sum_hex[i*2+1] = bin2hex[md5sum_bin[i] & 0xf];
    }

    md5sum_hex[md5sum_hex_len] = '\0';

    return md5sum_hex;
}

#ifdef HAVE_SHA256
char *file_sha256sum_alloc(const char *file_name)
{
    static const int sha256sum_bin_len = 32;
    static const int sha256sum_hex_len = 64;

    static const unsigned char bin2hex[16] = {
	'0', '1', '2', '3',
	'4', '5', '6', '7',
	'8', '9', 'a', 'b',
	'c', 'd', 'e', 'f'
    };

    int i, err;
    FILE *file;
    char *sha256sum_hex;
    unsigned char sha256sum_bin[sha256sum_bin_len];

    sha256sum_hex = xcalloc(1, sha256sum_hex_len + 1);

    file = fopen(file_name, "r");
    if (file == NULL) {
	opkg_perror(ERROR, "Failed to open file %s", file_name);
	free(sha256sum_hex);
	return NULL;
    }

    err = sha256_stream(file, sha256sum_bin);
    if (err) {
	opkg_msg(ERROR, "Could't compute sha256sum for %s.\n", file_name);
	fclose(file);
	free(sha256sum_hex);
	return NULL;
    }

    fclose(file);

    for (i=0; i < sha256sum_bin_len; i++) {
	sha256sum_hex[i*2] = bin2hex[sha256sum_bin[i] >> 4];
	sha256sum_hex[i*2+1] = bin2hex[sha256sum_bin[i] & 0xf];
    }

    sha256sum_hex[sha256sum_hex_len] = '\0';

    return sha256sum_hex;
}

#endif


int
rm_r(const char *path)
{
	int ret = 0;
	DIR *dir;
	struct dirent *dent;

	if (path == NULL) {
		opkg_perror(ERROR, "Missing directory parameter");
		return -1;
	}

	dir = opendir(path);
	if (dir == NULL) {
		opkg_perror(ERROR, "Failed to open dir %s", path);
		return -1;
	}

	if (fchdir(dirfd(dir)) == -1) {
		opkg_perror(ERROR, "Failed to change to dir %s", path);
		closedir(dir);
		return -1;
	}

	while (1) {
		errno = 0;
		if ((dent = readdir(dir)) == NULL) {
			if (errno) {
				opkg_perror(ERROR, "Failed to read dir %s",
						path);
				ret = -1;
			}
			break;
		}

		if (!strcmp(dent->d_name, ".") || !strcmp(dent->d_name, ".."))
			continue;

#ifdef _BSD_SOURCE
		if (dent->d_type == DT_DIR) {
			if ((ret = rm_r(dent->d_name)) == -1)
				break;
			continue;
		} else if (dent->d_type == DT_UNKNOWN)
#endif
		{
			struct stat st;
			if ((ret = lstat(dent->d_name, &st)) == -1) {
				opkg_perror(ERROR, "Failed to lstat %s",
						dent->d_name);
				break;
			}
			if (S_ISDIR(st.st_mode)) {
				if ((ret = rm_r(dent->d_name)) == -1)
					break;
				continue;
			}
		}

		if ((ret = unlink(dent->d_name)) == -1) {
			opkg_perror(ERROR, "Failed to unlink %s", dent->d_name);
			break;
		}
	}

	if (chdir("..") == -1) {
		ret = -1;
		opkg_perror(ERROR, "Failed to change to dir %s/..", path);
	}

	if (rmdir(path) == -1 ) {
		ret = -1;
		opkg_perror(ERROR, "Failed to remove dir %s", path);
	}

	if (closedir(dir) == -1) {
		ret = -1;
		opkg_perror(ERROR, "Failed to close dir %s", path);
	}

	return ret;
}

int
file_decompress(const char * in, const char * out)
{
	int r = 0;
	size_t sz;
	void * buffer = NULL;
	struct archive * ar = NULL;
	struct archive_entry * entry;
	FILE * f = NULL;

	buffer = xmalloc(EXTRACT_BUFFER_LEN);
	ar = archive_read_new();
	if (!ar) {
		r = -1;
		goto err;
	}

	/* Support raw data in any compression format. */
	archive_read_support_filter_all(ar);
	archive_read_support_format_raw(ar);

	/* Open input file and prepare for reading. */
	r = archive_read_open_filename(ar, in, EXTRACT_BUFFER_LEN);
	if (r != ARCHIVE_OK)
		goto err;

	r = archive_read_next_header(ar, &entry);
	if (r != ARCHIVE_OK)
		goto err;

	/* Open output file. */
	f = fopen(out, "w");
	if (!f) {
		r = -1;
		goto err;
	}

	/* Copy decompressed data. */
	while (1) {
		sz = archive_read_data(ar, buffer, EXTRACT_BUFFER_LEN);
		if (sz < 0) {
			r = sz;
			break;
		}
		if (sz == 0) {
			/* We finished. */
			r = 0;
			break;
		}

		r = fwrite(buffer, 1, sz, f);
		if (r < sz) {
			break;
		}
	}

err:
	if (ar)
		archive_read_free(ar);
	if (buffer)
		free(buffer);
	if (f)
		fclose(f);

	return r;
}
