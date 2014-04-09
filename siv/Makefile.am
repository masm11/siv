SUBDIRS = src include

EXTRA_DIST = siv-config.in siv.m4.in
CLEANFILES = siv-config siv.m4

bin_SCRIPTS = siv-config

siv-config: siv-config.in Makefile
	sed -e 's,[@]pkglibexecdir[@],$(pkglibexecdir),g' \
	    -e 's,[@]pkgincludedir[@],$(pkgincludedir),g' \
	    -e 's,[@]version[@],$(VERSION),g' \
	    < $(srcdir)/siv-config.in > siv-config
	chmod +x siv-config

aclocaldir = @aclocaldir@
aclocal_DATA = siv.m4

siv.m4: siv.m4.in Makefile
	sed -e 's,[@]version[@],$(VERSION),g' \
	    < $(srcdir)/siv.m4.in > siv.m4

AUTOMAKE_OPTIONS = foreign
DISTCHECK_CONFIGURE_FLAGS = --disable-honor-aclocaldir
