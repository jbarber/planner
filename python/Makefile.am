NULL =

INCLUDES = 				\
	-I$(top_srcdir)			\
	$(PYTHON_INCLUDES)		\
	$(PYGTK_CFLAGS)			\
	$(LIBPLANNER_CFLAGS)		\
	$(NULL)

pkgpythondir = $(pythondir)/gtk-2.0
pkgpyexecdir = $(pyexecdir)/gtk-2.0

pkgpyexec_LTLIBRARIES = planner.la
planner_la_CFLAGS = -Wno-missing-declarations -Wno-missing-prototypes
planner_la_LDFLAGS = -module -avoid-version -export-symbols-regex initplanner
planner_la_LIBADD = $(LIBPLANNER_LIBS) $(top_builddir)/libplanner/libplanner-1.la
planner_la_SOURCES = plannermodule.c
nodist_planner_la_SOURCES = planner.c

planner.c: planner.defs planner.override

CLEANFILES = planner.c

EXTRA_DIST = 				\
	planner.override		\
	planner.defs			\
	planner-types.defs		\
	planner-arg-types.py 		\
	$(NULL)

.defs.c:
	(cd $(srcdir)\
	 && $(PYGTK_CODEGEN) \
	    --override $*.override \
            --load-types planner-arg-types.py \
	    --prefix $* $*.defs) > gen-$*.c \
	&& cp gen-$*.c $*.c \
	&& rm -f gen-$*.c

