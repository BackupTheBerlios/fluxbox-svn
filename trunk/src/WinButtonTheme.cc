// WinButtonTheme.cc for Fluxbox - an X11 Window manager
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

// $Id: WinButtonTheme.cc,v 1.4 2003/08/22 14:48:10 fluxgen Exp $

#include "WinButtonTheme.hh"

#include "FbTk/App.hh"
#include "FbWinFrameTheme.hh"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef HAVE_XPM
#include <X11/xpm.h>
#endif // HAVE_XPM

// not used
template <>
void FbTk::ThemeItem<WinButtonTheme::PixmapWithMask>::
load() { }

template <>
void FbTk::ThemeItem<WinButtonTheme::PixmapWithMask>::
setDefaultValue() {
    // create empty pixmap
    (*this)->pixmap = FbTk::FbPixmap(); // pixmap
    (*this)->mask = FbTk::FbPixmap(); // mask
}

template <>
void FbTk::ThemeItem<WinButtonTheme::PixmapWithMask>::
setFromString(const char *str) {
    if (str == 0)
        setDefaultValue();
    else {
#ifdef HAVE_XPM
        XpmAttributes xpm_attr;
        xpm_attr.valuemask = 0;
        Display *dpy = FbTk::App::instance()->display();
        Pixmap pm = 0, mask = 0;
        int retvalue = XpmReadFileToPixmap(dpy,
                                           RootWindow(dpy, m_tm.screenNum()), 
                                           const_cast<char *>(str),
                                           &pm,
                                           &mask, &xpm_attr);
        if (retvalue == 0) { // success
            (*this)->pixmap = pm;
            (*this)->mask = mask;            
        } else {  // failure        
            setDefaultValue();
        }
#else
        setDefaultValue();
#endif // HAVE_XPM

    } 
}

WinButtonTheme::WinButtonTheme(int screen_num, const FbWinFrameTheme &frame_theme):
    FbTk::Theme(screen_num),
    m_close_pm(*this, "window.close.pixmap", "Window.Close.Pixmap"),
    m_close_unfocus_pm(*this, "window.close.unfocus.pixmap", "Window.Close.Unfocus.Pixmap"),
    m_close_pressed_pm(*this, "window.close.pressed.pixmap", "Window.Close.Pressed.Pixmap"),
    m_maximize_pm(*this, "window.maximize.pixmap", "Window.Maximize.Pixmap"),
    m_maximize_unfocus_pm(*this, "window.maximize.unfocus.pixmap", "Window.Maximize.Unfocus.pixmap"),
    m_maximize_pressed_pm(*this, "window.maximize.pressed.pixmap", "Window.Maximize.Pressed.Pixmap"),
    m_iconify_pm(*this, "window.iconify.pixmap", "Window.Iconify.Pixmap"),
    m_iconify_unfocus_pm(*this, "window.iconify.unfocus.pixmap", "Window.Iconify.Unfocus.Pixmap"),    
    m_iconify_pressed_pm(*this, "window.iconify.pressed.pixmap", "Window.Iconify.Pressed.Pixmap"),
    m_shade_pm(*this, "window.shade.pixmap", "Window.Shade.Pixmap"),
    m_shade_unfocus_pm(*this, "window.shade.unfocus.pixmap", "Window.Shade.Unfocus.Pixmap"),
    m_shade_pressed_pm(*this, "window.shade.pressed.pixmap", "Window.Shade.Pressed.Pixmap"),
    m_stick_pm(*this, "window.stick.pixmap", "Window.Stick.Pixmap"),
    m_stick_unfocus_pm(*this, "window.stick.unfocus.pixmap", "Window.Stick.Unfocus.Pixmap"),
    m_stick_pressed_pm(*this, "window.stick.pressed.pixmap", "Window.Stick.Pressed.Pixmap"),
    m_stuck_pm(*this, "window.stuck.pixmap", "Window.Stuck.Pixmap"),
    m_stuck_unfocus_pm(*this, "window.stuck.unfocus.pixmap", "Window.Stuck.Unfocus.Pixmap"),
    m_frame_theme(frame_theme) {

}

WinButtonTheme::~WinButtonTheme() {

}

void WinButtonTheme::reconfigTheme() {
    // rescale the pixmaps to match frame theme height

    unsigned int size = m_frame_theme.titleHeight();
    if (m_frame_theme.titleHeight() == 0) {
        // calculate height from font and border width to scale pixmaps
        const int bevel = 1;
        size = m_frame_theme.font().height() + bevel*2 + 2;

    } // else  use specified height to scale pixmaps

    // scale all pixmaps
    m_close_pm->scale(size, size);
    m_close_unfocus_pm->scale(size, size);
    m_close_pressed_pm->scale(size, size);
        
    m_maximize_pm->scale(size, size);
    m_maximize_unfocus_pm->scale(size, size);
    m_maximize_pressed_pm->scale(size, size);

    m_iconify_pm->scale(size, size);
    m_iconify_unfocus_pm->scale(size, size);
    m_iconify_pressed_pm->scale(size, size);

    m_shade_pm->scale(size, size);
    m_shade_unfocus_pm->scale(size, size);
    m_shade_pressed_pm->scale(size, size);

    m_stick_pm->scale(size, size);
    m_stick_unfocus_pm->scale(size, size);
    m_stick_pressed_pm->scale(size, size);

    m_stuck_pm->scale(size, size);
    m_stuck_unfocus_pm->scale(size, size);
}

