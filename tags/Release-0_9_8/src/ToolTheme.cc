// ToolTheme.cc
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

// $Id: ToolTheme.cc,v 1.5 2004/01/13 14:41:32 rathnor Exp $

#include "ToolTheme.hh"

ToolTheme::ToolTheme(int screen_num, const std::string &name, const std::string &altname):
    FbTk::Theme(screen_num),
    TextTheme(*this, name, altname),
    m_texture(*this, name, altname),
    m_border(*this, name, altname),
    m_alpha(*this, name+".alpha", altname+".Alpha") {

}

ToolTheme::~ToolTheme() {

}

void ToolTheme::reconfigTheme() { 
    // update text theme
    update();
}

bool ToolTheme::fallback(FbTk::ThemeItem_base &item) {
    if (item.name().find(".justify") != std::string::npos) {
        return FbTk::ThemeManager::instance().loadItem(item, 
                                                       "toolbar.justify",
                                                       "Toolbar.Justify");
    } else if (item.name().find(".alpha") != std::string::npos) {
        return FbTk::ThemeManager::instance().loadItem(item, 
                                                       "toolbar.alpha",
                                                       "Toolbar.Alpha");
    }

    return false;
}
