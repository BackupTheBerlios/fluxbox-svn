// ArrowButton.hh for Fluxbox Window Manager
// Copyright (c) 2002-2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

// $Id: ArrowButton.hh,v 1.2 2003/04/27 01:54:18 fluxgen Exp $

#ifndef ARROWBUTTON_HH
#define ARROWBUTTON_HH

#include "Button.hh"

/// Displays a arrow on a button
class ArrowButton: public FbTk::Button {
public:
    enum Type { LEFT, RIGHT, UP, DOWN};

    ArrowButton(Type arrow_type, FbTk::FbWindow &parent,
                int x, int y, 
                unsigned int width, unsigned int height);
    ArrowButton(Type arrow_type, int screen_num,
                int x, int y,
                unsigned int width, unsigned int height);
    void clear();
    void buttonReleaseEvent(XButtonEvent &event);
    void buttonPressEvent(XButtonEvent &event);
    void exposeEvent(XExposeEvent &event);

private:
    void drawArrow();
    Type m_arrow_type;                        
};

#endif // ARROWBUTTON_HH
