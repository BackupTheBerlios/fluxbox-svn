// WorkspaceNameTool.cc
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

// $Id: WorkspaceNameTool.cc,v 1.10 2004/08/29 08:33:13 rathnor Exp $

#include "WorkspaceNameTool.hh"

#include "ToolTheme.hh"
#include "Screen.hh"
#include "Workspace.hh"

#include "FbTk/ImageControl.hh"

WorkspaceNameTool::WorkspaceNameTool(const FbTk::FbWindow &parent, 
                                     ToolTheme &theme, BScreen &screen):
    ToolbarItem(ToolbarItem::FIXED),
    m_button(parent, theme.font(), "a workspace name"),
    m_theme(theme),
    m_screen(screen),
    m_pixmap(0) {
    // set text
    m_button.setGC(m_theme.textGC());
    m_button.setText(m_screen.currentWorkspace()->name());

    // setup signals
    screen.workspaceNamesSig().attach(this);
    screen.currentWorkspaceSig().attach(this);
    theme.reconfigSig().attach(this);

    renderTheme();
}

WorkspaceNameTool::~WorkspaceNameTool() {
    if (m_pixmap)
        m_screen.imageControl().removeImage(m_pixmap);
}

void WorkspaceNameTool::move(int x, int y) {
    m_button.move(x, y);
}

void WorkspaceNameTool::resize(unsigned int width, unsigned int height) {
    m_button.resize(width, height);
}


void WorkspaceNameTool::moveResize(int x, int y,
                                   unsigned int width, unsigned int height) {
    m_button.moveResize(x, y, width, height);
}

void WorkspaceNameTool::update(FbTk::Subject *subj) {
    m_button.setText(m_screen.currentWorkspace()->name());
    if (m_button.width() != width()) {
        resize(width(), height());
        resizeSig().notify();
    }
    renderTheme();
}

unsigned int WorkspaceNameTool::width() const {
    // calculate largest size
    int max_size = 0;
    const int num_workspaces = m_screen.getNumberOfWorkspaces();
    for (int workspace = 0; workspace < num_workspaces; ++workspace) {
        const std::string &name = m_screen.getWorkspace(workspace)->name().c_str();
        int size = m_theme.font().textWidth(name.c_str(), name.size());
        if (size > max_size)
            max_size = size;
    }
    // so align text dont cut the last character
    max_size += 2;

    return max_size;
}

unsigned int WorkspaceNameTool::height() const {
    return m_button.height();
}

unsigned int WorkspaceNameTool::borderWidth() const {
    return m_button.borderWidth();
}

void WorkspaceNameTool::show() {
    m_button.show();
}

void WorkspaceNameTool::hide() {
    m_button.hide();
}

void WorkspaceNameTool::updateSizing() {
    m_button.setBorderWidth(m_theme.border().width());
}

void WorkspaceNameTool::renderTheme() {
    Pixmap tmp = m_pixmap;
    if (!m_theme.texture().usePixmap()) {
        m_pixmap = 0;
        m_button.setBackgroundColor(m_theme.texture().color());
    } else {
        m_pixmap = m_screen.imageControl().renderImage(width(), height(),
                                                       m_theme.texture());
        m_button.setBackgroundPixmap(m_pixmap);
    }
    if (tmp)
        m_screen.imageControl().removeImage(tmp);

    m_button.setJustify(m_theme.justify());
    m_button.setBorderWidth(m_theme.border().width());
    m_button.setBorderColor(m_theme.border().color());
    m_button.setAlpha(m_theme.alpha());
    m_button.clear();
}
