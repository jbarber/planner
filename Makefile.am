SUBDIRS = \
	po 				\
	po-libplanner			\
	libplanner			\
	libegg				\
	src				\
	data				\
	docs

EXTRA_DIST = \
	libplanner-1.pc			\
	libplanner-1.pc.in		\
	intltool-extract		\
	intltool-merge			\
	intltool-update			\
	intltool-extract.in		\
	intltool-merge.in		\
	intltool-update.in		\
	xmldocs.make			\
	omf.make			\
	planner.spec.in			\
	planner.spec

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libplanner-1.pc

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc

# Workaround broken scrollkeeper that doesn't remove its files on uninstall
distuninstallcheck_listfiles = find . -type f -print | grep -v '^\./var/scrollkeeper'
