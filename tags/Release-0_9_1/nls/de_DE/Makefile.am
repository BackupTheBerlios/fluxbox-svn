# nls/de_DE/Makefile.am for fluxbox - fluxbox.sf.net

NLSTEST = @NLS@
CLEANFILES = fluxbox.cat
MAINTAINERCLEANFILES = Makefile.in Translation.m
MFILES = BaseDisplay.m Basemenu.m Configmenu.m Icon.m Image.m Screen.m Slit.m Toolbar.m Window.m Windowmenu.m Workspace.m Workspacemenu.m fluxbox.m common.m main.m bsetroot.m
EXTRA_DIST= $(MFILES)

all-local: fluxbox.cat

install-data-local: fluxbox.cat
	@if test x$(NLSTEST) = "x-DNLS"; then \
		echo "Installing catalog in $(DESTDIR)$(pkgdatadir)/nls/de_DE"; \
		$(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/nls/de_DE; \
		$(INSTALL_DATA) fluxbox.cat $(DESTDIR)$(pkgdatadir)/nls/de_DE; \
	fi

uninstall-local:
	@if test x$(NLSTEST) = "x-DNLS"; then \
		rm -f $(DESTDIR)$(pkgdatadir)/nls/de_DE/fluxbox.cat; \
		rmdir $(DESTDIR)$(pkgdatadir)/nls/de_DE; \
	fi

# note... when adding new catalogs... you do not need the
# header=../fluxbox-nls.hh which is passed to awk... we
# only need to generate it once.

Translation.m: $(MFILES)
	@if test x$(NLSTEST) = "x-DNLS"; then \
		rm -f Translation.m; \
		awk -f ../convert.awk output=Translation.m $(MFILES); \
	fi

fluxbox.cat: Translation.m
	@if test x$(NLSTEST) = "x-DNLS"; then \
		$(gencat_cmd) fluxbox.cat Translation.m; \
	fi
