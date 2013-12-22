/* vi: set sw=4 ts=4: */
/*
 * Busybox main internal header file
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef	__LIBBB_H__
#define	__LIBBB_H__    1

#include <stdio.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <netdb.h>

#include "../libopkg/opkg_message.h"

#define error_msg(fmt, args...) opkg_msg(ERROR, fmt"\n", ##args)
#define perror_msg(fmt, args...) opkg_perror(ERROR, fmt, ##args)
#define error_msg_and_die(fmt, args...) \
	do { \
		error_msg(fmt, ##args); \
		exit(EXIT_FAILURE); \
	} while (0)
#define perror_msg_and_die(fmt, args...) \
	do { \
		perror_msg(fmt, ##args); \
		exit(EXIT_FAILURE); \
	} while (0)

int copy_file(const char *source, const char *dest, int flags);
int copy_file_chunk(FILE *src_file, FILE *dst_file, unsigned long long chunksize);

extern void *xmalloc (size_t size);
extern void *xrealloc(void *old, size_t size);
extern void *xcalloc(size_t nmemb, size_t size);
extern char *xstrdup (const char *s);
extern char *xstrndup (const char *s, int n);

int make_directory (const char *path, long mode, int flags);

enum {
	FILEUTILS_PRESERVE_STATUS = 1,
	FILEUTILS_RECUR = 4,
	FILEUTILS_FORCE = 8,
};

#endif /* __LIBBB_H__ */
