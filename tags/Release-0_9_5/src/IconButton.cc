// IconButton.cc
// Copyright (c) 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
//                and Simon Bowden    (rathnor at users.sourceforge.net)
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

// $Id: IconButton.cc,v 1.7 2003/08/24 16:24:19 fluxgen Exp $

#include "IconButton.hh"

#include "FbTk/App.hh"
#include "FbTk/EventManager.hh"

#include "fluxbox.hh"
#include "Screen.hh"
#include "Window.hh"
#include "WinClient.hh"
#include "SimpleCommand.hh"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <X11/Xutil.h>
#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif // SHAPE

namespace {

class ShowMenu: public FbTk::Command {
public:
    explicit ShowMenu(FluxboxWindow &win):m_win(win) { }
    void execute() {
        // get last button pos
        const XEvent &event = Fluxbox::instance()->lastEvent();
        int x = event.xbutton.x_root - (m_win.menu().width() / 2);
        int y = event.xbutton.y_root - (m_win.menu().height() / 2);

        if (x < 0)
            x = 0;
        else if (x + m_win.menu().width() > m_win.screen().width())
            x = m_win.screen().width() - m_win.menu().width();

        if (y < 0)
            y = 0;
        else if (y + m_win.menu().height() > m_win.screen().height())
            y = m_win.screen().height() - m_win.menu().height();

        m_win.menu().move(x, y);
        m_win.menu().show();        
    }
private:
    FluxboxWindow &m_win;
};

} // end anonymous namespace

IconButton::IconButton(const FbTk::FbWindow &parent, const FbTk::Font &font,
                       FluxboxWindow &win):
    FbTk::TextButton(parent, font, win.winClient().title()),
    m_win(win), 
    m_icon_window(*this, 1, 1, 1, 1, 
                  ExposureMask | ButtonPressMask | ButtonReleaseMask) {

    FbTk::RefCount<FbTk::Command> focus(new FbTk::SimpleCommand<FluxboxWindow>(m_win, &FluxboxWindow::raiseAndFocus));
    FbTk::RefCount<FbTk::Command> menu(new ::ShowMenu(m_win));
    setOnClick(focus, 1);
    setOnClick(menu, 3);
    m_win.hintSig().attach(this);
    
    FbTk::EventManager::instance()->add(*this, m_icon_window);
    
    update(0);
}

IconButton::~IconButton() {

}

void IconButton::exposeEvent(XExposeEvent &event) {
    if (m_icon_window == event.window)
        m_icon_window.clear();
    else
        FbTk::Button::exposeEvent(event);
}
void IconButton::moveResize(int x, int y,
                            unsigned int width, unsigned int height) {

    FbTk::Button::moveResize(x, y, width, height);

    if (m_icon_window.width() != FbTk::Button::width() ||
        m_icon_window.height() != FbTk::Button::height())
        update(0); // update icon window
}

void IconButton::resize(unsigned int width, unsigned int height) {
    FbTk::Button::resize(width, height);
    if (m_icon_window.width() != FbTk::Button::width() ||
        m_icon_window.height() != FbTk::Button::height())
        update(0); // update icon window
}

void IconButton::clear() {
    FbTk::Button::clear();
    setupWindow();
}

void IconButton::update(FbTk::Subject *subj) {
    // we got signal that either title or 
    // icon pixmap was updated, 
    // so we refresh everything

    // we need to check our client first
    if (m_win.clientList().size() == 0)
        return;

    XWMHints *hints = XGetWMHints(FbTk::App::instance()->display(), m_win.winClient().window());
    if (hints == 0)
        return;

    if (hints->flags & IconPixmapHint && hints->icon_pixmap != 0) {
        // setup icon window
        m_icon_window.show();
        int new_height = height() - m_icon_window.y();
        int new_width = height();
        m_icon_window.resize(new_width ? new_width : 1, new_height ? new_height : 1);

        m_icon_pixmap.copy(hints->icon_pixmap);
        m_icon_pixmap.scale(m_icon_window.height(), m_icon_window.height());

        m_icon_window.setBackgroundPixmap(m_icon_pixmap.drawable());
    } else {
        // no icon pixmap
        m_icon_window.move(0, 0);
        m_icon_window.hide();
        m_icon_pixmap = 0;
    }

    if(hints->flags & IconMaskHint) {
        m_icon_mask.copy(hints->icon_mask);
        m_icon_mask.scale(m_icon_pixmap.width(), m_icon_pixmap.height());
    } else
        m_icon_mask = 0;

    XFree(hints);
    hints = 0;

#ifdef SHAPE

    if (m_icon_mask.drawable() != 0) {
        XShapeCombineMask(FbTk::App::instance()->display(),
                          m_icon_window.drawable(),
                          ShapeBounding,
                          0, 0,
                          m_icon_mask.drawable(),
                          ShapeSet);
    }

#endif // SHAPE

    setupWindow();
}

void IconButton::setupWindow() {
    
    m_icon_window.clear();

    if (m_win.clientList().size() == 0)
        return;

    setText(m_win.winClient().title());
    // draw with x offset and y offset
    drawText(m_icon_window.x() + m_icon_window.width() + 1);
}

