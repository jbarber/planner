keys_in_files = planner.keys.in
keys_files = $(keys_in_files:.keys.in=.keys)
@INTLTOOL_KEYS_RULE@

mimedir = $(datadir)/mime-info
mime_DATA = $(keys_files) planner.mime

applicationsdir = $(datadir)/application-registry
applications_DATA = planner.applications

EXTRA_DIST =				\
	$(keys_in_files)		\
	$(mime_DATA)			\
	$(applications_DATA)

CLEANFILES = $(keys_files)
