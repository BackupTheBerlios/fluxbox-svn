// Copyright (c) 2002 Henrik Kinnunen (fluxgen at linuxmail.org)
// Copyright (c) 1997 - 2000 Brad Hughes <bhughes at trolltech.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER

// $Id: bsetroot.cc,v 1.22 2004/08/31 15:26:40 rathnor Exp $

#include "bsetroot.hh"

#include "../src/FbTk/I18n.hh"
#include "../src/FbTk/ImageControl.hh"
#include "../src/FbRootWindow.hh"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <X11/Xatom.h>

#ifdef HAVE_CSTRING
  #include <cstring>
#else
  #include <string.h>
#endif
#ifdef HAVE_CSTDLIB
  #include <cstdlib>
#else
  #include <stdlib.h>
#endif
#ifdef HAVE_CSTDIO
  #include <cstdio>
#else
  #include <stdio.h>
#endif
#include <iostream>

using namespace std;

bsetroot::bsetroot(int argc, char **argv, char *dpy_name)
    : FbTk::App(dpy_name), m_app_name(argv[0]) {

    pixmaps = (Pixmap *) 0;
    grad = fore = back = (char *) 0;

    bool mod = false, sol = false, grd = false;
    int mod_x = 0, mod_y = 0, i = 0;

    img_ctrl = new FbTk::ImageControl*[ScreenCount(display())];
    for (; i < ScreenCount(display()); i++) {
        img_ctrl[i] = new FbTk::ImageControl(i, true);
    }

    for (i = 1; i < argc; i++) {
        if (! strcmp("-help", argv[i])) {
            usage();

        } else if ((! strcmp("-fg", argv[i])) ||
                   (! strcmp("-foreground", argv[i])) ||
                   (! strcmp("-from", argv[i]))) {
            if ((++i) >= argc)
                usage(1);
            fore = argv[i];

        } else if ((! strcmp("-bg", argv[i])) ||
                   (! strcmp("-background", argv[i])) ||
                   (! strcmp("-to", argv[i]))) {
            if ((++i) >= argc)
                usage(1);
            back = argv[i];

        } else if (! strcmp("-solid", argv[i])) {
            if ((++i) >= argc)
                usage(1);
            fore = argv[i];
            sol = true;

        } else if (! strcmp("-mod", argv[i])) {
            if ((++i) >= argc)
                usage();
            mod_x = atoi(argv[i]);
            if ((++i) >= argc)
                usage();
            mod_y = atoi(argv[i]);
            if (mod_x < 1)
                mod_x = 1;
            if (mod_y < 1)
                mod_y = 1;
            mod = true;

        } else if (! strcmp("-gradient", argv[i])) {
            if ((++i) >= argc)
                usage();

            grad = argv[i];
            grd = true;

        } else if (! strcmp("-display", argv[i])) {
            // -display passed through tests earlier... we just skip it now
            i++;

        } else
            usage();
    }

    if ((mod + sol + grd) != true) {
        _FB_USES_NLS;
        cerr<<m_app_name<<
            _FBTEXT(bsetroot, MustSpecify, 
                    "Error: must specify one of: -solid, -mod, -gradient\n",
                    "user didn't give one of the required options")<<endl;

        usage(2);
    }

    num_screens = ScreenCount(display());
 
    if (sol && fore)
        solid();
    else if (mod && mod_x && mod_y && fore && back)
        modula(mod_x, mod_y);
    else if (grd && grad && fore && back)
        gradient();
    else
        usage();

}


bsetroot::~bsetroot() {
    XKillClient(display(), AllTemporary);

    if (pixmaps) { // should always be true
        XSetCloseDownMode(display(), RetainTemporary);

        delete [] pixmaps;
    }

    if (img_ctrl != 0) {
        int i = 0;
        for (; i < num_screens; i++)
            delete img_ctrl[i];

        delete [] img_ctrl;
    } 
}

