// Window.hh for Fluxbox Window Manager
// Copyright (c) 2001 - 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
//
// Window.hh for Blackbox - an X11 Window manager
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

// $Id: Window.hh,v 1.118 2004/08/10 19:18:47 fluxgen Exp $

#ifndef	 WINDOW_HH
#define	 WINDOW_HH

#include "FbTk/Timer.hh"
#include "FbTk/Subject.hh"
#include "FbTk/EventHandler.hh"
#include "FbTk/XLayerItem.hh"

#include "FbWinFrame.hh"

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <vector>
#include <string>
#include <memory>
#include <map>

class WinClient;
class FbWinFrameTheme;
class BScreen;
class FbWinFrame;

namespace FbTk {
class TextButton;
class MenuTheme;
class ImageControl;
class XLayer;
class Menu;
}

/// Creates the window frame and handles any window event for it
class FluxboxWindow: public FbTk::EventHandler {
public:
    /// Represents certain "preset" sets of decorations.
    enum Decoration {
        DECOR_NONE=0, ///< no decor at all
        DECOR_NORMAL, ///< normal normal
        DECOR_TINY,   ///< tiny decoration
        DECOR_TOOL    ///< decor tool
    };

    /// Motif wm Hints
    enum {
        MwmHintsFunctions   = (1l << 0), ///< use motif wm functions
        MwmHintsDecorations	= (1l << 1) ///< use motif wm decorations
    };
	
    /// Motif wm functions
    enum MwmFunc{
        MwmFuncAll          = (1l << 0), ///< all motif wm functions
        MwmFuncResize       = (1l << 1), ///< resize 
        MwmFuncMove         = (1l << 2), ///< move 
        MwmFuncIconify      = (1l << 3), ///< iconify
        MwmFuncMaximize     = (1l << 4), ///< maximize
        MwmFuncClose        = (1l << 5)  ///< close
    };

    /// Motif wm decorations
    enum MwmDecor {
        MwmDecorAll         = (1l << 0), /// all decorations
        MwmDecorBorder      = (1l << 1), /// border
        MwmDecorHandle      = (1l << 2), /// handle
        MwmDecorTitle       = (1l << 3), /// title
        MwmDecorMenu        = (1l << 4), /// menu
        MwmDecorIconify     = (1l << 5), /// iconify
        MwmDecorMaximize    = (1l << 6)  /// maximize
    };

    /// attributes for BlackboxHints
    enum Attrib {
        ATTRIB_SHADED = 0x01,
        ATTRIB_MAXHORIZ = 0x02,
        ATTRIB_MAXVERT = 0x04,
        ATTRIB_OMNIPRESENT = 0x08,
        ATTRIB_WORKSPACE = 0x10,
        ATTRIB_STACK = 0x20,		
        ATTRIB_DECORATION = 0x40,
        ATTRIB_HIDDEN = 0x80,
    };	

    /**
     * Types of maximization
     */
    enum MaximizeMode {
        MAX_NONE = 0, ///< normal state
        MAX_HORZ = 1, ///< maximize horizontal
        MAX_VERT = 2, ///< maximize vertical
        MAX_FULL = 3  ///< maximize full
    };
    /** 
       This enumeration represents individual decoration 
       attributes, they can be OR-d together to get a mask.
       Useful for saving.
    */
    enum DecorationMask {
        DECORM_TITLEBAR = (1<<0),
        DECORM_HANDLE   = (1<<1),
        DECORM_BORDER   = (1<<2),
        DECORM_ICONIFY  = (1<<3),
        DECORM_MAXIMIZE = (1<<4),
        DECORM_CLOSE    = (1<<5),
        DECORM_MENU     = (1<<6),
        DECORM_STICKY   = (1<<7),
        DECORM_SHADE    = (1<<8),
        DECORM_TAB      = (1<<9),
        DECORM_ENABLED  = (1<<10),
        DECORM_LAST     = (1<<11) // useful for getting "All"
    };

    enum ResizeCorner {
        LEFTTOP,
        LEFTBOTTOM,
        RIGHTBOTTOM,
        RIGHTTOP
    };

    typedef struct _blackbox_hints {
        unsigned long flags, attrib, workspace, stack;
        int decoration;
    } BlackboxHints;

    typedef struct _blackbox_attributes {
        unsigned long flags, attrib, workspace, stack;
        int premax_x, premax_y;
        unsigned int premax_w, premax_h;
    } BlackboxAttributes;

