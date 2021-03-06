// FbMenu.hh for Fluxbox Window Manager
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

// $Id: FbMenu.hh,v 1.1 2003/02/18 15:06:25 rathnor Exp $

#ifndef FBMENU_HH
#define FBMENU_HH

#include "Menu.hh"
#include "XLayerItem.hh"

/// a layered menu
class FbMenu:public FbTk::Menu {
public:
    FbMenu(FbTk::MenuTheme &tm, int screen_num, FbTk::ImageControl &imgctrl,
                FbTk::XLayer &layer):FbTk::Menu(tm, screen_num, imgctrl), m_layeritem(fbwindow(), layer) { }
    void raise() { m_layeritem.raise(); }
    void lower() { m_layeritem.lower(); }
private:
    FbTk::XLayerItem m_layeritem;
};

#endif // FBMENU_HH