/**
   set root pixmap atoms so that apps like
   Eterm and xchat will be able to use
   transparent background
*/
void bsetroot::setRootAtoms(Pixmap pixmap, int screen) {
    Atom atom_root, atom_eroot, type;
    unsigned char *data_root, *data_eroot;
    int format;
    unsigned long length, after;

    atom_root = XInternAtom(display(), "_XROOTMAP_ID", true);
    atom_eroot = XInternAtom(display(), "ESETROOT_PMAP_ID", true);
    FbRootWindow root(screen);

    // doing this to clean up after old background
    if (atom_root != None && atom_eroot != None) {

        XGetWindowProperty(display(), root.window(),
                           atom_root, 0L, 1L, false, AnyPropertyType,
                           &type, &format, &length, &after, &data_root);

        if (type == XA_PIXMAP) {
            XGetWindowProperty(display(), root.window(),
                               atom_eroot, 0L, 1L, False, AnyPropertyType,
                               &type, &format, &length, &after, &data_eroot);
                
            if (data_root && data_eroot && type == XA_PIXMAP &&
                *((Pixmap *) data_root) == *((Pixmap *) data_eroot)) {
                    
                XKillClient(display(), *((Pixmap *) data_root));
            }
        }
    }

    atom_root = XInternAtom(display(), "_XROOTPMAP_ID", false);
    atom_eroot = XInternAtom(display(), "ESETROOT_PMAP_ID", false);

    if (atom_root == None || atom_eroot == None) {
        _FB_USES_NLS;
        cerr<<_FBTEXT(bsetroot, NoPixmapAtoms, "Couldn't create pixmap atoms, giving up!", "Couldn't create atoms to point at root pixmap")<<endl;
        exit(1);
    }

    // setting new background atoms
    XChangeProperty(display(), root.window(),
                    atom_root, XA_PIXMAP, 32, PropModeReplace, (unsigned char *) &pixmap, 1);
    XChangeProperty(display(), root.window(),
                    atom_eroot, XA_PIXMAP, 32, PropModeReplace, (unsigned char *) &pixmap, 1);

}

/**
   Draws pixmaps with a single color 
*/
void bsetroot::solid() {
    register int screen = 0;

    pixmaps = new Pixmap[ScreenCount(display())];

    for (; screen < ScreenCount(display()); screen++) {
        FbTk::Color c;
        if (!c.setFromString(fore, screen)) // just report error
            continue;

        FbRootWindow root(screen);

        GC gc;
        XGCValues gcv;

        if (! c.isAllocated())
            c.setPixel(BlackPixel(display(), screen));

        gcv.foreground = c.pixel();
        gc = XCreateGC(display(), root.window(),
                       GCForeground , &gcv);

        pixmaps[screen] = XCreatePixmap(display(), 
                                        root.window(),
                                        root.width(), root.height(),
                                        root.depth());

        XFillRectangle(display(), pixmaps[screen], gc, 0, 0,
                       root.width(), root.height());

        setRootAtoms(pixmaps[screen], screen);

        XSetWindowBackgroundPixmap(display(),
                                   root.window(), pixmaps[screen]);

        XClearWindow(display(), root.window());

        XFreeGC(display(), gc);
    }
}

/**
 Draws pixmaps with an 16x16 pattern with
 fg and bg colors.
*/
void bsetroot::modula(int x, int y) {
    char data[32];
    long pattern;

    register int screen, i;

    pixmaps = new Pixmap[ScreenCount(display())];

    for (pattern = 0, screen = 0; screen < ScreenCount(display()); screen++) {
        FbRootWindow root(screen);

        for (i = 0; i < 16; i++) {
            pattern <<= 1;
            if ((i % x) == 0)
                pattern |= 0x0001;
        }

        for (i = 0; i < 16; i++) {
            if ((i %  y) == 0) {
                data[(i * 2)] = (char) 0xff;
                data[(i * 2) + 1] = (char) 0xff;
            } else {
                data[(i * 2)] = pattern & 0xff;
                data[(i * 2) + 1] = (pattern >> 8) & 0xff;
            }
        }

        FbTk::Color f, b;
        GC gc;
        Pixmap bitmap, r_bitmap;
        XGCValues gcv;

        bitmap = XCreateBitmapFromData(display(),
                                       root.window(), data, 16, 16);

        // bitmap used as tile, needs to have the same depth as background pixmap
        r_bitmap = XCreatePixmap(display(),
                                 root.window(), 16, 16,
                                 root.depth());

        f.setFromString(fore, screen);
        b.setFromString(back, screen);

        if (! f.isAllocated())
            f.setPixel(WhitePixel(display(), screen));
        if (! b.isAllocated())
            b.setPixel(BlackPixel(display(), screen));

        gcv.foreground = f.pixel();
        gcv.background = b.pixel();

        gc = XCreateGC(display(), root.window(),
                       GCForeground | GCBackground, &gcv);

        // copying bitmap to the one going to be used as tile
        XCopyPlane(display(), bitmap, r_bitmap, gc,
                   0, 0, 16, 16, 0, 0, 1l);

        XSetTile(display(), gc, r_bitmap);
        XSetFillStyle(display(), gc, FillTiled);	

        pixmaps[screen] = XCreatePixmap(display(), 
                                        root.window(),
                                        root.width(), root.height(),
                                        root.depth());

        XFillRectangle(display(), pixmaps[screen], gc, 0, 0,
                       root.width(), root.height());

        setRootAtoms(pixmaps[screen], screen);

        XSetWindowBackgroundPixmap(display(),
                                   root.window(), pixmaps[screen]);

        XClearWindow(display(), root.window());

        XFreeGC(display(), gc);
        XFreePixmap(display(), bitmap);
        XFreePixmap(display(), r_bitmap);
    }
}

