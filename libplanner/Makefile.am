INCLUDES = \
	-I. -I$(top_srcdir) \
	$(LIBPLANNER_CFLAGS) $(WARN_CFLAGS) \
	-DMRP_STORAGEMODULEDIR=\""$(libdir)/planner/storage-modules"\" \
	-DMRP_FILE_MODULES_DIR=\""$(libdir)/planner/file-modules"\"

lib_LTLIBRARIES = libplanner-1.la

libplanner_1_la_SOURCES = \
	mrp-marshal-main.c			\
	mrp-application.c			\
	mrp-assignment.c			\
	mrp-calendar.c				\
	mrp-day.c				\
	mrp-group.c				\
	mrp-object.c				\
	mrp-file-module.c			\
	mrp-file-module.h			\
	mrp-project.c				\
	mrp-resource.c				\
	mrp-storage-module.c			\
	mrp-storage-module-factory.c		\
	mrp-storage-module-factory.h		\
	mrp-task-manager.c			\
	mrp-task-manager.h			\
	mrp-task.c				\
	mrp-relation.c				\
	mrp-types.c				\
	mrp-private.h				\
	mrp-property.c				\
	mrp-time.c				\
	mrp-error.c				\
	mrp-intl.h

libplannerinclude_HEADERS = \
	planner.h				\
	mrp-application.h			\
	mrp-assignment.h			\
	mrp-calendar.h				\
	mrp-day.h				\
	mrp-group.h				\
	mrp-object.h				\
	mrp-project.h				\
	mrp-property.h				\
	mrp-resource.h				\
	mrp-storage-module.h			\
	mrp-task.h				\
	mrp-relation.h				\
	mrp-types.h				\
	mrp-time.h				\
	mrp-error.h

libplanner_1_la_LIBADD = $(LIBPLANNER_LIBS)

# -export-symbols-regex "^[^_].*"

libplannerincludedir = $(includedir)/planner-1.0/libplanner

#
# Storage modules
#

storagemoduledir = $(libdir)/planner/storage-modules

if HAVE_GDA
sql_library = libstorage-sql.la
endif

storagemodule_LTLIBRARIES = \
	libstorage-mrproject-1.la			\
	$(sql_library)

libstorage_mrproject_1_la_SOURCES = \
	mrp-storage-mrproject.c				\
	mrp-storage-mrproject.h				\
	mrp-parser.c					\
	mrp-parser.h

libstorage_mrproject_1_la_LDFLAGS =  -avoid-version -module

if HAVE_GDA
libstorage_sql_la_SOURCES = \
	mrp-storage-sql.c				\
	mrp-storage-sql.h				\
	mrp-sql.c					\
	mrp-sql.h

libstorage_sql_la_LDFLAGS =  -avoid-version -module
libstorage_sql_la_LIBADD =  $(GDA_LIBS)
libstorage_sql_la_CFLAGS = $(GDA_CFLAGS) 
endif

#
# File modules
#

filemoduledir = $(libdir)/planner/file-modules

filemodule_LTLIBRARIES = \
	libmrp-xml.la					\
	libmrp-xsl.la

#	libmrp-mpx.la

libmrp_xml_la_SOURCES = \
	mrp-xml.c					\
	mrp-old-xml.c					\
	mrp-old-xml.h

libmrp_xml_la_CFLAGS = -DDTDDIR=\""$(datadir)/planner/dtd"\"
libmrp_xml_la_LDFLAGS = -avoid-version -module


libmrp_xsl_la_SOURCES = \
	mrp-xsl.c

libmrp_xsl_la_CFLAGS = \
	$(XSLT_CFLAGS)					\
	-DSTYLESHEETDIR=\""$(datadir)/planner/stylesheets"\"

libmrp_xsl_la_LIBADD = $(XSLT_LIBS)
libmrp_xsl_la_LDFLAGS = -avoid-version -module


#libmrp_mpx_la_SOURCES = \
#	mrp-mpx.c
#libmrp_mpx_la_LDFLAGS = -avoid-version -module

mrp-marshal.h: Makefile.am mrp-marshal.list
	(cd $(srcdir) && \
	@GLIB_GENMARSHAL@ --prefix=mrp_marshal mrp-marshal.list --header) > xgen-gmh \
	&& (cmp -s xgen-gmh mrp-marshal.h || cp xgen-gmh mrp-marshal.h) \
	&& rm -f xgen-gmh xgen-gmh~

mrp-marshal.c: Makefile.am mrp-marshal.list
	(cd $(srcdir) && \
	@GLIB_GENMARSHAL@ --prefix=mrp_marshal mrp-marshal.list --body) > xgen-gmc \
	&& cp xgen-gmc mrp-marshal.c \
	&& rm -f xgen-gmc xgen-gmc~

mrp-marshal-main.c: mrp-marshal.c mrp-marshal.h

# do not use BUILT_SOURCES because of weird semantics
# i *think* these should actually be stamp-* files
built_sources =	 mrp-marshal.h  mrp-marshal.c

$(libplanner_OBJECTS): $(built_sources)

EXTRA_DIST = mrp-marshal.list

CLEANFILES = $(built_sources)