    typedef std::list<WinClient *> ClientList;
    // this should perhaps be a refcount??
    typedef std::list<std::pair<const char *, FbTk::Menu *> > ExtraMenus;

    /// create a window from a client
    FluxboxWindow(WinClient &client,
                  FbWinFrameTheme &tm,
                  FbTk::XLayer &layer);

    virtual ~FluxboxWindow();

    /// attach client to our client list and remove it from old window
    void attachClient(WinClient &client);
    /// detach client (remove it from list) and create a new window for it 
    bool detachClient(WinClient &client);
    /// detach current working client if we have more than one
    void detachCurrentClient();
    /// remove client from client list
    bool removeClient(WinClient &client);
    /// set new current client and raise it
    bool setCurrentClient(WinClient &client, bool setinput = true);
    WinClient *findClient(Window win);
    void nextClient();
    void prevClient();
    void moveClientLeft();
    void moveClientRight();

    bool setInputFocus();
    void raiseAndFocus() { raise(); setInputFocus(); }
    void setFocusFlag(bool flag);
    // map this window
    void show();
    // unmap this window
    void hide() { hide(true); }
    void hide(bool interrupt_moving);
    void iconify();
    void deiconify(bool reassoc = true, bool do_raise = true);
    /// close current client
    void close();
    /// set the window in withdrawn state
    void withdraw(bool interrupt_moving);
    /// toggle maximize
    void maximize(int type = MAX_FULL);
    /// maximizes the window horizontal
    void maximizeHorizontal();
    /// maximizes the window vertical
    void maximizeVertical();
    /// maximizes the window fully
    void maximizeFull();
    /// toggles shade
    void shade();
    /// toggles sticky
    void stick(); 
    void raise();
    void lower();
    void tempRaise();
    void raiseLayer();
    void lowerLayer();
    void moveToLayer(int layernum);
    void setFocusHidden(bool value);
    void setIconHidden(bool value);
    void reconfigure();


    void installColormap(bool);
    void restore(WinClient *client, bool remap);
    void restore(bool remap);
    /// move frame to x, y
    void move(int x, int y, int gravity = ForgetGravity);
    /// resize frame to width, height
    void resize(unsigned int width, unsigned int height);
    /// move and resize frame to pox x,y and size width, height
    void moveResize(int x, int y, unsigned int width, unsigned int height, int gravity = ForgetGravity);

    void setWorkspace(int n);
    void changeBlackboxHints(const BlackboxHints &bh);
    void updateFunctions();
    void restoreAttributes();
    void showMenu(int mx, int my);
    // popup menu on last button press position
    void popupMenu();

    void pauseMoving();
    void resumeMoving();
    /**
       @name event handlers
    */
    //@{
    void handleEvent(XEvent &event);
    void buttonPressEvent(XButtonEvent &be);
    void buttonReleaseEvent(XButtonEvent &be);
    void motionNotifyEvent(XMotionEvent &me);
    void destroyNotifyEvent(XDestroyWindowEvent &dwe);
    void mapRequestEvent(XMapRequestEvent &mre);
    void mapNotifyEvent(XMapEvent &mapev);
    void unmapNotifyEvent(XUnmapEvent &unmapev);
    void exposeEvent(XExposeEvent &ee);
    void configureRequestEvent(XConfigureRequestEvent &ce);
    void propertyNotifyEvent(WinClient &client, Atom a);
    void enterNotifyEvent(XCrossingEvent &ev);
    void leaveNotifyEvent(XCrossingEvent &ev);
    //@}

    void setDecoration(Decoration decoration, bool apply = true);
    void applyDecorations(bool initial = false);
    void toggleDecoration();

    unsigned int decorationMask() const;
    void setDecorationMask(unsigned int mask);

    /**
       @name accessors		
    */
    //@{
    // whether this window can be tabbed with other windows, 
    // and others tabbed with it
    inline void setTabable(bool tabable) { functions.tabable = tabable; }
    inline bool isTabable() { return functions.tabable; }
    inline void setMovable(bool movable) { functions.move = movable; }
    inline void setResizable(bool resizable) { functions.resize = resizable; }

