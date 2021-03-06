// IconBar.hh for Fluxbox Window Manager
// Copyright (c) 2001 - 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// $Id: IconBar.hh,v 1.15 2003/06/26 12:22:42 rathnor Exp $

#ifndef ICONBAR_HH
#define ICONBAR_HH

#include <X11/Xlib.h>
#include <list>

class FluxboxWindow;
class BScreen;

namespace FbTk {
class Font;
};
/**
	Icon object in IconBar
*/
class IconBarObj
{
public:	
    IconBarObj(FluxboxWindow *fluxboxwin, Window iconwin);
    ~IconBarObj();
    Window getIconWin() const { return m_iconwin; }
    FluxboxWindow *getFluxboxWin() { return m_fluxboxwin; }
    const FluxboxWindow *getFluxboxWin() const { return m_fluxboxwin; }
    unsigned int width() const;
    unsigned int height() const;
private:
    FluxboxWindow *m_fluxboxwin;
    Window m_iconwin;
};

/**
	Icon container
*/
class IconBar
{
public:
    typedef std::list<Window> WindowList;
    IconBar(BScreen &scrn, Window parent, FbTk::Font &font);
    ~IconBar();
    void draw(); //TODO
    void reconfigure();
    Window addIcon(FluxboxWindow *fluxboxwin);
    Window delIcon(FluxboxWindow *fluxboxwin);
    WindowList *delAllIcons(bool ignore_stuck = false);
    void buttonPressEvent(XButtonEvent *be);	
    FluxboxWindow *findWindow(Window w);
    IconBarObj *findIcon(FluxboxWindow * const fluxboxwin);
    const IconBarObj *findIcon(const FluxboxWindow * const fluxboxwin) const;
    void exposeEvent(XExposeEvent *ee);

    void enableUpdates();
    void disableUpdates();    

    void draw(const IconBarObj * const obj, int width) const;
    void setVertical(bool value) { m_vertical = value; }
    BScreen &screen() { return m_screen; }
    const BScreen &screen() const { return m_screen; }
private:
    typedef std::list<IconBarObj *> IconList;

    void loadTheme(unsigned int width, unsigned int height);
    void decorate(Window win);
    void repositionIcons();
    Window createIconWindow(FluxboxWindow *fluxboxwin, Window parent);
    BScreen &m_screen;
    Display *m_display;
    Window m_parent;
    IconList m_iconlist;	
    Pixmap m_focus_pm;
    unsigned long m_focus_pixel;
    bool m_vertical;
    FbTk::Font &m_font;

    int allow_updates;
};

#endif // ICONBAR_HH
