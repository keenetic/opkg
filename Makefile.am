SUBDIRS = libbb libopkg src tests utils man

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libopkg.pc



interceptdir = $(datadir)/opkg/intercept
intercept_DATA = intercept/ldconfig intercept/depmod intercept/update-modules

install-data-hook:
	chmod +x $(DESTDIR)$(datadir)/opkg/intercept/*

EXTRA_DIST = $(intercept_DATA) tests/regress CONTRIBUTORS ChangeLog.ipkg

# Ensure libopkg/config.h doesn't slip into a distribution archive.
# TODO: Fix headers and remove config.h from libopkg_include_HEADERS so that
# this hack isn't needed!
opkg-distdir: distdir
	rm -f $(distdir)/libopkg/config.h

opkg-dist: opkg-distdir
	tar c $(distdir) | gzip -9 > $(distdir).tar.gz
