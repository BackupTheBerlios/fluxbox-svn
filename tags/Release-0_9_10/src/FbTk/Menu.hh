// Menu.hh for FbTk - Fluxbox Toolkit
// Copyright (c) 2001 - 2004 Henrik Kinnunen (fluxgen at users.sourceforge.net)
//
// Basemenu.hh for Blackbox - an X11 Window manager
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// $Id: Menu.hh,v 1.39 2004/08/29 12:35:29 rathnor Exp $

#ifndef	 FBTK_MENU_HH
#define	 FBTK_MENU_HH

#include <X11/Xlib.h>
#include <vector>
#include <string>
#include <memory>

#include "FbWindow.hh"
#include "EventHandler.hh"
#include "RefCount.hh"
#include "Command.hh"
#include "Observer.hh"
#include "FbPixmap.hh"
#include "MenuTheme.hh"
#include "Timer.hh"

namespace FbTk {

class MenuItem;
class ImageControl;
class Transparent;

///   Base class for menus
class Menu: public FbTk::EventHandler, protected FbTk::Observer {
public:
    enum Alignment{ ALIGNDONTCARE = 1, ALIGNTOP, ALIGNBOTTOM };
    enum { RIGHT = 1, LEFT };
	
    /**
       Bullet type
    */
    enum { EMPTY = 0, SQUARE, TRIANGLE, DIAMOND };
	
    Menu(MenuTheme &tm, ImageControl &imgctrl);
    virtual ~Menu();

    /**
       @name manipulators
    */
    //@{
    /// add a menu item with a label and a command
    int insert(const char *label, RefCount<Command> &cmd, int pos=-1);
    /// add empty menu item
    int insert(const char *label, int pos=-1);
    /// add submenu
    int insert(const char *label, Menu *submenu, int pos= -1);
    /// add menu item
    int insert(MenuItem *item, int pos=-1);
    /// remove an item
    int remove(unsigned int item);
    /// remove all items
    void removeAll();
    inline void setInternalMenu(bool val = true) { internal_menu = val; }
    inline void setAlignment(Alignment a) { m_alignment = a; }
    inline void setTorn() { torn = true; }
    inline void removeParent() { if (internal_menu) m_parent = 0; }
    /// raise this window
    virtual void raise();
    /// lower this window
    virtual void lower();
    /// select next item
    void nextItem();
    /// select previous item
    void prevItem();
    void enterSubmenu();
    void enterParent();

    void disableTitle();
    void enableTitle();

    /**
       @name event handlers
    */
    //@{
    void handleEvent(XEvent &event);
    void buttonPressEvent(XButtonEvent &bp);
    void buttonReleaseEvent(XButtonEvent &br);
    void motionNotifyEvent(XMotionEvent &mn);
    void enterNotifyEvent(XCrossingEvent &en);
    void leaveNotifyEvent(XCrossingEvent &ce);
    void exposeEvent(XExposeEvent &ee);
    void keyPressEvent(XKeyEvent &ke);
    //@}
    /// get input focus
    void grabInputFocus();
    virtual void reconfigure();
    /// set label string
    void setLabel(const char *labelstr);
    /// move menu to x,y
    void move(int x, int y);
    virtual void update(int active_index = -1);
    void setItemSelected(unsigned int index, bool val);
    void setItemEnabled(unsigned int index, bool val);
    inline void setMinimumSublevels(int m) { menu.minsub = m; }
    virtual void drawSubmenu(unsigned int index);
    /// show menu
    virtual void show();
    /// hide menu
    virtual void hide();
    virtual void clearWindow();
    void setActiveIndex(int index) { m_active_index = index; }
    /*@}*/
	
