// WinButton.hh for Fluxbox Window Manager
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

/// $Id: WinButton.hh,v 1.4 2003/04/28 22:39:12 fluxgen Exp $

#include "Button.hh"
#include "Observer.hh"

class FluxboxWindow;
class WinButtonTheme;

/// draws and handles basic window button graphic
class WinButton:public FbTk::Button, public FbTk::Observer {
public:
    /// draw type for the button
    enum Type {MAXIMIZE, MINIMIZE, SHADE, STICK, CLOSE};
    WinButton(const FluxboxWindow &listen_to, 
              WinButtonTheme &theme,
              Type buttontype, const FbTk::FbWindow &parent, int x, int y, 
              unsigned int width, unsigned int height);
    /// override for drawing
    void exposeEvent(XExposeEvent &event);
    void buttonReleaseEvent(XButtonEvent &event);
    /// override for redrawing
    void clear();
    void update(FbTk::Subject *subj);
private:
    void drawType();
    Type m_type; ///< the button type
    const FluxboxWindow &m_listen_to;
    WinButtonTheme &m_theme;

};
