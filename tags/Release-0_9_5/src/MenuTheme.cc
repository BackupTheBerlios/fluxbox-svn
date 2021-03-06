// MenuTheme.cc
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

// $Id: MenuTheme.cc,v 1.2 2003/07/10 14:16:11 fluxgen Exp $

#include "MenuTheme.hh"
#include "StringUtil.hh"
#include <iostream>
using namespace std;

template <>
void FbTk::ThemeItem<Shape::ShapePlace>::load() { }

template <>
void FbTk::ThemeItem<Shape::ShapePlace>::setDefaultValue() {
    *(*this) = Shape::NONE;
}

template <>
void FbTk::ThemeItem<Shape::ShapePlace>::setFromString(const char *str) {
    int places = 0;

    if (StringUtil::strcasestr(str, "topleft") != 0)
        places |= Shape::TOPLEFT;
    if (StringUtil::strcasestr(str, "topright") != 0)
        places |= Shape::TOPRIGHT;
    if (StringUtil::strcasestr(str, "bottomleft") != 0)
        places |= Shape::BOTTOMLEFT;
    if (StringUtil::strcasestr(str, "bottomright") != 0)
        places |= Shape::BOTTOMRIGHT;

    *(*this) = static_cast<Shape::ShapePlace>(places);
}

MenuTheme::MenuTheme(int screen_num):FbTk::MenuTheme(screen_num),
                                     m_shapeplace(*this, "menu.roundCorners", "Menu.RoundCorners") {
    *m_shapeplace = Shape::NONE;
}