    inline bool isFocusHidden() const { return (m_blackbox_attrib.flags & ATTRIB_HIDDEN); }
    inline bool isIconHidden() const { return m_icon_hidden; } 
    inline bool isManaged() const { return m_managed; }
    inline bool isFocused() const { return focused; }
    bool isVisible() const;
    inline bool isIconic() const { return iconic; }
    inline bool isShaded() const { return shaded; }
    inline bool isMaximized() const { return maximized == MAX_FULL; }
    inline bool isIconifiable() const { return functions.iconify; }
    inline bool isMaximizable() const { return functions.maximize; }
    inline bool isResizable() const { return functions.resize; }
    inline bool isClosable() const { return functions.close; }
    inline bool isStuck() const { return stuck; }
    inline bool hasTitlebar() const { return decorations.titlebar; }
    inline bool isMoving() const { return moving; }
    inline bool isResizing() const { return resizing; }
    bool isGroupable() const;
    inline int numClients() const { return m_clientlist.size(); }
    inline bool empty() const { return m_clientlist.empty(); }
    inline ClientList &clientList() { return m_clientlist; }
    inline const ClientList &clientList() const { return m_clientlist; }
    inline WinClient &winClient() { return *m_client; }
    inline const WinClient &winClient() const { return *m_client; }

    inline const BScreen &screen() const { return m_screen; }
    inline BScreen &screen() { return m_screen; }

    inline const FbTk::XLayerItem &layerItem() const { return m_layeritem; }
    inline FbTk::XLayerItem &layerItem() { return m_layeritem; }

    Window clientWindow() const;

    FbTk::FbWindow &fbWindow();
    const FbTk::FbWindow &fbWindow() const;

    FbTk::Menu &menu() { return *m_windowmenu.get(); }
    const FbTk::Menu &menu() const { return *m_windowmenu.get(); }


    // for extras to add menus. 
    // These menus will be marked internal, 
    // and deleted when the window dies (as opposed to Screen
    void addExtraMenu(const char *label, FbTk::Menu *menu);
    void removeExtraMenu(FbTk::Menu *menu);

    ExtraMenus &extraMenus() { return m_extramenus; }
    const ExtraMenus &extraMenus() const { return m_extramenus; }

    const FbTk::FbWindow &parent() const { return m_parent; }
    FbTk::FbWindow &parent() { return m_parent; }

    const std::string &title() const;
    const std::string &iconTitle() const;
    inline int x() const { return frame().x(); }
    inline int y() const { return frame().y(); }
    inline unsigned int width() const { return frame().width(); }
    inline unsigned int height() const { return frame().height(); }

    unsigned int workspaceNumber() const { return m_workspace_number; }

    int layerNum() const { return m_layernum; }
    void setLayerNum(int layernum);
 

    unsigned int titlebarHeight() const;

    bool isLowerTab() const;
    int initialState() const;

    inline FbWinFrame &frame() { return m_frame; }
    inline const FbWinFrame &frame() const { return m_frame; }

    /**
       @name signals
       @{
    */
    FbTk::Subject &stateSig() { return m_statesig; }
    const FbTk::Subject &stateSig() const { return m_statesig; }
    FbTk::Subject &layerSig() { return m_layersig; }
    const FbTk::Subject &layerSig() const { return m_layersig; }
    FbTk::Subject &hintSig() { return m_hintsig; }
    const FbTk::Subject &hintSig() const { return m_hintsig; }
    FbTk::Subject &workspaceSig() { return m_workspacesig; }
    const FbTk::Subject &workspaceSig() const { return m_workspacesig; }
    FbTk::Subject &dieSig() { return m_diesig; }
    const FbTk::Subject &dieSig() const { return m_diesig; }
    FbTk::Subject &focusSig() { return m_focussig; }
    FbTk::Subject &titleSig() { return m_titlesig; }
    /** @} */ // end group signals

    void reconfigTheme();

    const timeval &lastFocusTime() const { return m_last_focus_time;}

    //@}

    class WinSubject: public FbTk::Subject {
    public:
        WinSubject(FluxboxWindow &w):m_win(w) { }
        FluxboxWindow &win() { return m_win; }
        const FluxboxWindow &win() const { return m_win; }
    private:
        FluxboxWindow &m_win;
    };

    bool oplock; ///< Used to help stop transient loops occurring by locking a window during certain operations

private:
    static const int PropBlackboxAttributesElements = 8;

    void setupWindow();
    void setupMenu();

