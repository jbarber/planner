keys_in_files = planner.keys.in
keys_files = $(keys_in_files:.keys.in=.keys)
@INTLTOOL_KEYS_RULE@

xml_in_files = planner.xml.in
xml_files = $(xml_in_files:.xml.in=.xml)
@INTLTOOL_XML_RULE@

mimedir = $(datadir)/mime-info
mime_DATA = $(keys_files) planner.mime

applicationsdir = $(datadir)/application-registry
applications_DATA = planner.applications

newmimedir = $(datadir)/mime/packages
newmime_DATA = $(xml_files)

EXTRA_DIST =				\
	$(keys_in_files)		\
	$(mime_DATA)			\
	$(applications_DATA)		\
	$(xml_in_files)

CLEANFILES = $(keys_files) $(xml_files)

install-data-hook:
if HAVE_NEW_MIME
	$(UPDATE_MIME_DATABASE) "$(DESTDIR)$(datadir)/mime"
endif

uninstall-hook:
if HAVE_NEW_MIME
	$(UPDATE_MIME_DATABASE) "$(DESTDIR)$(datadir)/mime"
endif

