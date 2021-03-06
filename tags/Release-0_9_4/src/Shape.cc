// Shape.cc
// Copyright (c) 2003 Henrik Kinnunen (fluxgen(at)users.sourceforge.net)
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
// DEALINGS IN THE SOFTWARE.

// $Id: Shape.cc,v 1.2 2003/07/10 14:47:53 fluxgen Exp $

#include "Shape.hh"
#include "FbWindow.hh"
#include "App.hh"

#include <cstring>

#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif // SHAPE

#include <iostream>
#include <algorithm>
using namespace std;

namespace {

Pixmap createShape(FbTk::FbWindow &win, int place) {
    if (win.window() == 0 || place == 0 || 
        win.width() < 3 || win.height() < 3)
        return 0;
    
    static char left_bits[] = { 0xc0, 0xf8, 0xfc, 0xfe, 0xfe, 0xfe, 0xff, 0xff };
    static char right_bits[] = { 0x03, 0x1f, 0x3f, 0x7f, 0x7f, 0x7f, 0xff, 0xff};
    static char bottom_left_bits[] = { 0xff, 0xff, 0xfe, 0xfe, 0xfe, 0xfc, 0xf8, 0xc0 };
    static char bottom_right_bits[] = { 0xff, 0xff, 0x7f, 0x7f, 0x7f, 0x3f, 0x1f, 0x03 };

    const int win_width = win.width() + 2*win.borderWidth() + 1;
    const int win_height = win.height() + 2*win.borderWidth();
    const int pixmap_width = min(8, win_width);
    const int pixmap_height = min(8, win_height);

    Display *disp = FbTk::App::instance()->display();
    const size_t data_size = win_width * win_height;
    char *data = new char[data_size];
    memset(data, 0xFF, data_size);
    
    XImage *ximage = XCreateImage(disp,
                                  DefaultVisual(disp, DefaultScreen(disp)),
                                  1,
                                  XYPixmap, 0,
                                  data,
                                  win_width, win_height,
                                  32, 0);
    if (ximage == 0)
        return 0;

    XInitImage(ximage);

    // shape corners

    if (place & Shape::TOPLEFT) {
        for (int y=0; y<pixmap_height; y++) {
            for (int x=0; x<pixmap_width; x++) {
                XPutPixel(ximage, x + 1, y, (left_bits[y] & (0x01 << x)) ? 1 : 0);
            }
        }
    }
    
    if (place & Shape::TOPRIGHT) {
        for (int y=0; y<pixmap_height; y++) {
            for (int x=0; x<pixmap_width; x++) {
                XPutPixel(ximage, x + win_width - pixmap_width, y, 
                          (right_bits[y] & (0x01 << x)) ? 1 : 0);
            }     
        }
    }
    
    if (place & Shape::BOTTOMLEFT) {
        for (int y=0; y<pixmap_height; y++) {
            for (int x=0; x<pixmap_width; x++) {
                XPutPixel(ximage, x + 1, y + win_height - pixmap_height, 
                          (bottom_left_bits[y] & (0x01 << x)) ? 1 : 0);
            }
        }
    }

    if (place & Shape::BOTTOMRIGHT) {
        for (int y=0; y<pixmap_height; y++) {
            for (int x=0; x<pixmap_width; x++) {
                XPutPixel(ximage, x + win_width - pixmap_width, y + win_height - pixmap_height,
                          (bottom_right_bits[y] & (0x01 << x)) ? 1 : 0);
            }
        }
    }

    for (int y = 0; y<pixmap_height; ++y) {
        XPutPixel(ximage, 0, y, 1);
    }

    XGCValues values;
    values.foreground = 1;
    values.background = 0;
    Pixmap pm = XCreatePixmap(disp, win.window(), win_width, win_height, 1);
    GC gc = XCreateGC(disp, pm,
                      GCForeground | GCBackground, &values);

        
    XPutImage(disp, pm, gc, ximage, 0, 0, 0, 0,
              win_width, win_height);

    XFreeGC(disp, gc);
    XDestroyImage(ximage);

    return pm;


}

}; // end anonymous namespace

Shape::Shape(FbTk::FbWindow &win, int shapeplaces):
    m_win(&win),
    m_shapeplaces(shapeplaces) {

    m_shape = createShape(win, shapeplaces);
    m_width = win.width();
    m_height = win.height();

}

Shape::~Shape() {
    if (m_shape != 0)
        XFreePixmap(FbTk::App::instance()->display(), m_shape);
}

void Shape::setPlaces(int shapeplaces) {
    m_shapeplaces = shapeplaces;
}

void Shape::update() {
    if (m_win == 0 || m_win->window() == 0)
        return;
#ifdef SHAPE
    if (m_win->width() != m_width ||
        m_win->height() != m_height) {
        if (m_shape != 0)
            XFreePixmap(FbTk::App::instance()->display(), m_shape);
        m_shape = createShape(*m_win, m_shapeplaces);
        m_width = m_win->width();
        m_height = m_win->height();

    }

    if (m_shape == 0)
        return;

    XShapeCombineMask(FbTk::App::instance()->display(),
                      m_win->window(),
                      ShapeBounding,
                      -2, 0,
                      m_shape,
                      ShapeSet);

#endif // SHAPE

}

void Shape::setWindow(FbTk::FbWindow &win) {
    m_win = &win;
    update();
}
