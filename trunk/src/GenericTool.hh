// GenericTool.hh for Fluxbox
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

// $Id: GenericTool.hh,v 1.3 2004/09/12 14:56:18 rathnor Exp $

#ifndef GENERICTOOL_HH
#define GENERICTOOL_HH

#include "ToolbarItem.hh"
#include "ToolTheme.hh"

#include "FbTk/NotCopyable.hh"
#include "FbTk/Observer.hh"

#include <memory>

namespace FbTk {
class FbWindow;
}

/// helper class for simple tools, i.e buttons etc
class GenericTool: public ToolbarItem, public FbTk::Observer, private FbTk::NotCopyable {
public:
    GenericTool(FbTk::FbWindow *new_window, ToolbarItem::Type type, ToolTheme &theme);
    virtual ~GenericTool();
    void move(int x, int y);
    void resize(unsigned int x, unsigned int y);
    void moveResize(int x, int y,
                    unsigned int width, unsigned int height);
    void show();
    void hide();

    unsigned int width() const;
    unsigned int height() const;
    unsigned int borderWidth() const;

    const ToolTheme &theme() const { return m_theme; }
    FbTk::FbWindow &window() { return *m_window; }
    const FbTk::FbWindow &window() const { return *m_window; }

protected:
    virtual void renderTheme(unsigned char alpha);

private:
    void update(FbTk::Subject *subj);
    
    std::auto_ptr<FbTk::FbWindow> m_window;
    ToolTheme &m_theme;
};

#endif // GENERICTOOL_HH
