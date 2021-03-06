// TextTheme.cc
// Copyright (c) 2003 - 2005 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#include "TextTheme.hh"

#include "FbTk/App.hh"

#include <X11/Xlib.h>

TextTheme::TextTheme(FbTk::Theme &theme,
                     const std::string &name, const std::string &altname):
    m_font(theme, name + ".font", altname + ".Font"),
    m_text_color(theme, name + ".textColor", altname + ".TextColor"),
    m_justify(theme, name + ".justify", altname + ".Justify"),
    m_text_gc(RootWindow(FbTk::App::instance()->display(), theme.screenNum())) {
    *m_justify = FbTk::LEFT;
    // set default values
    m_text_color->setFromString("white", theme.screenNum());

    update();
}

TextTheme::~TextTheme() {

}

void TextTheme::update() {
    m_text_gc.setForeground(*m_text_color);
}
