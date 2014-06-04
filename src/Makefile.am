AM_CFLAGS = -I${top_srcdir}/libopkg
bin_PROGRAMS = opkg

opkg_SOURCES = opkg.c
opkg_LDADD = $(top_builddir)/libopkg/libopkg.la
if STATIC_LIBOPKG
opkg_LDFLAGS = -static
endif
