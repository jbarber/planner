INCLUDES = \
	$(PLANNER_CFLAGS) 		\
	-DEGG_COMPILATION
	-DGTK_DISABLE_DEPRECATED	\
	-DG_DISABLE_DEPRECATED

noinst_LTLIBRARIES = libeggrecent.la

libeggrecent_la_LIBADD = $(PLANNER_LIBS)

libeggrecent_la_SOURCES =		\
	egg-recent.h			\
	egg-recent-item.h		\
	egg-recent-item.c		\
	egg-recent-model.c		\
	egg-recent-model.h		\
	egg-recent-view.c		\
	egg-recent-view.h		\
	egg-recent-view-gtk.c		\
	egg-recent-view-gtk.h		\
	egg-recent-view-bonobo.c	\
	egg-recent-view-bonobo.h	\
	egg-recent-util.c		\
	egg-recent-util.h		\
	egg-recent-vfs-utils.c		\
	egg-recent-vfs-utils.h	

