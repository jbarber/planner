LIBRARY=	libplanner.dll
APIS=		libplanner-api.xml

INCLUDE_APIS=	
ASSEMBLIES=	-pkg:gtk-sharp

if HAVE_DOTNET

libplannersharpdir = $(libdir)
libplannersharp_DATA = libplanner.dll

gapidir=$(datadir)/gapi
gapi_DATA = $(APIS)

all: $(LIBRARY)

generated-stamp: $(APIS) $(srcdir)/*.custom $(srcdir)/*.metadata
	$(GAPI_FIXUP) --api=$(srcdir)/libplanner-api.xml --metadata=$(srcdir)/libplanner.metadata && \
	$(GAPI_CODEGEN) --include $(INCLUDE_APIS) --generate $(APIS) \
	--outdir=generated --customdir=$(srcdir) --assembly-name=libplanner && touch generated-stamp

$(LIBRARY): generated-stamp
	$(MCS) --unsafe -nowarn:0660,0661 --target library $(ASSEMBLIES) --recurse '*.cs' -o $(LIBRARY)

install-data-local:
	$(INSTALL) -m 644 $(LIBRARY) $(libdir)

uninstall-local:
	rm -f $(libdir)/$(LIBRARY)

clean:
	rm -rf $(LIBRARY) generated generated-stamp

EXTRA_DIST = *.custom *.metadata *.xml

endif

