// ScreensApp.cc for fluxbox 
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

// $Id: ScreensApp.cc,v 1.1.2.1 2003/10/28 21:34:52 rathnor Exp $

#include "ScreensApp.hh"

#include <string>

namespace FbTk {

ScreensApp *ScreensApp::s_screensapp = 0;

ScreensApp *ScreensApp::instance() {
    if (s_screensapp == 0)
       throw std::string("You must create an instance of FbTk::ScreensApp first!");
    return s_screensapp;
}

ScreensApp::ScreensApp(const char *displayname) :
    App(displayname),
    m_numscreens(ScreenCount(display())),
    m_active_screens(new bool[m_numscreens])
{
    if (s_screensapp != 0)
        throw std::string("Can't create more than one instance of FbTk::ScreensApp");
    s_screensapp = this;

    // no screens are active at the start
    for (int i=0; i < m_numscreens; ++i)
        m_active_screens[i] = false;

}

ScreensApp::~ScreensApp() {
    delete m_active_screens;
}

bool ScreensApp::isScreenUsed(int screen) const {
    if (screen >= 0 && screen < m_numscreens)
        return m_active_screens[screen];
    else
        return false;
}

void ScreensApp::setScreenUsed(int screen, bool value) {
    if (screen >= 0 && screen < m_numscreens)
        m_active_screens[screen] = value;

}

}; // end namespace FbTk

