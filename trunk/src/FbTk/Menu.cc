// Menu.cc for FbTk - Fluxbox Toolkit 
// Copyright (c) 2001 - 2002 Henrik Kinnunen (fluxgen at users.sourceforge.net)
//
// Basemenu.cc for blackbox - an X11 Window manager
// Copyright (c) 1997 - 2000 Brad Hughes (bhughes at tcac.net)
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

// $Id: Menu.cc,v 1.2 2003/01/07 02:10:24 fluxgen Exp $

//use GNU extensions
#ifndef	 _GNU_SOURCE
#define	 _GNU_SOURCE
#endif // _GNU_SOURCE

#include "Menu.hh"

#include "../ImageControl.hh"
#include "MenuTheme.hh"
#include "App.hh"
#include "EventManager.hh"

#include <cstdio>
#include <cstdlib>
#include <cstring>


#ifdef DEBUG
#include <iostream>
using namespace std;
#endif //DEBUG

namespace FbTk {

static Menu *shown = 0;

Menu::Menu(MenuTheme &tm, int screen_num, BImageControl &imgctrl):
    m_theme(tm),
    m_screen_num(screen_num),
    m_image_ctrl(imgctrl),
    m_display(FbTk::App::instance()->display()),
    m_parent(0),
    m_screen_width(DisplayWidth(m_display, screen_num)),
    m_screen_height(DisplayHeight(m_display, screen_num)),
    m_alignment(ALIGNDONTCARE),
    m_border_width(0) {

    title_vis =
        movable =
        hide_tree = true;

    shifted =
        internal_menu =
        moving =
        torn =
        visible = false;

    menu.x =
        menu.y =
        menu.x_shift =
        menu.y_shift =
        menu.x_move =
        menu.y_move = 0;

    which_sub =
        which_press =
        which_sbl = -1;

    menu.frame_pixmap =
        menu.title_pixmap =
        menu.hilite_pixmap =
        menu.sel_pixmap = None;

    menu.bevel_w = 2;

    menu.width = menu.title_h = menu.item_w = menu.frame_h =
        m_theme.titleFont().height() + menu.bevel_w * 2;

    menu.sublevels =
        menu.persub =
        menu.minsub = 0;

    menu.item_h = m_theme.frameFont().height() + menu.bevel_w;

    menu.height = menu.title_h + 2 + menu.frame_h;
	
    //set attributes for menu window
    unsigned long attrib_mask = CWOverrideRedirect | CWEventMask;
    XSetWindowAttributes attrib;
    attrib.override_redirect = True;
    attrib.event_mask = ButtonPressMask | ButtonReleaseMask | ButtonMotionMask | ExposureMask;
#ifdef DEBUG
    cerr<<__FILE__<<": Creating menu("<<menu.width<<", "<<menu.height<<")"<<endl;
#endif // DEBUG    
    //create menu window
    menu.window = XCreateWindow(m_display, RootWindow(m_display, screen_num), 
                                menu.x, menu.y, menu.width, menu.height,  // pos and size
                                0, CopyFromParent,
                                InputOutput, CopyFromParent, attrib_mask, &attrib);

    FbTk::EventManager &evm = *FbTk::EventManager::instance();
    evm.add(*this, menu.window);
	
    //attibutes for title to menuwindow
    attrib_mask = CWEventMask;
    attrib.event_mask |= EnterWindowMask | LeaveWindowMask;
    //create menu title
    menu.title =
        XCreateWindow(m_display, menu.window.window(), 0, 0, menu.width, menu.height, 0,
                      CopyFromParent, InputOutput, CopyFromParent,
                      attrib_mask, &attrib);
    evm.add(*this, menu.title);

    attrib.event_mask |= PointerMotionMask;
    menu.frame = XCreateWindow(m_display, menu.window.window(), 0,
                               menu.title_h,
                               menu.width, menu.frame_h, 0,
                               CopyFromParent, InputOutput,
                               CopyFromParent, attrib_mask, &attrib);
    evm.add(*this, menu.frame);
}


Menu::~Menu() {
    menu.window.hide();
   
    if (shown && shown->windowID() == windowID())
        shown = (Menu *) 0;

    //TODO: this looks kind of strange
    int n = menuitems.size() - 1;
    for (int i = 0; i < n; ++i)
        remove(0);

    if (menu.title_pixmap)
        m_image_ctrl.removeImage(menu.title_pixmap);

    if (menu.frame_pixmap)
        m_image_ctrl.removeImage(menu.frame_pixmap);

    if (menu.hilite_pixmap)
        m_image_ctrl.removeImage(menu.hilite_pixmap);

    if (menu.sel_pixmap)
        m_image_ctrl.removeImage(menu.sel_pixmap);

    FbTk::EventManager &evm = *FbTk::EventManager::instance();
    evm.remove(menu.title);
    evm.remove(menu.frame);
    evm.remove(menu.window);
   
}

int Menu::insert(const char *label, RefCount<Command> &cmd, int pos) {
    MenuItem *item = new MenuItem(label, cmd);
    if (pos < 0)
        menuitems.push_back(item);
    else {
        if ( pos > menuitems.size())
            pos = menuitems.size();
        menuitems.insert(menuitems.begin() + pos, item);
    }
}

int Menu::insert(const char *label, int function, const char *exec, int pos) {
    MenuItem *item = new MenuItem(label, function, exec);
    if (pos == -1) {
        menuitems.push_back(item);
    } else {
        menuitems.insert(menuitems.begin() + pos, item);
    }

    return menuitems.size();
}


int Menu::insert(const char *label, Menu *submenu, int pos) {
    MenuItem *item = new MenuItem(label, submenu);
    if (pos == -1) {
        menuitems.push_back(item);
    } else {
        menuitems.insert(menuitems.begin() + pos, item);
    }

    submenu->m_parent = this;

    return menuitems.size();
}

int Menu::remove(unsigned int index) {
    if (index >= menuitems.size()) {
#ifdef DEBUG
        std::cout << "Bad index (" << index << ") given to Menu::remove()"
                  << " -- should be between 0 and " << menuitems.size()
                  << " inclusive." << std::endl;
#endif // DEBUG
        return -1;
    }

    Menuitems::iterator it = menuitems.begin() + index;
    MenuItem *item = (*it);

    if (item) {
        menuitems.erase(it);
        if ((! internal_menu) && (item->submenu())) {
            Menu *tmp = (Menu *) item->submenu();

            if (! tmp->internal_menu) {
                delete tmp;				
            } else
                tmp->internal_hide();
        }
		
        delete item;
    }

    if (static_cast<unsigned int>(which_sub) == index)
        which_sub = -1;
    else if (static_cast<unsigned int>(which_sub) > index)
        which_sub--;

    return menuitems.size();
}

void Menu::removeAll() {
    while (!menuitems.empty()) {
        delete menuitems.back();
        menuitems.pop_back();
    }
}

void Menu::raise() {
    menu.window.raise();
}

void Menu::lower() {
    menu.window.lower();
}

void Menu::disableTitle() {
    setTitleVisibility(false);
}

void Menu::enableTitle() {
    setTitleVisibility(true);
}

void Menu::update() {
 
    menu.item_h = m_theme.frameFont().height() + menu.bevel_w;
    menu.title_h = m_theme.frameFont().height() + menu.bevel_w*2;
	
    if (title_vis) {

        menu.item_w = m_theme.titleFont().textWidth(menu.label.c_str(), menu.label.size());
		
        menu.item_w += (menu.bevel_w * 2);
    }	else
        menu.item_w = 1;

    int ii = 0;
    Menuitems::iterator it = menuitems.begin();
    Menuitems::iterator it_end = menuitems.end();
    for (; it != it_end; ++it) {
        MenuItem *itmp = (*it);

        const char *s = itmp->label().c_str();
        int l = itmp->label().size();

        ii = m_theme.frameFont().textWidth(s, l);
			

        ii += (menu.bevel_w * 2) + (menu.item_h * 2);

        menu.item_w = ((menu.item_w < (unsigned int) ii) ? ii : menu.item_w);
    }

    if (menuitems.size()) {
        menu.sublevels = 1;

        while (menu.item_h * (menuitems.size() + 1) / menu.sublevels +
               menu.title_h + m_border_width > m_screen_height)           
            menu.sublevels++;

        if (menu.sublevels < menu.minsub) 
            menu.sublevels = menu.minsub;

        menu.persub = menuitems.size() / menu.sublevels;
        if (menuitems.size() % menu.sublevels) menu.persub++;
    } else {
        menu.sublevels = 0;
        menu.persub = 0;
    }

    menu.width = (menu.sublevels * (menu.item_w));
    if (! menu.width) menu.width = menu.item_w;

    menu.frame_h = (menu.item_h * menu.persub);
    menu.height = ((title_vis) ? menu.title_h + m_border_width : 0) +
        menu.frame_h;
    if (! menu.frame_h) menu.frame_h = 1;
    if (menu.height < 1) menu.height = 1;

    Pixmap tmp;
    if (title_vis) {
        tmp = menu.title_pixmap;
        const FbTk::Texture &tex = m_theme.titleTexture();
        if (tex.type() == (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
            menu.title_pixmap = None;
            menu.title.setBackgroundColor(tex.color());
        } else {
            menu.title_pixmap =
                m_image_ctrl.renderImage(menu.width, menu.title_h, tex);
            menu.title.setBackgroundPixmap(menu.title_pixmap);
        }

        if (tmp) 
            m_image_ctrl.removeImage(tmp);

        menu.title.clear();
    }

    tmp = menu.frame_pixmap;
    const FbTk::Texture &frame_tex = m_theme.frameTexture();
    if (frame_tex.type() == (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
        menu.frame_pixmap = None;
        menu.frame.setBackgroundColor(frame_tex.color());
    } else {
        menu.frame_pixmap =
            m_image_ctrl.renderImage(menu.width, menu.frame_h, frame_tex);
        menu.frame.setBackgroundPixmap(menu.frame_pixmap);
    }

    if (tmp)
        m_image_ctrl.removeImage(tmp);

    tmp = menu.hilite_pixmap;
    const FbTk::Texture &hilite_tex = m_theme.hiliteTexture();
    if (hilite_tex.type() == (FbTk::Texture::FLAT | FbTk::Texture::SOLID))
        menu.hilite_pixmap = None;
    else
        menu.hilite_pixmap =
            m_image_ctrl.renderImage(menu.item_w, menu.item_h, hilite_tex);
    if (tmp)
        m_image_ctrl.removeImage(tmp);

    tmp = menu.sel_pixmap;
    if (hilite_tex.type() == (FbTk::Texture::FLAT | FbTk::Texture::SOLID))
        menu.sel_pixmap = None;
    else {
        int hw = menu.item_h / 2;
        menu.sel_pixmap =
            m_image_ctrl.renderImage(hw, hw, hilite_tex);
    }
    if (tmp) 
        m_image_ctrl.removeImage(tmp);

    menu.window.resize(menu.width, menu.height);

    if (title_vis)
        menu.title.resize(menu.width, menu.title_h);

    menu.frame.moveResize(0, ((title_vis) ? menu.title_h + m_border_width : 0), 
                          menu.width, menu.frame_h);
    menu.window.clear();
    menu.title.clear();
    menu.frame.clear();

    if (title_vis && visible) 
        redrawTitle();

    unsigned int i = 0;
    for (i = 0; visible && i < menuitems.size(); i++) {
        if (i == (unsigned int)which_sub) {
            drawItem(i, true, 0);
            drawSubmenu(i);
        } else
            drawItem(i, false, 0);
    }

    if (m_parent && visible)
        m_parent->drawSubmenu(m_parent->which_sub);

    menu.window.showSubwindows();
}


void Menu::show() {
    menu.window.showSubwindows();
    menu.window.show();
    visible = true;

    if (! m_parent) {
        if (shown && (! shown->torn))
            shown->hide();

        shown = this;
    }

}


void Menu::hide() {
    if ((! torn) && hide_tree && m_parent && m_parent->isVisible()) {
        Menu *p = m_parent;

        while (p->isVisible() && (! p->torn) && p->m_parent)
            p = p->m_parent;
        p->internal_hide();
    } else
        internal_hide();
}


void Menu::internal_hide() {
    if (which_sub >= 0) {
        MenuItem *tmp = menuitems[which_sub];
        tmp->submenu()->internal_hide();
    }

    if (m_parent && (! torn)) {
        m_parent->drawItem(m_parent->which_sub, False, True);

        m_parent->which_sub = -1;
    } else if (shown && shown->menu.window == menu.window)
        shown = (Menu *) 0;

    torn = visible = false;
    which_sub = which_press = which_sub = -1;

    menu.window.hide();
}


void Menu::move(int x, int y) {
    menu.x = x;
    menu.y = y;
    menu.window.move(x, y);

    if (which_sub != -1)
        drawSubmenu(which_sub);
}


void Menu::redrawTitle() {
    const char *text = menu.label.c_str();

    const FbTk::Font &font = m_theme.titleFont();
    int dx = menu.bevel_w, len = menu.label.size();
    unsigned int l = font.textWidth(text, len) + menu.bevel_w*2;

    switch (m_theme.titleFontJustify()) {
    case FbTk::RIGHT:
        dx += menu.width - l;
        break;

    case FbTk::CENTER:
        dx += (menu.width - l) / 2;
        break;
    default:
        break;
    }

    font.drawText( 
                  menu.title.window(), // drawable
                  screenNumber(),
                  m_theme.titleTextGC(), // graphic context
                  text, len,  // text string with lenght
                  dx, font.ascent() + menu.bevel_w);  // position


}


void Menu::drawSubmenu(unsigned int index) {
    if (which_sub >= 0 && static_cast<unsigned int>(which_sub) != index && 
        static_cast<unsigned int>(which_sub) < menuitems.size()) {
        MenuItem *itmp = menuitems[which_sub];

        if (! itmp->submenu()->isTorn())
            itmp->submenu()->internal_hide();
    }

    if (index < menuitems.size()) {
        MenuItem *item = menuitems[index];
        if (item->submenu() && visible && (! item->submenu()->isTorn()) &&
            item->isEnabled()) {
			
            if (item->submenu()->m_parent != this)
                item->submenu()->m_parent = this;
			
            int sbl = index / menu.persub, i = index - (sbl * menu.persub),
                x = menu.x +
                ((menu.item_w * (sbl + 1)) + m_border_width), y;
		
            if (m_alignment == ALIGNTOP) {
                y = (((shifted) ? menu.y_shift : menu.y) +
                     ((title_vis) ? menu.title_h + m_border_width : 0) -
                     ((item->submenu()->title_vis) ?
                      item->submenu()->menu.title_h + m_border_width : 0));
            } else {
                y = (((shifted) ? menu.y_shift : menu.y) +
                     (menu.item_h * i) +
                     ((title_vis) ? menu.title_h + m_border_width : 0) -
                     ((item->submenu()->title_vis) ?
                      item->submenu()->menu.title_h + m_border_width : 0));
            }
			
            if (m_alignment == ALIGNBOTTOM &&
                (y + item->submenu()->menu.height) > ((shifted) ? menu.y_shift :
                                                      menu.y) + menu.height) {
                y = (((shifted) ? menu.y_shift : menu.y) +
                     menu.height - item->submenu()->menu.height);
            }

            if ((x + item->submenu()->width()) > m_screen_width) {
                x = ((shifted) ? menu.x_shift : menu.x) -
                    item->submenu()->width() - m_border_width;
            }
			
            if (x < 0)
                x = 0;

            if ((y + item->submenu()->height()) > m_screen_height) {
                y = m_screen_height - item->submenu()->height() -
                    m_border_width * 2;
            }
			
            if (y < 0)
                y = 0;

            item->submenu()->move(x, y);
            if (! moving)
                drawItem(index, True);
		
            if (! item->submenu()->isVisible())
                item->submenu()->show();
			
            item->submenu()->moving = moving;
            which_sub = index;
        } else
            which_sub = -1;
    }
}


bool Menu::hasSubmenu(unsigned int index) const {
    if (index >= menuitems.size()) //boundary check
        return false;
	
    if (!menuitems[index]->submenu()) //has submenu?
        return false;
	
    return true;	
}


void Menu::drawItem(unsigned int index, bool highlight, bool clear,
                    int x, int y, unsigned int w, unsigned int h)
{
    if (index >= menuitems.size() || menuitems.size() == 0)
        return;

    MenuItem *item = menuitems[index];
    if (! item) return;
	
    bool dotext = true, dohilite = true, dosel = true;
    const char *text = item->label().c_str();
    int sbl = 0, i = index - (sbl * menu.persub);
    if (menu.persub != 0)
        sbl = index / menu.persub;

    int item_x = (sbl * menu.item_w), item_y = (i * menu.item_h);
    int hilite_x = item_x, hilite_y = item_y, hoff_x = 0, hoff_y = 0;
    int text_x = 0, text_y = 0, len = strlen(text), sel_x = 0, sel_y = 0;
    unsigned int hilite_w = menu.item_w, hilite_h = menu.item_h, text_w = 0, text_h = 0;
    unsigned int half_w = menu.item_h / 2, quarter_w = menu.item_h / 4;
    const FbTk::Font &font = m_theme.frameFont();
    if (text) {		
        text_w = font.textWidth(text, len);

        text_y = item_y + menu.bevel_w/2 + font.ascent();

        switch(m_theme.frameFontJustify()) {
        case FbTk::LEFT:
            text_x = item_x + menu.bevel_w + menu.item_h + 1;
            break;
			
        case FbTk::RIGHT:
            text_x = item_x + menu.item_w - (menu.item_h + menu.bevel_w + text_w);
            break;			
        default: //center
            text_x = item_x + ((menu.item_w + 1 - text_w) / 2);
            break;
        }

        text_h = menu.item_h - menu.bevel_w;
    }
	
    GC gc =
        ((highlight || item->isSelected()) ? m_theme.hiliteTextGC() :
         m_theme.frameTextGC());
    GC tgc =
        (highlight ? m_theme.hiliteTextGC() :
         /*else*/ (item->isEnabled() ? m_theme.frameTextGC() : m_theme.disableTextGC() ) );
	
    sel_x = item_x;
	
    if (m_theme.bulletPos() == FbTk::RIGHT)
        sel_x += (menu.item_w - menu.item_h - menu.bevel_w);
	
    sel_x += quarter_w;
    sel_y = item_y + quarter_w;
	
    if (clear) {
        XClearArea(m_display, menu.frame.window(), item_x, item_y, menu.item_w, menu.item_h,
                   False);
    } else if (! (x == y && y == -1 && w == h && h == 0)) {
        // calculate the which part of the hilite to redraw
        if (! (std::max(item_x, x) <= (signed) std::min(item_x + menu.item_w, x + w) &&
               std::max(item_y, y) <= (signed) std::min(item_y + menu.item_h, y + h))) {
            dohilite = False;
        } else {
            hilite_x = std::max(item_x, x);
            hilite_y = std::max(item_y, y);
            hilite_w = std::min(item_x + menu.item_w, x + w) - hilite_x;
            hilite_h = std::min(item_y + menu.item_h, y + h) - hilite_y;
            hoff_x = hilite_x % menu.item_w;
            hoff_y = hilite_y % menu.item_h;
        }
		
        // check if we need to redraw the text		
        int text_ry = item_y + (menu.bevel_w / 2);
        if (! (std::max(text_x, x) <= (signed) std::min(text_x + text_w, x + w) &&
               std::max(text_ry, y) <= (signed) std::min(text_ry + text_h, y + h)))
            dotext = False;
		
        // check if we need to redraw the select pixmap/menu bullet
        if (! (std::max(sel_x, x) <= (signed) std::min(sel_x + half_w, x + w) &&
               std::max(sel_y, y) <= (signed) std::min(sel_y + half_w, y + h)))
            dosel = False;
	
    }
	
    if (dohilite && highlight && (menu.hilite_pixmap != ParentRelative)) {
        if (menu.hilite_pixmap) {
            XCopyArea(m_display, menu.hilite_pixmap, menu.frame.window(),
                      m_theme.hiliteGC(), hoff_x, hoff_y,
                      hilite_w, hilite_h, hilite_x, hilite_y);
        } else {
            XFillRectangle(m_display, menu.frame.window(),
                           m_theme.hiliteGC(),
                           hilite_x, hilite_y, hilite_w, hilite_h);
        }
    } 
	
    if (dosel && item->isSelected() &&
        (menu.sel_pixmap != ParentRelative)) {
        if (menu.sel_pixmap) {
            XCopyArea(m_display, highlight ? menu.frame_pixmap : menu.sel_pixmap, menu.frame.window(),
                      m_theme.hiliteGC(), 0, 0,
                      half_w, half_w, sel_x, sel_y);
        } else {
            XFillRectangle(m_display, menu.frame.window(),
                           m_theme.hiliteGC(),
                           sel_x, sel_y, half_w, half_w);
        }
    }
	
    if (dotext && text) {
        m_theme.frameFont().drawText(
                                     menu.frame.window(), // drawable
                                     screenNumber(),
                                     tgc,
                                     text, len, // text string and lenght
                                     text_x, text_y); // position

    }

    if (dosel && item->submenu()) {
        switch (m_theme.bullet()) {
        case SQUARE:
            XDrawRectangle(m_display, menu.frame.window(), gc, sel_x, sel_y, half_w, half_w);
            break;

        case TRIANGLE:
            XPoint tri[3];

            if (m_theme.bulletPos() == FbTk::RIGHT) {
                tri[0].x = sel_x + quarter_w - 2;
                tri[0].y = sel_y + quarter_w - 2;
                tri[1].x = 4;
                tri[1].y = 2;
                tri[2].x = -4;
                tri[2].y = 2;
            } else {
                tri[0].x = sel_x + quarter_w - 2;
                tri[0].y = item_y + half_w;
                tri[1].x = 4;
                tri[1].y = 2;
                tri[2].x = 0;
                tri[2].y = -4;
            }
			
            XFillPolygon(m_display, menu.frame.window(), gc, tri, 3, Convex,
                         CoordModePrevious);
            break;
			
        case DIAMOND:
            XPoint dia[4];

            dia[0].x = sel_x + quarter_w - 3;
            dia[0].y = item_y + half_w;
            dia[1].x = 3;
            dia[1].y = -3;
            dia[2].x = 3;
            dia[2].y = 3;
            dia[3].x = -3;
            dia[3].y = 3;

            XFillPolygon(m_display, menu.frame.window(), gc, dia, 4, Convex,
                         CoordModePrevious);
            break;
        }
    }
}


void Menu::setLabel(const char *labelstr) {
    //make sure we don't send 0 to std::string
    menu.label = (labelstr ? labelstr : "");
    reconfigure();
}


void Menu::setItemSelected(unsigned int index, bool sel) {
    if (index >= menuitems.size()) return;

    MenuItem *item = find(index);
    if (! item) return;

    item->setSelected(sel);
    if (visible) drawItem(index, (index == (unsigned int)which_sub), true);
}


bool Menu::isItemSelected(unsigned int index) const{
    if (index >= menuitems.size()) return false;

    const MenuItem *item = find(index);
    if (!item)
        return false;

    return item->isSelected();
}


void Menu::setItemEnabled(unsigned int index, bool enable) {
    if (index >= menuitems.size()) return;

    MenuItem *item = find(index);
    if (! item) return;

    item->setEnabled(enable);
    if (visible) 
        drawItem(index, (index == static_cast<unsigned int>(which_sub)), True);
}


bool Menu::isItemEnabled(unsigned int index) const {
    if (index >= menuitems.size()) return false;

    MenuItem *item = find(index);
    if (!item)
        return false;

    return item->isEnabled();
}


void Menu::buttonPressEvent(XButtonEvent &be) {
    if (be.window == menu.frame) {
        int sbl = (be.x / menu.item_w), i = (be.y / menu.item_h);
        int w = (sbl * menu.persub) + i;

        if (w < static_cast<int>(menuitems.size()) && w >= 0) {
            which_press = i;
            which_sbl = sbl;

            MenuItem *item = menuitems[w];

            if (item->submenu())
                drawSubmenu(w);
            else
                drawItem(w, (item->isEnabled()), true);
        }
    } else {
        menu.x_move = be.x_root - menu.x;
        menu.y_move = be.y_root - menu.y;
    }
}


void Menu::buttonReleaseEvent(XButtonEvent &re) {
    if (re.window == menu.title) {
        if (moving) {
            moving = false;
			
            if (which_sub >= 0)
                drawSubmenu(which_sub);
        }

        if (re.x >= 0 && re.x <= (signed) menu.width &&
            re.y >= 0 && re.y <= (signed) menu.title_h &&
            re.button == 3)
            hide();
			
    } else if (re.window == menu.frame &&
               re.x >= 0 && re.x < (signed) menu.width &&
               re.y >= 0 && re.y < (signed) menu.frame_h) {
			
        if (re.button == 3) {
            hide();
        } else {
            int sbl = (re.x / menu.item_w), i = (re.y / menu.item_h),
                ix = sbl * menu.item_w, iy = i * menu.item_h,
                w = (sbl * menu.persub) + i,
                p = (which_sbl * menu.persub) + which_press;

            if (w < static_cast<int>(menuitems.size()) && w >= 0) {
                drawItem(p, (p == which_sub), True);

                if	(p == w && isItemEnabled(w)) {
                    if (re.x > ix && re.x < (signed) (ix + menu.item_w) &&
                        re.y > iy && re.y < (signed) (iy + menu.item_h)) {
                        if (*menuitems[w]->command() != 0)
                            menuitems[w]->command()->execute();
                        else
                            itemSelected(re.button, w);
                    }
                }
            } else
                drawItem(p, false, true);
				
        }
    }
}


void Menu::motionNotifyEvent(XMotionEvent &me) {
    if (me.window == menu.title && (me.state & Button1Mask)) {
        if (movable) {
            if (! moving) {
                if (m_parent && (! torn)) {
                    m_parent->drawItem(m_parent->which_sub, false, true);
                    m_parent->which_sub = -1;
                }

                moving = torn = True;

                if (which_sub >= 0)
                    drawSubmenu(which_sub);
            } else {
                menu.x = me.x_root - menu.x_move,
                    menu.y = me.y_root - menu.y_move;
	
                menu.window.move(menu.x, menu.y);

                if (which_sub >= 0)
                    drawSubmenu(which_sub);
            }
        }
    } else if ((! (me.state & Button1Mask)) && me.window == menu.frame &&
               me.x >= 0 && me.x < (signed) menu.width &&
               me.y >= 0 && me.y < (signed) menu.frame_h) {
        int sbl = (me.x / menu.item_w), i = (me.y / menu.item_h),
            w = (sbl * menu.persub) + i;

        if ((i != which_press || sbl != which_sbl) &&
            (w < static_cast<int>(menuitems.size()) && w >= 0)) {
            if (which_press != -1 && which_sbl != -1) {
                int p = (which_sbl * menu.persub) + which_press;
                MenuItem *item = menuitems[p];

                drawItem(p, False, True);
                if (item->submenu()) {
                    if (item->submenu()->isVisible() &&
                        (! item->submenu()->isTorn())) {
                        item->submenu()->internal_hide();
                        which_sub = -1;
                    }
                }
            }

            which_press = i;
            which_sbl = sbl;

            MenuItem *itmp = menuitems[w];

            if (itmp->submenu())
                drawSubmenu(w);
            else
                drawItem(w, (itmp->isEnabled()), True);
        }
    }
}


void Menu::exposeEvent(XExposeEvent &ee) {
    if (ee.window == menu.title) {
        redrawTitle();
    } else if (ee.window == menu.frame) {
        // this is a compilicated algorithm... lets do it step by step...
        // first... we see in which sub level the expose starts... and how many
        // items down in that sublevel

        int sbl = (ee.x / menu.item_w), id = (ee.y / menu.item_h),
            // next... figure out how many sublevels over the redraw spans
            sbl_d = ((ee.x + ee.width) / menu.item_w),
            // then we see how many items down to redraw
            id_d = ((ee.y + ee.height) / menu.item_h);

        if (id_d > menu.persub) id_d = menu.persub;

        // draw the sublevels and the number of items the exposure spans
        int i, ii;
        for (i = sbl; i <= sbl_d; i++) {
            // set the iterator to the first item in the sublevel needing redrawing
            int index = id + i * menu.persub;
            if (index < static_cast<int>(menuitems.size()) && index >= 0) {
                Menuitems::iterator it = menuitems.begin() + index;
                Menuitems::iterator it_end = menuitems.end();
                for (ii = id; ii <= id_d && it != it_end; ++it, ii++) {
                    int index = ii + (i * menu.persub);
                    // redraw the item
                    drawItem(index, (which_sub == index), true,
                             ee.x, ee.y, ee.width, ee.height);
                }
            }
        }
    }
}


void Menu::enterNotifyEvent(XCrossingEvent &ce) {

    if (menu.frame != ce.window)
        return;

    menu.x_shift = menu.x, menu.y_shift = menu.y;
    if (menu.x + menu.width > m_screen_width) {
        menu.x_shift = m_screen_width - menu.width - m_border_width;
        shifted = true;
    } else if (menu.x < 0) {
        menu.x_shift = -m_border_width;
        shifted = true;
    }

    if (menu.y + menu.height > m_screen_height) {
        menu.y_shift = m_screen_height - menu.height - m_border_width;
        shifted = true;
    } else if (menu.y + (signed) menu.title_h < 0) {
        menu.y_shift = -m_border_width;;
        shifted = true;
    }


    if (shifted)
        menu.window.move(menu.x_shift, menu.y_shift);
    
    if (which_sub >= 0 && static_cast<size_t>(which_sub) < menuitems.size()) {
        MenuItem *tmp = menuitems[which_sub];
        if (tmp->submenu()->isVisible()) {
            int sbl = (ce.x / menu.item_w), i = (ce.y / menu.item_h),
                w = (sbl * menu.persub) + i;

            if (w != which_sub && (! tmp->submenu()->isTorn())) {
                tmp->submenu()->internal_hide();

                drawItem(which_sub, false, true);
                which_sub = -1;
            }
        }
    }
}

void Menu::leaveNotifyEvent(XCrossingEvent &ce) {
    if (menu.frame != ce.window)
        return;

    if (which_press != -1 && which_sbl != -1 && menuitems.size() > 0) {
        int p = (which_sbl * menu.persub) + which_press;

        drawItem(p, (p == which_sub), true);

        which_sbl = which_press = -1;
    }

    if (shifted) {
        menu.window.move(menu.x, menu.y);
        shifted = false;

        if (which_sub >= 0)
            drawSubmenu(which_sub);
    }
}


void Menu::reconfigure() {
    //!! TODO
    //   menu.window.setBackgroundColor(*m_screen->getBorderColor());
    //   menu.window.setBorderColor(*m_screen->getBorderColor());
    //   menu.window.setBorderWidth(m_screen->getBorderWidth());

    //    menu.bevel_w = m_screen->getBevelWidth();
    update();
}

}; // end namespace FbTk
