INCLUDES = \
	-I$(top_srcdir)						\
	-I$(top_srcdir)/src					\
	$(PLANNER_CFLAGS)					\
	$(WARN_CFLAGS)						\
	-DG_LOG_DOMAIN=\"Planner\"				\
	-DPREFIX=\""$(prefix)"\"				\
	-DSYSCONFDIR=\""$(sysconfdir)"\"			\
	-DLIBDIR=\""$(libdir)"\"				\
	-DDATADIR=\""$(datadir)"\"				\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"		\
	-DIMAGEDIR=\""$(datadir)/pixmaps/planner"\"		\
	-DGLADEDIR=\""$(datadir)/planner/glade"\"		\
	-DMRP_VIEWDIR=\""$(libdir)/planner/views"\"		\
	-DMRP_PLUGINDIR=\""$(libdir)/planner/plugins"\"		\
	$(GNOMEUI_UNSTABLE)

bin_PROGRAMS = planner 

planner_SOURCES = \
	eel-canvas-rect.c		\
	eel-canvas-rect.h		\
	planner-application.c		\
	planner-application.h		\
	planner-assignment-model.c	\
	planner-assignment-model.h	\
	planner-calendar.c		\
	planner-calendar-dialog.c	\
	planner-calendar-dialog.h	\
	planner-calendar.h		\
	planner-calendar-selector.c	\
	planner-calendar-selector.h	\
	planner-canvas-line.c		\
	planner-canvas-line.h		\
	planner-cell-renderer-date.c	\
	planner-cell-renderer-date.h	\
	planner-cell-renderer-list.c	\
	planner-cell-renderer-list.h	\
	planner-cell-renderer-popup.c	\
	planner-cell-renderer-popup.h	\
	planner-cmd-manager.c		\
	planner-cmd-manager.h		\
	planner-day-type-dialog.c	\
	planner-day-type-dialog.h	\
	planner-default-week-dialog.c	\
	planner-default-week-dialog.h	\
	planner-format.c		\
	planner-format.h		\
	planner-group-dialog.c		\
	planner-group-dialog.h		\
	planner-group-model.c		\
	planner-group-model.h		\
	planner-list-model.c		\
	planner-list-model.h		\
	planner-main.c			\
	planner-phase-dialog.c		\
	planner-phase-dialog.h		\
	planner-plugin.c		\
	planner-plugin.h		\
	planner-plugin-loader.c		\
	planner-plugin-loader.h		\
	planner-popup-entry.c		\
	planner-popup-entry.h		\
	planner-predecessor-model.c	\
	planner-predecessor-model.h	\
	planner-print-dialog.c		\
	planner-print-dialog.h		\
	planner-print-job.c		\
	planner-print-job.h		\
	planner-project-properties.c	\
	planner-project-properties.h	\
	planner-property-dialog.c	\
	planner-property-dialog.h	\
	planner-property-model.c	\
	planner-property-model.h	\
	planner-resource-dialog.c	\
	planner-resource-dialog.h	\
	planner-resource-input-dialog.c	\
	planner-resource-input-dialog.h	\
	planner-sidebar.c		\
	planner-sidebar.h		\
	planner-table-print-sheet.c	\
	planner-table-print-sheet.h	\
	planner-task-dialog.c		\
	planner-task-dialog.h		\
	planner-task-input-dialog.c	\
	planner-task-input-dialog.h	\
	planner-view.c			\
	planner-view.h			\
	planner-view-loader.c		\
	planner-view-loader.h		\
	planner-window.c		\
	planner-window.h		\
	planner-working-time-dialog.c	\
	planner-working-time-dialog.h	


planner_LDFLAGS = -export-dynamic

planner_LDADD =	\
	$(top_builddir)/libplanner/libplanner-1.la \
	$(top_builddir)/libegg/recent-files/libeggrecent.la \
	libutil.la	\
	$(PLANNER_LIBS)

#	$(top_builddir)/libfoocanvas/libfoocanvas.la 

noinst_LTLIBRARIES = libutil.la
libutil_la_SOURCES = planner-marshal-main.c

#
# Views
#

viewdir = $(libdir)/planner/views
view_LTLIBRARIES = libgantt-view.la libtask-view.la libresource-view.la 

libgantt_view_la_SOURCES = \
	planner-relation-arrow.c	\
	planner-relation-arrow.h	\
	planner-gantt-background.c	\
	planner-gantt-background.h	\
	planner-gantt-chart.c		\
	planner-gantt-chart.h		\
	planner-gantt-header.c		\
	planner-gantt-header.h		\
	planner-gantt-model.c		\
	planner-gantt-model.h		\
	planner-gantt-print.c		\
	planner-gantt-print.h		\
	planner-gantt-row.c		\
	planner-gantt-row.h		\
	planner-gantt-view.c		\
	planner-scale-utils.c		\
	planner-scale-utils.h		\
	planner-task-tree.c		\
	planner-task-tree.h			

libgantt_view_la_LDFLAGS = -avoid-version -module
libgantt_view_la_LIBADD = libutil.la

libtask_view_la_SOURCES = \
	planner-task-view.c		\
	planner-gantt-model.c		\
	planner-gantt-model.h		\
	planner-task-tree.c		\
	planner-task-tree.h			

libtask_view_la_LDFLAGS = -avoid-version -module
libtask_view_la_LIBADD = libutil.la

libresource_view_la_SOURCES = \
	planner-resource-view.c		

libresource_view_la_LDFLAGS = -avoid-version -module
libresource_view_la_LIBADD = libutil.la

#
# Plugins
#

plugindir = $(libdir)/planner/plugins

if HAVE_POSTGRES
sql_plugin = libsql-plugin.la
endif

plugin_LTLIBRARIES = \
	libhtml-plugin.la		\
	$(sql_plugin)

libhtml_plugin_la_SOURCES = planner-html-plugin.c
libhtml_plugin_la_LDFLAGS = -avoid-version -module

if HAVE_POSTGRES
libsql_plugin_la_SOURCES = planner-sql-plugin.c
libsql_plugin_la_LDFLAGS = -avoid-version -module
libsql_plugin_la_CFLAGS = $(PG_CFLAGS)
libsql_plugin_la_LIBADD = $(PG_LIBS)
endif

planner-marshal.h: Makefile.am planner-marshal.list
	(cd $(srcdir) && \
	@GLIB_GENMARSHAL@ --prefix=planner_marshal planner-marshal.list --header) > xgen-gmh \
	&& (cmp -s xgen-gmh planner-marshal.h || cp xgen-gmh planner-marshal.h) \
	&& rm -f xgen-gmh xgen-gmh~

planner-marshal.c: Makefile.am planner-marshal.list
	(cd $(srcdir) && \
	@GLIB_GENMARSHAL@ --prefix=planner_marshal planner-marshal.list --body) > xgen-gmc \
	&& cp xgen-gmc planner-marshal.c \
	&& rm -f xgen-gmc xgen-gmc~

planner-marshal-main.c: $(built_sources)

# do not use BUILT_SOURCES because of weird semantics
# i *think* these should actually be stamp-* files
built_sources = planner-marshal.h planner-marshal.c

# an explicit dependency here so all generated files get built
$(OBJECTS): $(built_sources)

EXTRA_DIST = planner-marshal.list

CLEANFILES = $(built_sources)
