// Font.cc for FbTk
// Copyright (c) 2002-2004 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

//$Id$

#ifndef FBTK_FONT_HH
#define FBTK_FONT_HH

#include <X11/Xlib.h>
#include <X11/Xresource.h>

#include <string>
#include <memory>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef HAVE_ICONV
#include <iconv.h>
#endif // HAVE_ICONV

namespace FbTk {

class FontImp;
class FbDrawable;

/**
   Handles the client to fontimp bridge.
*/
class Font {
public:

    /// called at FbTk::App creation time, initializes some stuff
    static void init();

    /// called at FbTk::App destruction time, cleans up what was inited first
    static void shutdown();
    
    /// @return true if multibyte is enabled, else false
    static bool multibyte() { return m_multibyte; }
    /// @return true if utf-8 mode is enabled, else false
    static bool utf8() { return m_utf8mode; }



    Font(const char *name=0, bool antialias = false);
    virtual ~Font();
    /** 
        Load a font
        @return true on success, else false and it'll fall back on the last
        loaded font
    */
    bool load(const std::string &name);

    void setAntialias(bool flag);
    inline void setShadow(bool flag) { m_shadow = flag; if (m_shadow) setHalo(false); }
    inline void setHalo(bool flag)   { m_halo = flag; if (m_halo) setShadow(false); }
    /**
       @param text text to check size
       @param size length of text in bytes
       @return size of text in pixels
    */
    unsigned int textWidth(const char * const text, unsigned int size) const;
    unsigned int height() const;	
    int ascent() const;
    int descent() const;
    /**
       Rotate font in any angle 
       (currently only 90 degrees supported and just XFont implementation)
    */
    void rotate(float angle);

    /**
       Draws text to drawable
       @param w the drawable
       @param screen screen number
       @param gc Graphic Context
       @param text the text buffer
       @param len size of text buffer
       @param x position
       @param y position
       @param rotate if the text should be drawn rotated (if it's rotated before)
    */	
    void drawText(const FbDrawable &w, int screen, GC gc, 
                  const char *text, size_t len, 
                  int x, int y, bool rotate=true) const;
    bool isAntialias() const { return m_antialias; }
    /// @return true if the font is rotated, else false
    bool isRotated() const { return m_rotated; }
    /// @return rotated angle
    float angle() const { return m_angle; }
    bool shadow() const { return m_shadow; }
    bool halo() const { return m_halo; }
private:
	
    std::auto_ptr<FontImp> m_fontimp; ///< font implementation
    std::string m_fontstr; ///< font name
    static bool m_multibyte; ///< if the fontimp should be a multibyte font
    static bool m_utf8mode; ///< should the font use utf8 font imp
    bool m_antialias; ///< is font antialias
    bool m_rotated; ///< wheter we're rotated or not
    float m_angle; ///< rotation angle
    bool m_shadow; ///< shadow text
    std::string m_shadow_color; ///< shadow color
    int m_shadow_offx; ///< offset y for shadow
    int m_shadow_offy; ///< offset x for shadow
    bool m_halo; ///< halo text
    std::string m_halo_color; ///< halo color
#ifdef HAVE_ICONV
    iconv_t m_iconv;
#else
    int m_iconv;
#endif // HAVE_ICONV
};

} //end namespace FbTk

#endif //FBTK_FONT_HH