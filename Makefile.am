if HAVE_DOTNET
dotnet_DIR = dotnet
endif

if HAVE_PYTHON
python_DIR = python
endif

SUBDIRS = \
	po 				\
	libplanner			\
	libegg				\
	src				\
	data				\
	docs				\
	examples			\
	$(python_DIR)			\
	$(dotnet_DIR)

EXTRA_DIST = \
	libplanner-1.pc			\
	libplanner-1.pc.in		\
	intltool-extract.in		\
	intltool-merge.in		\
	intltool-update.in		\
	xmldocs.make			\
	omf.make			\
	planner.spec.in			\
	planner.spec

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libplanner-1.pc

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc --enable-database --disable-dotnet --enable-python

# Workaround broken scrollkeeper that doesn't remove its files on
# uninstall. Same with the shared mime info, the generated files
# aren't empty when there is no mime data
distuninstallcheck_listfiles = find . -type f -print | grep -v '^\./var/scrollkeeper' | \
 grep -v '/share/mime/' | grep -v '/share/omf/planner'

DISTCLEANFILES = intltool-extract intltool-merge intltool-update 

