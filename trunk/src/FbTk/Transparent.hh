// Transparent.hh for FbTk - Fluxbox Toolkit
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

// $Id: Transparent.hh,v 1.1 2003/04/20 13:27:16 fluxgen Exp $

#ifndef FBTK_TRANSPARENT_HH
#define FBTK_TRANSPARENT_HH

#include <X11/Xlib.h>

namespace FbTk {

/// renders to drawable together with an alpha mask
class Transparent {
public:
    Transparent(Drawable source, Drawable dest, unsigned char alpha, int screen_num);
    ~Transparent();
    /// sets alpha value
    void setAlpha(unsigned char alpha);
    /// sets source drawable
    void setSource(Drawable src, int screen_num);
    /// sets destination drawable
    void setDest(Drawable dest, int screen_num);
    /**
       renders to dest from src with specified coordinates and size
    */
    void render(int src_x, int src_y,
                int dest_x, int dest_y,
                unsigned int width, unsigned int height) const;
    unsigned char alpha() const { return m_alpha; }
private:
    void freeAlpha();
    void allocAlpha(unsigned char newval);
    unsigned long m_alpha_pic;
    unsigned long m_src_pic;
    unsigned long m_dest_pic;
    Drawable m_source, m_dest;
    unsigned char m_alpha;
};

}; // end namespace  FbTk

#endif // FBTK_TRANSPARENT_HH

