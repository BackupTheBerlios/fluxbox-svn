// ButtonTool.cc for Fluxbox
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

// $Id: ButtonTool.cc,v 1.6 2004/09/12 14:56:18 rathnor Exp $

#include "ButtonTool.hh"

#include "FbTk/Button.hh"
#include "FbTk/ImageControl.hh"
#include "ButtonTheme.hh"

ButtonTool::ButtonTool(FbTk::Button *button, 
                       ToolbarItem::Type type, 
                       ButtonTheme &theme,
                       FbTk::ImageControl &img_ctrl):
    GenericTool(button, type, theme),
    m_cache_pm(0),
    m_cache_pressed_pm(0),
    m_image_ctrl(img_ctrl) {

}

ButtonTool::~ButtonTool() {
    if (m_cache_pm)
        m_image_ctrl.removeImage(m_cache_pm);

    if (m_cache_pressed_pm)
        m_image_ctrl.removeImage(m_cache_pressed_pm);

}

void ButtonTool::updateSizing() {
    FbTk::Button &btn = static_cast<FbTk::Button &>(window());
    btn.setBorderWidth(theme().border().width());
}

void ButtonTool::renderTheme(unsigned char alpha) {
    FbTk::Button &btn = static_cast<FbTk::Button &>(window());

    btn.setGC(static_cast<const ButtonTheme &>(theme()).gc());
    btn.setBorderColor(theme().border().color());
    btn.setBorderWidth(theme().border().width());
    btn.setAlpha(alpha);
    btn.updateTheme(static_cast<const FbTk::Theme &>(theme()));

    Pixmap old_pm = m_cache_pm;
    if (!theme().texture().usePixmap()) {
        m_cache_pm = 0;
        btn.setBackgroundColor(theme().texture().color());
    } else {
        m_cache_pm = m_image_ctrl.renderImage(width(), height(),
                                              theme().texture());
        btn.setBackgroundPixmap(m_cache_pm);
    }
    if (old_pm)
        m_image_ctrl.removeImage(old_pm);

    old_pm = m_cache_pressed_pm;
    if (! static_cast<const ButtonTheme &>(theme()).pressed().usePixmap()) {
        m_cache_pressed_pm = 0;
        btn.setPressedColor(static_cast<const ButtonTheme &>(theme()).pressed().color());
    } else {
        m_cache_pressed_pm = m_image_ctrl.renderImage(width(), height(),
                                                      static_cast<const ButtonTheme &>(theme()).pressed());
        btn.setPressedPixmap(m_cache_pressed_pm);
    }

    if (old_pm)
        m_image_ctrl.removeImage(old_pm);

    btn.clear();
    btn.updateTransparent();
}

