# nls/tr_TR/Makefile.am for Blackbox-0.61.x - an X11 Window Manager
# 
# -- T�rkce terc�me dizini i�in bir yapim-dosya-taban�
# 	kabalak -- kabalak@gmx.net
#

NLSTEST = @NLS@
CLEANFILES = blackbox.cat
MAINTAINERCLEANFILES = Makefile.in Translation.m
MFILES = BaseDisplay.m Basemenu.m Configmenu.m Icon.m Image.m Screen.m Slit.m Toolbar.m Window.m Windowmenu.m Workspace.m Workspacemenu.m blackbox.m common.m main.m bsetroot.m
EXTRA_DIST=$(MFILES)

all-local: blackbox.cat
install-data-local: blackbox.cat
	@if test x$(NLSTEST) = "x-DNLS"; then \
		echo "Installing catalog in $(DESTDIR)$(pkgdatadir)/nls/tr_TR"; \
		$(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/nls/tr_TR; \
		$(INSTALL_DATA) blackbox.cat $(DESTDIR)$(pkgdatadir)/nls/tr_TR; \
	fi
uninstall-local:
	@if test x$(NLSTEST) = "x-DNLS"; then \
		rm -f $(DESTDIR)$(pkgdatadir)/nls/tr_TR/blackbox.cat; \
		rmdir $(DESTDIR)$(pkgdatadir)/nls/tr_TR; \
	fi

Translation.m: $(MFILES)
	@if test x$(NLSTEST) = "x-DNLS"; then \
		rm -f Translation.m; \
		awk -f ../convert.awk output=Translation.m $(MFILES); \
	fi

blackbox.cat: Translation.m
	@if test x$(NLSTEST) = "x-DNLS"; then \
		LANG=tr_TR $(gencat_cmd) blackbox.cat Translation.m; \
	fi
