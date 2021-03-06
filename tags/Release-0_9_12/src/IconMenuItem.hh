// IconMenuItem.hh for Fluxbox Window Manager
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

// $Id$

#ifndef ICONMENUITEM_HH
#define ICONMENUITEM_HH

#include "FbTk/MenuItem.hh"
#include "WinClient.hh"
#include "Window.hh"

class IconMenuItem: public FbTk::MenuItem {
public:
    explicit IconMenuItem(WinClient &client):FbTk::MenuItem(client.iconTitle().c_str()), m_client(client) { }
    const std::string &label() const { return m_client.iconTitle(); }
    void click(int button, int time) {
        if (m_client.fbwindow() != 0)
            m_client.fbwindow()->deiconify();
    }
private:
    WinClient &m_client;
};

#endif // ICONMENUITEM_HH
