INCLUDES = 						\
	$(LIBMRPROJECT_CFLAGS)				\
	-I$(top_srcdir)

LDFLAGS =						\
	$(LIBMRPROJECT_LIBS)				\
	$(top_builddir)/mrproject/libmrproject.la

noinst_LTLIBRARIES = libselfcheck.la

libselfcheck_la_SOURCES =				\
	self-check.c					\
	self-check.h

noinst_PROGRAMS = time-test calendar-test task-test

time_test_SOURCES = time-test.c
time_test_LDFLAGS = $(top_builddir)/tests/libselfcheck.la

calendar_test_SOURCES = calendar-test.c
calendar_test_LDFLAGS = $(top_builddir)/tests/libselfcheck.la

task_test_SOURCES = task-test.c
task_test_LDFLAGS = $(top_builddir)/tests/libselfcheck.la

TESTS = time-test calendar-test task-test
