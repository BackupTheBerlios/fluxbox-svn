// IconbarTool.hh
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
//                and Simon Bowden    (rathnor at users.sourceforge.net)
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

// $Id$

#ifndef ICONBARTOOL_HH
#define ICONBARTOOL_HH

#include "ToolbarItem.hh"
#include "Container.hh"
#include "FbMenu.hh"

#include "FbTk/CachedPixmap.hh"
#include "FbTk/Observer.hh"
#include "FbTk/Resource.hh"
#include "FbTk/Menu.hh"

#include <X11/Xlib.h>

#include <list>

class IconbarTheme;
class BScreen;
class IconButton;
class Focusable;

class IconbarTool: public ToolbarItem, public FbTk::Observer {
public:
    typedef std::list<IconButton *> IconList;
    /// iconbar mode
    enum Mode {
        NONE, ///< no icons
        ICONS,  ///< all icons from all workspaces
        NOICONS, ///< all noniconified windows from all workspaces
        WORKSPACEICONS,  ///< icons on current workspace
        WORKSPACENOICONS, ///< non iconified workspaces on current workspaces
        WORKSPACE, ///< all windows and all icons on current workspace
        ALLWINDOWS ///< all windows and all icons from all workspaces
    };

    IconbarTool(const FbTk::FbWindow &parent, IconbarTheme &theme, 
                BScreen &screen, FbTk::Menu &menu);
    ~IconbarTool();

    void move(int x, int y);
    void resize(unsigned int width, unsigned int height);
    void moveResize(int x, int y,
                    unsigned int width, unsigned int height);

    void update(FbTk::Subject *subj);
    void show();
    void hide();
    void setAlignment(Container::Alignment a);
    void setMode(Mode mode);
    void parentMoved() { m_icon_container.parentMoved(); }

    unsigned int width() const;
    unsigned int height() const;
    unsigned int borderWidth() const;

    Mode mode() const { return *m_rc_mode; }

    void setOrientation(FbTk::Orientation orient);
    Container::Alignment alignment() const { return m_icon_container.alignment(); }

    const BScreen &screen() const { return m_screen; }
private:

    /// @return button associated with window
    IconButton *findButton(Focusable &win);

    void updateSizing();

    /// render single button, and probably apply changes (clear)
    /// @param button the button to render
    /// @param clear if the window should be cleared first
    void renderButton(IconButton &button, bool clear = true);
    /// render all buttons
    void renderTheme();
    void renderTheme(unsigned char alpha);
    /// destroy all icons
    void deleteIcons();
    /// remove a single window
    void removeWindow(Focusable &win);
    /// add a single window 
    void addWindow(Focusable &win);
    /// add icons to the list
    void updateList();
    /// check if window is already in the list
    bool checkDuplicate(Focusable &win);

    BScreen &m_screen;
    Container m_icon_container;
    IconbarTheme &m_theme;
    FbTk::CachedPixmap m_empty_pm; ///< pixmap for empty container


    IconList m_icon_list;
    FbTk::Resource<Mode> m_rc_mode;
    FbTk::Resource<Container::Alignment> m_rc_alignment; ///< alignment of buttons
    FbTk::Resource<int> m_rc_client_width; ///< size of client button in LEFT/RIGHT mode
    FbTk::Resource<unsigned int> m_rc_client_padding; ///< padding of the text
    FbTk::Resource<bool> m_rc_use_pixmap; ///< if iconbar should use win pixmap or not
    FbMenu m_menu;
    unsigned char m_alpha;
};

#endif // ICONBARTOOL_HH
