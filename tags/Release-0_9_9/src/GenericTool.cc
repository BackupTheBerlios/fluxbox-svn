// GenericTool.cc for Fluxbox
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

// $Id: GenericTool.cc,v 1.3 2004/01/13 14:41:32 rathnor Exp $

#include "GenericTool.hh"
#include "FbTk/FbWindow.hh"

#include <string>

GenericTool::GenericTool(FbTk::FbWindow *new_window, ToolbarItem::Type type,
                         ToolTheme &theme):
    ToolbarItem(type),
    m_window(new_window),
    m_theme(theme) {

    theme.reconfigSig().attach(this);

    if (new_window == 0)
        throw std::string("GenericTool: Error! Tried to create a tool with window = 0");
}

GenericTool::~GenericTool() {

}

void GenericTool::move(int x, int y) {
    m_window->move(x, y);
}

void GenericTool::resize(unsigned int width, unsigned int height) {
    m_window->resize(width, height);
}

void GenericTool::moveResize(int x, int y,
                             unsigned int width, unsigned int height) {
    m_window->moveResize(x, y, width, height);
}

void GenericTool::show() {
    m_window->show();
}

void GenericTool::hide() {
    m_window->hide();
}

unsigned int GenericTool::width() const {
    return m_window->width();
}

unsigned int GenericTool::height() const {
    return m_window->height();

}

unsigned int GenericTool::borderWidth() const {
    return m_window->borderWidth();
}


void GenericTool::renderTheme() {
    m_window->setAlpha(theme().alpha());
    m_window->clear();
    m_window->updateTransparent();
}

void GenericTool::update(FbTk::Subject *subj) {
    renderTheme();
}
