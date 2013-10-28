#! /bin/sh

set -e

if [ \( $# -eq 1 \) -a \( "$1" = "--clean" \) ]; then
	# Deep clean of all generated files
	rm -f configure
	rm -f config.log config.status
	rm -f libtool
	rm -f aclocal.m4
	rm -f libopkg.pc
	rm -f man/{opkg-cl.1,opkg-key.1}
	rm -f Makefile {libbb,libopkg,tests,man,utils,src}/Makefile
	rm -f Makefile.in {libbb,libopkg,tests,man,utils,src}/Makefile.in
	rm -f libopkg/config.h{,.in}
	rm -rf po conf autom4te.cache
	rm -rf {libbb,libopkg,src,tests}/.deps

	rm -f {libbb,libopkg,tests,src}/*.o
	rm -f {libbb,libopkg}/*.lo
	rm -f {libbb,libopkg}/*.la
	rm -f src/opkg-cl tests/libopkg_test
	rm -f libopkg/stamp-h1
	rm -f utils/update-alternatives
	rm -rf {libbb,libopkg,tests,src}/.libs

	rm -f tests/regress/*.py{c,o}
	rm -rf tests/regress/__pycache__

	exit 0
fi

# If we didn't get '--clean' we're bootstrapping the project
autoreconf -v --install || exit 1
glib-gettextize --force --copy || exit 1
./configure "$@"

