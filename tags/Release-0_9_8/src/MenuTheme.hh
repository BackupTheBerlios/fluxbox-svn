// MenuTheme.hh
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

// $Id: MenuTheme.hh,v 1.1 2003/07/10 11:52:47 fluxgen Exp $

#ifndef MENUTHEME_HH
#define MENUTHEME_HH

#include "FbTk/MenuTheme.hh"
#include "Shape.hh"

/// this class extends FbTk MenuTheme and adds shape item
class MenuTheme:public FbTk::MenuTheme {
public:
    explicit MenuTheme(int screen_num);
    Shape::ShapePlace shapePlaces() const { return *m_shapeplace; }
private:
    FbTk::ThemeItem<Shape::ShapePlace> m_shapeplace;
};

#endif // MENUTHEME_HH

