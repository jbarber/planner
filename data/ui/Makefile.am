uidir = $(datadir)/planner/ui

python_script_ui_files = \
	python-plugin.ui

if HAVE_PYTHON_PLUGIN
python_script_ui = $(python_script_ui_files)
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
	xml-planner-plugin.ui	\
	time-table-view.ui	\
	$(python_script_ui)


EXTRA_DIST = $(ui_DATA) $(python_script_ui_files)