    void init();
    /// applies a shape mask to the window if it has one
    void shape();
    void updateClientLeftWindow();
    void grabButtons();

    void startMoving(Window win);
    void stopMoving(bool interrupted = false);
    void startResizing(Window win, int x, int y);
    void stopResizing(bool interrupted = false);
    /// try to attach current attaching client to a window at pos x, y
    void attachTo(int x, int y, bool interrupted = false);

    bool getState();
    /// gets title string from client window and updates frame's title
    void updateTitleFromClient(WinClient &client);
    /// gets icon name from client window
    void updateIconNameFromClient(WinClient &client);
    void updateMWMHintsFromClient(WinClient &client);
    void updateBlackboxHintsFromClient(const WinClient &client);
    void updateRememberStateFromClient(WinClient &client);
    void saveBlackboxAttribs();
    void associateClientWindow(bool use_attrs = false, int x = 0, int y = 0, unsigned int width = 1, unsigned int height = 1);

    void restoreGravity();
    void setGravityOffsets();
    void setState(unsigned long stateval, bool setting_up);

    // modifies left and top if snap is necessary
    void doSnapping(int &left, int &top);
    // user_w/h return the values that should be shown to the user
    void fixsize(int *user_w = 0, int *user_h = 0);
    void moveResizeClient(WinClient &client, int x, int y, unsigned int width, unsigned int height);
    /// sends configurenotify to all clients
    void sendConfigureNotify(bool send_to_netizens = true);

    static void grabPointer(Window grab_window,
                     Bool owner_events,
                     unsigned int event_mask,
                     int pointer_mode, int keyboard_mode,
                     Window confine_to,
                     Cursor cursor,
                     Time time);
    static void ungrabPointer(Time time);

    // state and hint signals
    WinSubject m_hintsig, m_statesig, m_layersig, m_workspacesig, m_diesig, m_focussig, m_titlesig;

    class ThemeListener: public FbTk::Observer {
    public:
        ThemeListener(FluxboxWindow &win):m_win(win) { }
        void update(FbTk::Subject *) {
            m_win.reconfigTheme();
        }
    private:
        FluxboxWindow &m_win;
    };
    ThemeListener m_themelistener;

    // Window states
    bool moving, resizing, shaded, iconic,
        focused, stuck, m_managed;

    int maximized;

    WinClient *m_attaching_tab;

    BScreen &m_screen; /// screen on which this window exist
    FbTk::Timer m_timer;
    Display *display; /// display connection
    BlackboxAttributes m_blackbox_attrib;

    std::auto_ptr<FbTk::Menu> m_windowmenu;

    timeval m_last_focus_time;

    int m_button_grab_x, m_button_grab_y; // handles last button press event for move
    int m_last_resize_x, m_last_resize_y; // handles last button press event for resize
    int m_last_move_x, m_last_move_y; // handles last pos for non opaque moving
    unsigned int m_last_resize_h, m_last_resize_w; // handles height/width for resize "window"

    unsigned int m_workspace_number;
    unsigned long m_current_state;

    Decoration m_old_decoration;

    ClientList m_clientlist;
    WinClient *m_client; ///< current client
    typedef std::map<WinClient *, FbTk::TextButton *> Client2ButtonMap;
    Client2ButtonMap m_labelbuttons;

    // just temporary solution
    friend class WinClient;

    struct _decorations {
        bool titlebar, handle, border, iconify,
            maximize, close, menu, sticky, shade, tab, enabled;
    } decorations;

    bool m_toggled_decos; 

    struct _functions {
        bool resize, move, iconify, maximize, close, tabable;
    } functions;

    bool m_shaped; ///< if the window is shaped with a mask
    bool m_icon_hidden;  ///< if the window is in the iconbar 
    int m_old_pos_x, m_old_pos_y; ///< old position so we can restore from maximized
    unsigned int m_old_width, m_old_height; ///< old size so we can restore from maximized state
    int m_last_button_x, ///< last known x position of the mouse button
        m_last_button_y; ///< last known y position of the mouse button
    FbWinFrame m_frame;

    FbTk::XLayerItem m_layeritem;
    int m_layernum;

    FbTk::FbWindow &m_parent; ///< window on which we draw move/resize rectangle  (the "root window")

    ResizeCorner m_resize_corner;

    ExtraMenus m_extramenus;
    static int s_num_grabs; ///< number of XGrabPointer's
};


#endif // WINDOW_HH
