// ButtonTool.hh for Fluxbox
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// $Id: ButtonTool.hh,v 1.2 2004/08/29 08:33:12 rathnor Exp $

#ifndef BUTTONTOOL_HH
#define BUTTONTOOL_HH

#include "GenericTool.hh"
#include "FbTk/Observer.hh"

class ButtonTheme;

namespace FbTk {
class Button;
class ImageControl;
};

class ButtonTool: public GenericTool {
public:
    ButtonTool(FbTk::Button *button, ToolbarItem::Type type, 
               ButtonTheme &theme, FbTk::ImageControl &img_ctrl);
    virtual ~ButtonTool();

protected:
    void renderTheme();
    void updateSizing();
    Pixmap m_cache_pm, m_cache_pressed_pm;
    FbTk::ImageControl &m_image_ctrl;
};

#endif // BUTTONTOOL_HH
