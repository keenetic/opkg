ACLOCAL_AMFLAGS = -I m4

SUBDIRS = libopkg src tests utils man

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libopkg.pc



interceptdir = $(datadir)/opkg/intercept
intercept_DATA = intercept/ldconfig intercept/depmod intercept/update-modules

install-data-hook:
	chmod +x $(DESTDIR)$(datadir)/opkg/intercept/*

EXTRA_DIST = $(intercept_DATA) tests/regress CONTRIBUTING ChangeLog.ipkg \
	     developer-doc
