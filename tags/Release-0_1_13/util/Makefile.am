# util/Makefile.am for Blackbox 0.61.x  - an X11 Window manager
SUBDIRS = fbrun
bin_SCRIPTS = bsetbg fluxbox-generate_menu
bin_PROGRAMS =		bsetroot
bsetroot_SOURCES =	bsetroot.cc bsetroot.hh
bsetroot_LDADD =	../src/BaseDisplay.o ../src/Timer.o ../src/Image.o ../src/i18n.o ../src/FbAtoms.o \
	../src/Color.o ../src/Texture.o ../src/StringUtil.o

MAINTAINERCLEANFILES =	Makefile.in
EXTRA_DIST=bsetbg fluxbox-generate_menu

distclean-local:
	rm -f *\~

bsetroot.o: bsetroot.cc ../config.h bsetroot.hh ../src/BaseDisplay.hh \
 ../src/Timer.hh ../src/Image.hh ../src/FbAtoms.hh ../src/Color.hh ../src/Texture.hh

