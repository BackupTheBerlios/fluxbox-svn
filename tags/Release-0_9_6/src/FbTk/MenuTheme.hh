// MenuTheme.hh for FbTk
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// $Id: MenuTheme.hh,v 1.9 2003/09/12 23:32:02 fluxgen Exp $

#ifndef FBTK_MENUTHEME_HH
#define FBTK_MENUTHEME_HH

#include "Theme.hh"
#include "Color.hh"
#include "Font.hh"
#include "Texture.hh"
#include "Text.hh"
#include "Subject.hh"
#include "PixmapWithMask.hh"
#include "GContext.hh"

namespace FbTk {

class MenuTheme:public FbTk::Theme {
public:
    enum BulletType { EMPTY, SQUARE, TRIANGLE, DIAMOND};
    MenuTheme(int screen_num);
    virtual ~MenuTheme();

    void reconfigTheme();

    bool fallback(ThemeItem_base &item);

    /**
       @name text colors
    */
    ///@{
    const FbTk::Color &titleTextColor() const { return *t_text; }
    const FbTk::Color &frameTextColor() const { return *f_text; }
    const FbTk::Color &highlightTextColor() const { return *h_text; }
    const FbTk::Color &disableTextColor() const { return *d_text; }
    ///@}
    /**
       @name textures
    */
    ///@{
    const FbTk::Texture &titleTexture() const { return *title; }
    const FbTk::Texture &frameTexture() const { return *frame; }
    const FbTk::Texture &hiliteTexture() const { return *hilite; }
    ///@}

    const FbTk::PixmapWithMask &bulletPixmap() const { return *m_bullet_pixmap; }
    const FbTk::PixmapWithMask &selectedPixmap() const { return *m_selected_pixmap; }
    const FbTk::PixmapWithMask &unselectedPixmap() const { return *m_unselected_pixmap; }
    /**
       @name fonts
    */
    ///@{
    const FbTk::Font &titleFont() const { return *titlefont; }
    FbTk::Font &titleFont() { return *titlefont; }
    const FbTk::Font &frameFont() const { return *framefont; }
    FbTk::Font &frameFont() { return *framefont; }
    ///@}

    FbTk::Justify frameFontJustify() const { return *framefont_justify; }
    FbTk::Justify titleFontJustify() const { return *titlefont_justify; }
	
    /**
       @name graphic contexts
    */
    ///@{
    GC titleTextGC() const { return t_text_gc.gc(); }
    GC frameTextGC() const { return f_text_gc.gc(); }
    GC hiliteTextGC() const { return h_text_gc.gc(); }
    GC disableTextGC() const { return d_text_gc.gc(); }
    GC hiliteGC() const { return hilite_gc.gc(); }
    ///@}
    BulletType bullet() const { return *m_bullet; }
    FbTk::Justify bulletPos() const { return *bullet_pos; }

    unsigned int borderWidth() const { return *m_border_width; }
    unsigned int bevelWidth() const { return *m_bevel_width; }

    inline unsigned char alpha() const { return m_alpha; }
    void setAlpha(unsigned char alpha) { m_alpha = alpha; }

    const FbTk::Color &borderColor() const { return *m_border_color; }
    FbTk::Subject &themeChangeSig() { return m_theme_change_sig; }
    /// attach observer
    void addListener(FbTk::Observer &obs) { m_theme_change_sig.attach(&obs); }
    /// detach observer
    void removeListener(FbTk::Observer &obs) { m_theme_change_sig.detach(&obs); }
private:
    FbTk::ThemeItem<FbTk::Color> t_text, f_text, h_text, d_text;
    FbTk::ThemeItem<FbTk::Texture> title, frame, hilite;
    FbTk::ThemeItem<FbTk::Font> titlefont, framefont;
    FbTk::ThemeItem<FbTk::Justify> framefont_justify, titlefont_justify;
    FbTk::ThemeItem<FbTk::Justify> bullet_pos; 
    FbTk::ThemeItem<BulletType> m_bullet;
    FbTk::ThemeItem<unsigned int> m_border_width;
    FbTk::ThemeItem<unsigned int> m_bevel_width;
    FbTk::ThemeItem<FbTk::Color> m_border_color;
    FbTk::ThemeItem<FbTk::PixmapWithMask> m_bullet_pixmap, m_selected_pixmap, m_unselected_pixmap;

    Display *m_display;
    FbTk::GContext t_text_gc, f_text_gc, h_text_gc, d_text_gc, hilite_gc;
    FbTk::Subject m_theme_change_sig;

    unsigned char m_alpha;
};

}; // end namespace FbTk

#endif // FBTK_MENUTHEME_HH
