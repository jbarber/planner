uidir = $(datadir)/planner/ui

if BUILD_TIMETABLE
timetable_view_ui = time-table-view.ui
else
timetable_view_ui =
endif

if HAVE_PYTHON_PLUGIN
python_script_ui = python-plugin.ui
else
python_script_ui =
endif

ui_DATA = \
	gantt-view.ui		\
	task-view.ui		\
	gantt-view.ui		\
	html-plugin.ui		\
	main-window.ui		\
	resource-view.ui	\
	sql-plugin.ui		\
	task-view.ui		\
	$(python_script_ui)	\
	$(timetable_view_ui)

EXTRA_DIST = $(ui_DATA) $(timetable_view_ui) $(python_script_ui)
