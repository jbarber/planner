NULL =

INCLUDES = 				\
	-I$(top_srcdir)			\
	$(PYTHON_INCLUDES)		\
	$(PYGTK_CFLAGS)			\
	$(LIBPLANNER_CFLAGS)		\
	$(NULL)

CFLAGS += -Wno-missing-declarations -Wno-missing-prototypes

pyexec_LTLIBRARIES = plannermodule.la
plannermodule_la_LDFLAGS = -module -avoid-version -export-symbols-regex initplanner
plannermodule_la_LIBADD = $(LIBPLANNER_LIBS) ../planner/libplanner.la
plannermodule_la_SOURCES = plannermodule.c
nodist_plannermodule_la_SOURCES = planner.c

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