/**
 draws pixmaps with a fluxbox texure
*/
void bsetroot::gradient() {
    // using temporaray pixmap and then copying it to background pixmap, as it'll
    // get crashed somewhere on the way causing apps like XChat chrashing
    // as the pixmap has been destroyed
    Pixmap tmp;
    pixmaps = new Pixmap[ScreenCount(display())];

    for (int screen = 0; screen < ScreenCount(display()); screen++) {
        FbRootWindow root(screen);

        FbTk::Texture texture;
        GC gc;
        XGCValues gcv;

        texture.setFromString(grad);
        texture.color().setFromString(fore, screen);
        texture.colorTo().setFromString(back, screen);
		
        if (! texture.color().isAllocated())
            texture.color().setPixel(WhitePixel(display(), screen));
        if (! texture.colorTo().isAllocated())
            texture.colorTo().setPixel(BlackPixel(display(), screen));

        tmp = img_ctrl[screen]->renderImage(root.width(),
                                            root.height(), texture);

        pixmaps[screen] = XCreatePixmap(display(), 
                                        root.window(),
                                        root.width(), root.height(),
                                        root.depth());

        gc = XCreateGC(display(), root.window(),
                       GCForeground , &gcv);

        XCopyArea(display(), tmp, pixmaps[screen], gc, 0, 0,
                  root.width(), root.height(),
                  0, 0);

        setRootAtoms(pixmaps[screen], screen);

        XSetWindowBackgroundPixmap(display(),
                                   root.window(), pixmaps[screen]);

        XClearWindow(display(), root.window());

        if (! (root.visual()->c_class & 1)) {
            img_ctrl[screen]->removeImage(tmp);
            img_ctrl[screen]->cleanCache();
        }

        XFreeGC(display(), gc);
    }
}

/**
 Shows information about usage
*/
void bsetroot::usage(int exit_code) {
    _FB_USES_NLS;
    cerr<<m_app_name<<" 2.2 : (c) 2003 Fluxbox Development Team"<<endl;
    cerr<<m_app_name<<" 2.1 : (c) 2002 Claes Nasten"<<endl;
    cerr<<m_app_name<<" 2.0 : (c) 1997-2000 Brad Hughes\n"<<endl;
    cerr<<_FBTEXT(bsetroot, Usage,
                  "  -display <string>        display connection\n"
                  "  -mod <x> <y>             modula pattern\n"
                  "  -foreground, -fg <color> modula foreground color\n"
                  "  -background, -bg <color> modula background color\n\n"
                  "  -gradient <texture>      gradient texture\n"
                  "  -from <color>            gradient start color\n"
                  "  -to <color>              gradient end color\n\n"
                  "  -solid <color>           solid color\n\n"
                  "  -help                    print this help text and exit\n",
                  "bsetroot usage options")<<endl;
    exit(exit_code);
}


int main(int argc, char **argv) {
    char *display_name = (char *) 0;
    int i = 1;
  
    FbTk::NLSInit("fluxbox.cat");
  
    for (; i < argc; i++) {
        if (! strcmp(argv[i], "-display")) {
            // check for -display option

            if ((++i) >= argc) {
                _FB_USES_NLS;
                cerr<<_FBTEXT(main, DISPLAYRequiresArg,
                              "error: '-display' requires an argument",
                              "option requires an argument")<<endl;

                ::exit(1);
            }

            display_name = argv[i];
        }
    }
 
    bsetroot app(argc, argv, display_name);
 
    return (0);
}
