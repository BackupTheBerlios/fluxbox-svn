// GContext.cc for FbTk - fluxbox toolkit
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

// $Id: GContext.cc,v 1.4 2003/10/09 16:48:09 rathnor Exp $

#include "GContext.hh"

#include "App.hh"
#include "FbDrawable.hh"
#include "FbPixmap.hh"
#include "Color.hh"
#include "Font.hh"

namespace FbTk {

GContext::GContext(const FbTk::FbDrawable &drawable): 
    m_display(FbTk::App::instance()->display()),     
    m_gc(XCreateGC(m_display,
                   drawable.drawable(),
                   0, 0)) {
    setGraphicsExposure(false);
}

GContext::GContext(Drawable drawable):
    m_display(FbTk::App::instance()->display()), 
    m_gc(XCreateGC(m_display,
                   drawable,
                   0, 0))
{
    setGraphicsExposure(false);
}

GContext::~GContext() {
    if (m_gc)
        XFreeGC(m_display, m_gc);
}

/// not implemented!
//void GContext::setFont(const FbTk::Font &font) {
    //!! TODO
//}

} // end namespace FbTk
