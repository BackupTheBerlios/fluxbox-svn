// SlitClient.cc for fluxbox
// Copyright (c) 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

// $Id: SlitClient.cc,v 1.1 2003/06/20 01:25:26 fluxgen Exp $

#include "SlitClient.hh"

#include "I18n.hh"
#include "Screen.hh"
#include "App.hh"

#include <X11/Xutil.h>
#include <X11/Xatom.h>

namespace {

void getWMName(BScreen *screen, Window window, std::string& name) {
    name = "";

    if (screen == 0 || window == None)
        return;

    Display *display = FbTk::App::instance()->display();

    XTextProperty text_prop;
    char **list;
    int num;
    I18n *i18n = I18n::instance();

    if (XGetWMName(display, window, &text_prop)) {
        if (text_prop.value && text_prop.nitems > 0) {
            if (text_prop.encoding != XA_STRING) {
				
                text_prop.nitems = strlen((char *) text_prop.value);
				
                if ((XmbTextPropertyToTextList(display, &text_prop,
                                               &list, &num) == Success) &&
                    (num > 0) && *list) {
                    name = static_cast<char *>(*list);
                    XFreeStringList(list);
                } else
                    name = (char *)text_prop.value;
					
            } else				
                name = (char *)text_prop.value;
        } else { // default name
            name = i18n->getMessage(
                                    FBNLS::WindowSet, FBNLS::WindowUnnamed,
                                    "Unnamed");
        }
    } else {
        // default name
        name = i18n->getMessage(
                                FBNLS::WindowSet, FBNLS::WindowUnnamed,
                                "Unnamed");
    }

}

}; // end anonymous namespace 

SlitClient::SlitClient(BScreen *screen, Window win) {
    initialize(screen, win);
}

SlitClient::SlitClient(const char *name):m_match_name(name == 0 ? "" : name) { 
    initialize();
}


void SlitClient::initialize(BScreen *screen, Window win) {
    // Now we pre-initialize a list of slit clients with names for
    // comparison with incoming client windows.  This allows the slit
    // to maintain a sorted order based on a saved window name list.
    // Incoming windows not found in the list are appended.  Matching
    // duplicates are inserted after the last found instance of the
    // matching name.

    m_client_window = win;
    m_window = m_icon_window = None;
    move(0, 0);
    resize(0, 0);

    if (matchName().empty())
        getWMName(screen, clientWindow(), m_match_name);
    m_visible = true;        
}

void SlitClient::disableEvents() {
    if (window() == 0)
        return;
    Display *disp = FbTk::App::instance()->display();
    XSelectInput(disp, window(), NoEventMask);
}

void SlitClient::enableEvents() {
    if (window() == 0)
        return;
    Display *disp = FbTk::App::instance()->display();
    XSelectInput(disp, window(), StructureNotifyMask |
                 SubstructureNotifyMask | EnterWindowMask);
}

void SlitClient::hide() {
    XUnmapWindow(FbTk::App::instance()->display(), window());
}

void SlitClient::show() {
    XMapWindow(FbTk::App::instance()->display(), window());
}
