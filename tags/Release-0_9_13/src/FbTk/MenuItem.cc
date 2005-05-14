// MenuItem.cc for FbTk - Fluxbox Toolkit
// Copyright (c) 2003 - 2005 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

// $Id$

#include "MenuItem.hh"
#include "Command.hh"
#include "GContext.hh"
#include "MenuTheme.hh"
#include "PixmapWithMask.hh"
#include "Image.hh"
#include "App.hh"
#include "StringUtil.hh"

namespace FbTk {

void MenuItem::click(int button, int time) {
    if (m_command.get() != 0)
        m_command->execute();
}

void MenuItem::draw(FbDrawable &draw,
                    const MenuTheme &theme,
                    bool highlight, bool draw_foreground, bool draw_background,
                    int x, int y,
                    unsigned int width, unsigned int height) const {

    // text and submenu icon are background
    // selected pixmaps are foreground

    Display *disp = App::instance()->display();
    //
    // Icon
    //
    if (draw_background) {
        if (m_icon.get() != 0 && m_icon->pixmap.get() != 0) {
            // scale pixmap to right size
            if (height - 2*theme.bevelWidth() != m_icon->pixmap->height() &&
                !m_icon->filename.empty()) {
                unsigned int scale_size = height - 2*theme.bevelWidth();
                m_icon->pixmap->scale(scale_size, scale_size);
            }

            if (m_icon->pixmap->pixmap().drawable() != 0) {
                GC gc = theme.frameTextGC().gc();
                int icon_x = x + theme.bevelWidth();
                int icon_y = y + theme.bevelWidth();
                // enable clip mask
                XSetClipMask(disp, gc, m_icon->pixmap->mask().drawable());
                XSetClipOrigin(disp, gc, icon_x, icon_y);

                draw.copyArea(m_icon->pixmap->pixmap().drawable(),
                              gc,
                              0, 0,
                              icon_x, icon_y,
                              m_icon->pixmap->width(), m_icon->pixmap->height());

                // restore clip mask
                XSetClipMask(disp, gc, None);
            }
        }
    }

    if (label().empty())
        return;

    // text is background
    if (draw_background) {
        const GContext &tgc =
            (highlight ? theme.hiliteTextGC() :
             (isEnabled() ? theme.frameTextGC() : theme.disableTextGC() ) );
        //
        // Text
        //
        int text_y = y, text_x = x;

        int text_w = theme.frameFont().textWidth(label().c_str(), label().size());

        int height_offset = theme.itemHeight() - (theme.frameFont().height() + 2*theme.bevelWidth());
        text_y = y + theme.bevelWidth() + theme.frameFont().ascent() + height_offset/2;

        switch(theme.frameFontJustify()) {
        case FbTk::LEFT:
            text_x = x + theme.bevelWidth() + height + 1;
            break;

        case FbTk::RIGHT:
            text_x = x +  width - (height + theme.bevelWidth() + text_w);
            break;
        default: //center
            text_x = x + ((width + 1 - text_w) / 2);
            break;
        }

        theme.frameFont().drawText(draw, // drawable
                                   theme.screenNum(),
                                   tgc.gc(),
                                   m_label.c_str(), m_label.size(), // text string and lenght
                                   text_x, text_y); // position
    }

    GC gc = (highlight) ? theme.hiliteTextGC().gc() :
        theme.frameTextGC().gc();
    int sel_x = x;
    int sel_y = y;
    unsigned int item_pm_height = theme.itemHeight();

    if (theme.bulletPos() == FbTk::RIGHT)
        sel_x += width - height - theme.bevelWidth();

    // selected pixmap is foreground
    if (draw_foreground && isToggleItem()) {

        //
        // ToggleItem
        //
        const PixmapWithMask *pm = 0;

        if (isSelected()) {
            if (highlight && theme.highlightSelectedPixmap().pixmap().drawable() != 0)
                pm = &theme.highlightSelectedPixmap();
            else
                pm = &theme.selectedPixmap();
        } else {
            if (highlight && theme.highlightUnselectedPixmap().pixmap().drawable() != 0)
                pm = &theme.highlightUnselectedPixmap();
            else
                pm = &theme.unselectedPixmap();
        }
        if (pm != 0 && pm->pixmap().drawable() != 0) {
            unsigned int selw = pm->width();
            unsigned int selh = pm->height();
            int offset_x = 0;
            int offset_y = 0;
            if (selw < item_pm_height)
                offset_x += (item_pm_height - selw) / 2;
            if (selh < item_pm_height)
                offset_y += (item_pm_height - selh) / 2;

            XSetClipMask(disp, gc, pm->mask().drawable());
            XSetClipOrigin(disp, gc, sel_x+offset_x, sel_y+offset_y);
            // copy bullet pixmap to drawable
            draw.copyArea(pm->pixmap().drawable(),
                          gc,
                          0, 0,
                          sel_x+offset_x, sel_y+offset_y,
                          selw,
                          selh);
            // disable clip mask
            XSetClipMask(disp, gc, None);
        } else if (isSelected()) {
                draw.fillRectangle(theme.hiliteGC().gc(),
                                   sel_x+item_pm_height/4, sel_y+item_pm_height/4, item_pm_height/2, item_pm_height/2);
        }
    }

    //
    // Submenu (background)
    //
    if (draw_background && submenu()) {

        const PixmapWithMask *pm = 0;

        if (highlight && theme.highlightBulletPixmap().pixmap().drawable() != 0)
            pm = &theme.highlightBulletPixmap();
        else
            pm = &theme.bulletPixmap();

        if (pm && pm->pixmap().drawable() != 0) {
            unsigned int selw = pm->width();
            unsigned int selh = pm->height();

            int offset_x = 0;
            int offset_y = 0;
            if (selw < item_pm_height)
                offset_x += (item_pm_height - selw) / 2;
            if (selh < item_pm_height)
                offset_y += (item_pm_height - selh) / 2;

            XSetClipMask(disp, gc, pm->mask().drawable());
            XSetClipOrigin(disp, gc, sel_x+offset_x, sel_y+offset_y);
            // copy bullet pixmap to drawable
            draw.copyArea(pm->pixmap().drawable(),
                          gc,
                          0, 0,
                          sel_x+offset_x, sel_y+offset_y,
                          selw,
                          selh);
            // disable clip mask
            XSetClipMask(disp, gc, None);

        } else {
            unsigned int half_w = item_pm_height / 2, quarter_w = item_pm_height / 4;
            switch (theme.bullet()) {
            case MenuTheme::SQUARE:
                draw.drawRectangle(gc, sel_x+quarter_w, y+quarter_w, half_w, half_w);
                break;

            case MenuTheme::TRIANGLE:
                XPoint tri[3];

                if (theme.bulletPos() == FbTk::RIGHT) {
                    tri[0].x = sel_x + half_w - 2;
                    tri[0].y = sel_y + half_w - 2;
                    tri[1].x = 4;
                    tri[1].y = 2;
                    tri[2].x = -4;
                    tri[2].y = 2;
                } else { // point the other way
                    tri[0].x = sel_x + half_w - 2;
                    tri[0].y = sel_y + half_w;
                    tri[1].x = 4;
                    tri[1].y = 2;
                    tri[2].x = 0;
                    tri[2].y = -4;
                }

                draw.fillPolygon(gc, tri, 3, Convex,
                                 CoordModePrevious);
                break;

            case MenuTheme::DIAMOND:
                XPoint dia[4];

                dia[0].x = sel_x + half_w - 3;
                dia[0].y = sel_y + half_w;
                dia[1].x = 3;
                dia[1].y = -3;
                dia[2].x = 3;
                dia[2].y = 3;
                dia[3].x = -3;
                dia[3].y = 3;

                draw.fillPolygon(gc, dia, 4, Convex,
                                 CoordModePrevious);
                break;
            default:
                break;
            }
        }
    }


}

void MenuItem::setIcon(const std::string &filename, int screen_num) {
    if (filename.empty()) {
        if (m_icon.get() != 0)
            m_icon.reset(0);
        return;
    }

    if (m_icon.get() == 0)
        m_icon.reset(new Icon);

    m_icon->filename = FbTk::StringUtil::expandFilename(filename);
    m_icon->pixmap.reset(Image::load(m_icon->filename.c_str(),
                         screen_num));
}

unsigned int MenuItem::height(const MenuTheme &theme) const {
    return std::max(theme.frameFont().height() + 2*theme.bevelWidth(), theme.itemHeight());
}

unsigned int MenuItem::width(const MenuTheme &theme) const {
    // textwidth + bevel width on each side of the text
    const unsigned int icon_width = height(theme);
    const unsigned int normal = theme.frameFont().textWidth(label().c_str(), label().size()) +
                                2 * (theme.bevelWidth() + icon_width);
    return m_icon.get() == 0 ? normal : normal + icon_width;
}

void MenuItem::updateTheme(const MenuTheme &theme) {
    if (m_icon.get() == 0)
        return;

    m_icon->pixmap.reset(Image::load(m_icon->filename.c_str(), theme.screenNum()));


}

}; // end namespace FbTk