    /**
       @name accessors
    */
    //@{
    inline int activeIndex() const { return m_active_index; }
    inline bool isTorn() const { return torn; }
    inline bool isVisible() const { return visible; }
    inline int screenNumber() const { return menu.window.screenNumber(); }
    inline Window window() const { return menu.window.window(); }
    inline FbWindow &fbwindow() { return menu.window; }
    inline const FbWindow &fbwindow() const { return menu.window; }
    inline FbWindow &titleWindow() { return menu.title; }
    inline FbWindow &frameWindow() { return menu.frame; }
    inline const std::string &label() const { return menu.label; }  
    inline int x() const { return menu.window.x(); }
    inline int y() const { return menu.window.y(); }
    inline unsigned int width() const { return menu.window.width(); }
    inline unsigned int height() const { return menu.window.height(); }
    inline unsigned int numberOfItems() const { return menuitems.size(); }
    inline int currentSubmenu() const { return which_sub; } 
    bool hasSubmenu(unsigned int index) const;
    bool isItemSelected(unsigned int index) const;
    bool isItemEnabled(unsigned int index) const;
    bool isItemSelectable(unsigned int index) const;
    inline const MenuTheme &theme() const { return m_theme; }
    inline unsigned char alpha() const { return theme().alpha(); }
    inline static Menu *focused() { return s_focused; }
    /// @return menuitem at index
    inline const MenuItem *find(unsigned int index) const { return menuitems[index]; }
    inline MenuItem *find(unsigned int index) { return menuitems[index]; }
    //@}
    /// @return true if index is valid
    inline bool validIndex(int index) const { return (index < static_cast<int>(numberOfItems()) && index >= 0); }

    inline Menu *parent() { return m_parent; }
    inline const Menu *parent() const { return m_parent; }

protected:

    inline void setTitleVisibility(bool b) { 
        title_vis = b; m_need_update = true; 
        if (!b)
            titleWindow().lower();
        else
            titleWindow().raise();
    }

    virtual void itemSelected(int button, unsigned int index) { }
    virtual int drawItem(unsigned int index,
                         bool clear = false, bool render_trans = true,
                         int x= -1, int y= -1, 
                         unsigned int width= 0, unsigned int height= 0);
    virtual void redrawTitle();
    virtual void internal_hide();

    void update(FbTk::Subject *);
    void renderTransp(int x, int y,
                      unsigned int width, unsigned int height);
private: 

    void openSubmenu();
    void closeMenu();
    void startHide();
    void stopHide();


    typedef std::vector<MenuItem *> Menuitems;
    const MenuTheme &m_theme;
    Menu *m_parent;
    ImageControl &m_image_ctrl;
    Menuitems menuitems;

    const unsigned int m_screen_width, m_screen_height;
    bool moving; ///< if we're moving/draging or not
    bool visible; ///< menu visibility
    bool torn; ///< torn from parent
    bool internal_menu; ///< whether we should destroy this menu or if it's managed somewhere else
    bool title_vis; ///< title visibility
    bool shifted; ///< if the menu is shifted to the other side of the parent
	
    int which_sub, which_press, which_sbl;
    Alignment m_alignment;

    struct _menu {
        Pixmap frame_pixmap, title_pixmap, hilite_pixmap, sel_pixmap;
        FbTk::FbWindow window, frame, title;

        std::string label;
        int x_move, y_move, x_shift, y_shift, sublevels, persub, minsub,
            grab_x, grab_y;

        unsigned int frame_h, item_w;
    } menu;

    int m_active_index; ///< current highlighted index

    Drawable m_root_pm;
    static Menu *s_focused; ///< holds current input focused menu, so one can determine if a menu is focused
    FbPixmap m_frame_pm,  ///< buffer pixmap
        m_real_frame_pm; ///< buffer pixmap (this one is shown to the user)
    FbPixmap m_title_pm, ///< buffer pixmap to avoid flicker
        m_real_title_pm; ///< buffer pixmap (this one is shown to the user)
    std::auto_ptr<Transparent> m_transp;
    bool m_need_update;
    Timer m_submenu_timer;
    Timer m_hide_timer;
};

} // end namespace FbTk

#endif // FBTK_MENU_HH
