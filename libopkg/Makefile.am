
AM_CFLAGS=-Wall -DHOST_CPU_STR=\"@host_cpu@\" -DBUILD_CPU=@build_cpu@ -DLIBDIR=\"@libdir@\" -DOPKGLIBDIR=\"@opkglibdir@\" -DOPKGETCDIR=\"@opkgetcdir@\" -DOPKGLOCKFILE=\"@opkglockfile@\" -DDATADIR=\"@datadir@\" -I$(top_srcdir) $(BIGENDIAN_CFLAGS) $(CURL_CFLAGS) $(GPGME_CFLAGS) $(PATHFINDER_CFLAGS)

libopkg_includedir=$(includedir)/libopkg

# Headers to be installed: everything except opkg_pathfinder.h as that header
# requires symbols from config.h
libopkg_include_HEADERS = $(srcdir)/active_list.h \
	$(srcdir)/cksum_list.h \
	$(srcdir)/conffile.h \
	$(srcdir)/conffile_list.h \
	$(srcdir)/file_util.h \
	$(srcdir)/hash_table.h \
	$(srcdir)/list.h \
	$(srcdir)/md5.h \
	$(srcdir)/nv_pair.h \
	$(srcdir)/nv_pair_list.h \
	$(srcdir)/opkg.h \
	$(srcdir)/opkg_cmd.h \
	$(srcdir)/opkg_conf.h \
	$(srcdir)/opkg_configure.h \
	$(srcdir)/opkg_defines.h \
	$(srcdir)/opkg_download.h \
	$(srcdir)/opkg_install.h \
	$(srcdir)/opkg_message.h \
	$(srcdir)/opkg_remove.h \
	$(srcdir)/opkg_upgrade.h \
	$(srcdir)/opkg_utils.h \
	$(srcdir)/parse_util.h \
	$(srcdir)/pkg.h \
	$(srcdir)/pkg_depends.h \
	$(srcdir)/pkg_dest.h \
	$(srcdir)/pkg_dest_list.h \
	$(srcdir)/pkg_extract.h \
	$(srcdir)/pkg_hash.h \
	$(srcdir)/pkg_parse.h \
	$(srcdir)/pkg_src.h \
	$(srcdir)/pkg_src_list.h \
	$(srcdir)/pkg_vec.h \
	$(srcdir)/release.h \
	$(srcdir)/release_parse.h \
	$(srcdir)/sha256.h \
	$(srcdir)/sprintf_alloc.h \
	$(srcdir)/str_list.h \
	$(srcdir)/void_list.h \
	$(srcdir)/xregex.h \
	$(srcdir)/xsystem.h

opkg_libcore_sources = \
		    opkg.c opkg.h \
		    opkg_defines.h
opkg_cmd_sources = opkg_cmd.c opkg_cmd.h \
		   opkg_configure.c opkg_configure.h \
		   opkg_download.c opkg_download.h \
		   opkg_install.c opkg_install.h \
		   opkg_upgrade.c opkg_upgrade.h \
		   opkg_remove.c opkg_remove.h
opkg_db_sources = opkg_conf.c opkg_conf.h \
		  release.c release.h release_parse.c release_parse.h \
		  opkg_utils.c opkg_utils.h pkg.c pkg.h hash_table.h \
		  pkg_depends.c pkg_depends.h pkg_extract.c pkg_extract.h \
		  hash_table.c pkg_hash.c pkg_hash.h pkg_parse.c pkg_parse.h \
		  pkg_vec.c pkg_vec.h
opkg_list_sources = conffile.c conffile.h conffile_list.c conffile_list.h \
		    nv_pair.c nv_pair.h nv_pair_list.c nv_pair_list.h \
		    pkg_dest.c pkg_dest.h pkg_dest_list.c pkg_dest_list.h \
		    pkg_src.c pkg_src.h pkg_src_list.c pkg_src_list.h \
		    str_list.c str_list.h void_list.c void_list.h \
		    active_list.c active_list.h list.h 
opkg_util_sources = file_util.c file_util.h opkg_message.h opkg_message.c md5.c md5.h \
		    parse_util.c parse_util.h \
		    cksum_list.c cksum_list.h \
		    sprintf_alloc.c sprintf_alloc.h \
		    xregex.c xregex.h xsystem.c xsystem.h
if HAVE_PATHFINDER
opkg_util_sources += opkg_pathfinder.c opkg_pathfinder.h
endif
if HAVE_SHA256
opkg_util_sources += sha256.c sha256.h
endif

lib_LTLIBRARIES = libopkg.la
libopkg_la_SOURCES = \
	$(opkg_libcore_sources) \
	$(opkg_cmd_sources) $(opkg_db_sources) \
	$(opkg_util_sources) $(opkg_list_sources)

libopkg_la_LIBADD = $(top_builddir)/libbb/libbb.la $(CURL_LIBS) $(GPGME_LIBS) $(OPENSSL_LIBS) $(PATHFINDER_LIBS)

libopkg_la_LDFLAGS = -version-info 1:0:0

# make sure we only export symbols that are for public use
#libopkg_la_LDFLAGS = -export-symbols-regex "^opkg_.*"



