SUBDIRS = \
	po 				\
	libplanner			\
	libegg				\
	src				\
	data				\
	docs

EXTRA_DIST = \
	libplanner-1.pc			\
	libplanner-1.pc.in		\
	intltool-extract.in		\
	intltool-merge.in		\
	intltool-update.in		\
	xmldocs.make			\
	omf.make			\
	planner.spec.in			\
	planner.spec			\
	intltool-modules/XML/Parser/Style/OrigTree.pm

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libplanner-1.pc

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc

# Workaround broken scrollkeeper that doesn't remove its files on uninstall
distuninstallcheck_listfiles = find . -type f -print | grep -v '^\./var/scrollkeeper'
