#!/bin/sh

PACKAGE="opkg"

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

cd "$srcdir"
DIE=0

if [ \( $# -eq 1 \) -a \( "$1" = "--clean" \) ]; then
	# Deep clean of all generated files
	echo "Removing old files (no Makefile around?) ..."
	rm -f configure
	rm -f config.log config.status
	rm -f libtool
	rm -f aclocal.m4
	rm -f libopkg.pc
	rm -f man/{opkg-cl.1,opkg-key.1}
	rm -f Makefile {libbb,libopkg,tests,man,utils,src}/Makefile
	rm -f Makefile.in {libbb,libopkg,tests,man,utils,src}/Makefile.in
	rm -f libopkg/config.h{,.in}
	rm -rf po conf autom4te.cache m4
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

	echo "Done. If you want regenerate the Autotool files call 'autogen.sh' without the '--clean' argument."
	exit 0
fi

(autoconf --version) < /dev/null > /dev/null 2>&1 || {
	echo
	echo "You must have autoconf installed to compile $PACKAGE."
	echo "Download the appropriate PACKAGE for your system,"
	echo "or get the source from one of the GNU ftp sites"
	echo "listed in http://www.gnu.org/order/ftp.html"
	DIE=1
}

(automake --version) < /dev/null > /dev/null 2>&1 || {
	echo
	echo "You must have automake installed to compile $PACKAGE."
	echo "Download the appropriate PACKAGE for your system,"
	echo "or get the source from one of the GNU ftp sites"
	echo "listed in http://www.gnu.org/order/ftp.html"
	DIE=1
}

(libtool --version) < /dev/null > /dev/null 2>&1 || {
	echo
	echo "You must have libtool installed to compile $PACKAGE."
	echo "Download the appropriate PACKAGE for your system,"
	echo "or get the source from one of the GNU ftp sites"
	echo "listed in http://www.gnu.org/order/ftp.html"
	DIE=1
}

# currently gettext is not used, no need to check
#(gettext --version) < /dev/null > /dev/null 2>&1 || {
#	echo
#	echo "You must have gettext installed to compile $PACKAGE."
#	echo "Download the appropriate PACKAGE for your system,"
#	echo "or get the source from one of the GNU ftp sites"
#	echo "listed in http://www.gnu.org/order/ftp.html"
#	DIE=1
#}

if test "$DIE" -eq 1; then
	exit 1
fi

echo "Generating configuration files for $PACKAGE, please wait...."
if [ "$ACLOCAL_FLAGS" == "" ]; then
        echo "No option for 'aclocal' given. Possibly you have forgotten to use 'ACLOCAL_FLAGS='?"
fi

echo "  aclocal $ACLOCAL_FLAGS"
aclocal $ACLOCAL_FLAGS
if [ "$?" = "1" ]; then
	echo "aclocal failed!" && exit 1
fi

echo "  libtoolize --automake"
libtoolize --automake
if [ "$?" = "1" ]; then
	echo "libtoolize failed!" && exit 1
fi

#echo "  gettextize"
#gettextize
#if [ "$?" = "1" ]; then
#	"echo gettextsize failed!" && exit 1
#fi
echo "  autoconf"
autoconf
if [ "$?" = "1" ]; then
	echo "autoconf failed!" && exit 1
fi

echo "  autoheader"
autoheader
if [ "$?" = "1" ]; then
	echo "autoheader failed!" && exit 1
fi

echo "  automake --add-missing"
automake --add-missing
if [ "$?" = "1" ]; then
	echo "automake failed!" && exit 1
fi

echo "You can now run 'configure [options]' to configure $PACKAGE."
