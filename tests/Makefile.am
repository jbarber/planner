SUBDIRS = files

INCLUDES = \
	-I$(top_srcdir)					\
	-I$(top_srcdir)/tests				\
	$(PLANNER_CFLAGS) $(WARN_CFLAGS)		\
	-DG_LOG_DOMAIN=\"Planner\"			\
	-DEXAMPLESDIR=\"$(top_srcdir)/tests/files\"

LDADD =	\
	$(top_builddir)/libplanner/libplanner-1.la 	\
	$(PLANNER_LIBS)

noinst_LTLIBRARIES = libselfcheck.la

libselfcheck_la_SOURCES =                              \
       self-check.c                                    \
       self-check.h

noinst_PROGRAMS = scheduler-test time-test calendar-test task-test

scheduler_test_SOURCES = scheduler-test.c
scheduler_test_LDFLAGS = $(top_builddir)/tests/libselfcheck.la

time_test_SOURCES = time-test.c
time_test_LDFLAGS = $(top_builddir)/tests/libselfcheck.la

calendar_test_SOURCES = calendar-test.c
calendar_test_LDFLAGS = $(top_builddir)/tests/libselfcheck.la

task_test_SOURCES = task-test.c
task_test_LDFLAGS = $(top_builddir)/tests/libselfcheck.la

TESTS = scheduler-test time-test calendar-test task-test
