// Ewmh.cc for fluxbox
// Copyright (c) 2002 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#include "Ewmh.hh"

#include "Screen.hh"
#include "Window.hh"
#include "WinClient.hh"
#include "Workspace.hh"
#include "Layer.hh"
#include "WinClientUtil.hh"
#include "fluxbox.hh"

#include "FbTk/App.hh"
#include "FbTk/FbWindow.hh"
#include "FbTk/I18n.hh"
#include "FbTk/XLayerItem.hh"
#include "FbTk/XLayer.hh"

#include <iostream>
#include <algorithm>
#include <new>

using namespace std;

// mipspro has no new(nothrow)
#if defined sgi && ! defined GCC
#define FB_new_nothrow new
#else
#define FB_new_nothrow new(std::nothrow)
#endif

enum EwmhMoveResizeDirection {
    _NET_WM_MOVERESIZE_SIZE_TOPLEFT    =   0,
    _NET_WM_MOVERESIZE_SIZE_TOP        =   1,
    _NET_WM_MOVERESIZE_SIZE_TOPRIGHT   =   2,
    _NET_WM_MOVERESIZE_SIZE_RIGHT      =   3,
    _NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT =  4,
    _NET_WM_MOVERESIZE_SIZE_BOTTOM      =  5,
    _NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT  =  6,
    _NET_WM_MOVERESIZE_SIZE_LEFT        =  7,
    _NET_WM_MOVERESIZE_MOVE             =  8,   // movement only 
    _NET_WM_MOVERESIZE_SIZE_KEYBOARD    =  9,   // size via keyboard
    _NET_WM_MOVERESIZE_MOVE_KEYBOARD    = 10 
};

Ewmh::Ewmh() {
    createAtoms();
}

Ewmh::~Ewmh() {
    while (!m_windows.empty()) {
        XDestroyWindow(FbTk::App::instance()->display(), m_windows.back());
        m_windows.pop_back();
    }
}

void Ewmh::initForScreen(BScreen &screen) {
    Display *disp = FbTk::App::instance()->display();

    /* From Extended Window Manager Hints, draft 1.3:
     *
     * _NET_SUPPORTING_WM_CHECK
     * 
     * The Window Manager MUST set this property on the root window 
     * to be the ID of a child window created by himself, to indicate 
     * that a compliant window manager is active. The child window 
     * MUST also have the _NET_SUPPORTING_WM_CHECK property set to 
     * the ID of the child window. The child window MUST also have 
     * the _NET_WM_NAME property set to the name of the Window Manager.
     * 
     * Rationale: The child window is used to distinguish an active 
     * Window Manager from a stale _NET_SUPPORTING_WM_CHECK property 
     * that happens to point to another window. If the 
     * _NET_SUPPORTING_WM_CHECK window on the client window is missing 
     * or not properly set, clients SHOULD assume that no conforming 
     * Window Manager is present.
     */

    Window wincheck = XCreateSimpleWindow(disp,
                                          screen.rootWindow().window(),
                                          -10, -10, 5, 5, 0, 0, 0);

    if (wincheck != None) {
        // store the window so we can delete it later
        m_windows.push_back(wincheck);

        screen.rootWindow().changeProperty(m_net_supporting_wm_check, XA_WINDOW, 32,
                                           PropModeReplace, (unsigned char *) &wincheck, 1);
        XChangeProperty(disp, wincheck, m_net_supporting_wm_check, XA_WINDOW, 32,
                        PropModeReplace, (unsigned char *) &wincheck, 1);

        XChangeProperty(disp, wincheck, m_net_wm_name, utf8_string, 8,
                        PropModeReplace, (unsigned char *) "Fluxbox", strlen("Fluxbox"));
    }

    //set supported atoms
    Atom atomsupported[] = {
        // window properties
        m_net_wm_strut,
        m_net_wm_state,
        m_net_wm_name,
        m_net_wm_icon_name,

        // states that we support:
        m_net_wm_state_sticky,
        m_net_wm_state_shaded,
        m_net_wm_state_maximized_horz,
        m_net_wm_state_maximized_vert,
        m_net_wm_state_fullscreen,
        m_net_wm_state_hidden,
        m_net_wm_state_skip_taskbar,
        m_net_wm_state_modal,
        m_net_wm_state_below,
        m_net_wm_state_above,
        m_net_wm_state_demands_attention,
        
        // window type
        m_net_wm_window_type,
        m_net_wm_window_type_dock,
        m_net_wm_window_type_desktop,
        m_net_wm_window_type_splash,
        m_net_wm_window_type_dialog,
        m_net_wm_window_type_normal,

        // window actions
        m_net_wm_allowed_actions,
        m_net_wm_action_move,
        m_net_wm_action_resize,
        m_net_wm_action_minimize,
        m_net_wm_action_shade,
        m_net_wm_action_stick,
        m_net_wm_action_maximize_horz,
        m_net_wm_action_maximize_vert,
        m_net_wm_action_fullscreen,
        m_net_wm_action_change_desktop,
        m_net_wm_action_close,

        // root properties
        m_net_client_list,
        m_net_client_list_stacking,
        m_net_number_of_desktops,
        m_net_current_desktop,
        m_net_active_window,
        m_net_close_window,
        m_net_moveresize_window,
        m_net_workarea,
        m_net_restack_window,

        m_net_wm_moveresize,
        
        m_net_frame_extents,

        // desktop properties
        m_net_wm_desktop,
        m_net_desktop_names,
        m_net_desktop_viewport,
        m_net_desktop_geometry,

        m_net_supporting_wm_check
    };
    /* From Extended Window Manager Hints, draft 1.3:
     *
     * _NET_SUPPORTED, ATOM[]/32
     *
     * This property MUST be set by the Window Manager
     * to indicate which hints it supports. For
     * example: considering _NET_WM_STATE both this
     * atom and all supported states
     * e.g. _NET_WM_STATE_MODAL, _NET_WM_STATE_STICKY,
     * would be listed. This assumes that backwards
     * incompatible changes will not be made to the
     * hints (without being renamed).
     */
    screen.rootWindow().changeProperty(m_net_supported, XA_ATOM, 32,
                                       PropModeReplace,
                                       (unsigned char *) &atomsupported,
                                       (sizeof atomsupported)/sizeof atomsupported[0]);

    // update atoms

    updateWorkspaceCount(screen);
    updateCurrentWorkspace(screen);
    updateWorkspaceNames(screen);
    updateClientList(screen);
    updateViewPort(screen);
    updateGeometry(screen);
    updateWorkarea(screen);

}

void Ewmh::setupClient(WinClient &winclient) {
    updateStrut(winclient);

    FbTk::FbString newtitle = winclient.textProperty(m_net_wm_name);
    if (!newtitle.empty()) {
        winclient.setTitle(newtitle);
    }
    newtitle = winclient.textProperty(m_net_wm_icon_name);
    if (!newtitle.empty()) {
        winclient.setIconTitle(newtitle);
    }

    if ( winclient.fbwindow() )
        winclient.fbwindow()->titleSig().notify();
}

void Ewmh::setupFrame(FluxboxWindow &win) {
    Atom ret_type;
    int fmt;
    unsigned long nitems, bytes_after;
    unsigned char *data = 0;

    /* From Extended Window Manager Hints, draft 1.3:
     *
     * _NET_WM_WINDOW_TYPE, ATOM[]/32
     *
     * This SHOULD be set by the Client before mapping to a list of atoms
     * indicating the functional type of the window. This property SHOULD
     * be used by the window manager in determining the decoration,
     * stacking position and other behavior of the window. The Client
     * SHOULD specify window types in order of preference (the first being
     * most preferable) but MUST include at least one of the basic window
     * type atoms from the list below. This is to allow for extension of
     * the list of types whilst providing default behavior for Window
     * Managers that do not recognize the extensions.
     *
     */

    win.winClient().property(m_net_wm_window_type, 0, 0x7fffffff, False, XA_ATOM,
                             &ret_type, &fmt, &nitems, &bytes_after,
                             &data);
    if (data) {
        Atom *atoms = (unsigned long *)data;
        for (unsigned long l = 0; l < nitems; ++l) {
            /* From Extended Window Manager Hints, draft 1.3:
             *
             * _NET_WM_WINDOW_TYPE_DOCK indicates a dock or panel feature.
             * Typically a Window Manager would keep such windows on top
             * of all other windows.
             *
             */
            if (atoms[l] == m_net_wm_window_type_dock) {
                // we also assume it shouldn't be visible in any toolbar
                win.setFocusHidden(true);
                win.setIconHidden(true);
            } else if (atoms[l] == m_net_wm_window_type_desktop) {
                /*
                 * _NET_WM_WINDOW_TYPE_DESKTOP indicates a "false desktop" window
                 * We let it be the size it wants, but it gets no decoration,
                 * is hidden in the toolbar and window cycling list, plus
                 * windows don't tab with it and is right on the bottom.
                 */

                win.setFocusHidden(true);
                win.setIconHidden(true);
                win.moveToLayer(Layer::DESKTOP);
                win.setDecorationMask(0);
                win.setTabable(false);
                win.setMovable(false);
                win.setResizable(false);
                win.stick();

            } else if (atoms[l] == m_net_wm_window_type_splash) {
                /*
                 * _NET_WM_WINDOW_TYPE_SPLASH indicates that the 
                 * window is a splash screen displayed as an application 
                 * is starting up.
                 */
                win.setDecoration(FluxboxWindow::DECOR_NONE);
                win.setFocusHidden(true);
                win.setIconHidden(true);
                win.setMovable(false);
            } else if (atoms[l] == m_net_wm_window_type_normal) {
                // do nothing, this is ..normal..
            } else if (atoms[l] == m_net_wm_window_type_dialog) {
                // dialog windows should not be tabable
                win.setTabable(false);
            }

        }
        XFree(data);
    } else {
        // if _NET_WM_WINDOW_TYPE not set and this window 
        // has transient_for the type must be set to _NET_WM_WINDOW_TYPE_DIALOG 
        if ( win.winClient().isTransient() ) {
            win.winClient().
                changeProperty(m_net_wm_window_type, 
                               XA_ATOM, 32, PropModeReplace,
                               (unsigned char*)&m_net_wm_window_type_dialog, 1);
        }
    }

    setupState(win);

    if (win.winClient().property(m_net_wm_desktop, 0, 1, False, XA_CARDINAL,
                                 &ret_type, &fmt, &nitems, &bytes_after,
                                 (unsigned char **) &data) && data) {
        unsigned int desktop = static_cast<long>(*data);
        if (desktop == (unsigned int)(-1) && !win.isStuck())
            win.stick();
        else
            win.setWorkspace(desktop);

        XFree(data);
    } else {
        updateWorkspace(win);
    }

    updateFrameExtents(win);

}

void Ewmh::updateFrameClose(FluxboxWindow &win) {
    clearState(win);
}

void Ewmh::updateFocusedWindow(BScreen &screen, Window win) {
    /* From Extended Window Manager Hints, draft 1.3:
     *
     * _NET_ACTIVE_WINDOW, WINDOW/32
     *
     * The window ID of the currently active window or None
     * if no window has the focus. This is a read-only
     * property set by the Window Manager.
     *
     */
    screen.rootWindow().changeProperty(m_net_active_window,
                                       XA_WINDOW, 32,
                                       PropModeReplace,
                                       (unsigned char *)&win, 1);
}

void Ewmh::updateClientClose(WinClient &winclient){
    updateClientList(winclient.screen());
}

void Ewmh::updateClientList(BScreen &screen) {
    size_t num=0;

    BScreen::Workspaces::const_iterator workspace_it =
        screen.getWorkspacesList().begin();
    const BScreen::Workspaces::const_iterator workspace_it_end =
        screen.getWorkspacesList().end();
    for (; workspace_it != workspace_it_end; ++workspace_it) {
        Workspace::Windows::iterator win_it =
            (*workspace_it)->windowList().begin();
        Workspace::Windows::iterator win_it_end =
            (*workspace_it)->windowList().end();
        for (; win_it != win_it_end; ++win_it) {
            num += (*win_it)->numClients();
        }

    }
    // and count icons
    BScreen::Icons::const_iterator icon_it = screen.iconList().begin();
    BScreen::Icons::const_iterator icon_it_end = screen.iconList().end();
    for (; icon_it != icon_it_end; ++icon_it) {
        num += (*icon_it)->numClients();
    }

    Window *wl = FB_new_nothrow Window[num];
    if (wl == 0) {
        _FB_USES_NLS;
        cerr<<_FB_CONSOLETEXT(Ewmh, OutOfMemoryClientList, 
                      "Fatal: Out of memory, can't allocate for EWMH client list", "")<<endl;
        return;
    }

    //start the iterator from begining
    workspace_it = screen.getWorkspacesList().begin();
    int win=0;
    for (; workspace_it != workspace_it_end; ++workspace_it) {

        // Fill in array of window ID's
        Workspace::Windows::const_iterator it =
            (*workspace_it)->windowList().begin();
        Workspace::Windows::const_iterator it_end =
            (*workspace_it)->windowList().end();
        for (; it != it_end; ++it) {
            if ((*it)->numClients() == 1) {
                wl[win++] = (*it)->clientWindow();
            } else {
                // add every client in fluxboxwindow to list window list
                std::list<WinClient *>::iterator client_it =
                    (*it)->clientList().begin();
                std::list<WinClient *>::iterator client_it_end =
                    (*it)->clientList().end();
                for (; client_it != client_it_end; ++client_it)
                    wl[win++] = (*client_it)->window();
            }
        }
    }

    // plus iconified windows
    icon_it = screen.iconList().begin();
    for (; icon_it != icon_it_end; ++icon_it) {
        FluxboxWindow::ClientList::iterator client_it = (*icon_it)->clientList().begin();
        FluxboxWindow::ClientList::iterator client_it_end = (*icon_it)->clientList().end();
        for (; client_it != client_it_end; ++client_it)
            wl[win++] = (*client_it)->window();
    }
    //number of windows to show in client list
    num = win;

    /*  From Extended Window Manager Hints, draft 1.3:
     *
     * _NET_CLIENT_LIST, WINDOW[]/32
     * _NET_CLIENT_LIST_STACKING, WINDOW[]/32
     *
     * These arrays contain all X Windows managed by
     * the Window Manager. _NET_CLIENT_LIST has
     * initial mapping order, starting with the oldest
     * window. _NET_CLIENT_LIST_STACKING has
     * bottom-to-top stacking order. These properties
     * SHOULD be set and updated by the Window
     * Manager.
     */
    screen.rootWindow().changeProperty(m_net_client_list,
                                       XA_WINDOW, 32,
                                       PropModeReplace, (unsigned char *)wl, num);
    screen.rootWindow().changeProperty(m_net_client_list_stacking,
                                       XA_WINDOW, 32,
                                       PropModeReplace, (unsigned char *)wl, num);

    delete [] wl;
}

void Ewmh::updateWorkspaceNames(BScreen &screen) {
    /* From Extended Window Manager Hints, draft 1.3:
     *
     * _NET_DESKTOP_NAMES, UTF8_STRING[]
     *
     * The names of all virtual desktops.
     * This is a list of NULL-terminated strings in UTF-8
     * encoding [UTF8]. This property MAY be changed by a
     * Pager or the Window Manager at any time.
     *
     * Note: The number of names could be different from
     * _NET_NUMBER_OF_DESKTOPS. If it is less than
     * _NET_NUMBER_OF_DESKTOPS, then the desktops with high
     *  numbers are unnamed. If it is larger than
     * _NET_NUMBER_OF_DESKTOPS, then the excess names outside
     * of the _NET_NUMBER_OF_DESKTOPS are considered to be
     * reserved in case the number of desktops is increased.
     *
     * Rationale: The name is not a necessary attribute of a
     * virtual desktop. Thus the availability or unavailability
     * of names has no impact on virtual desktop functionality.
     * Since names are set by users and users are likely to
     * preset names for a fixed number of desktops, it
     * doesn't make sense to shrink or grow this list when the
     * number of available desktops changes.
     *
     */
    XTextProperty text;
    const BScreen::WorkspaceNames &workspacenames = screen.getWorkspaceNames();
    const size_t number_of_desks = workspacenames.size();

    char *names[number_of_desks];

    for (size_t i = 0; i < number_of_desks; i++) {
        names[i] = new char[workspacenames[i].size() + 1]; // +1 for \0
        memset(names[i], 0, workspacenames[i].size());
        strcpy(names[i], workspacenames[i].c_str());
    }

#ifdef X_HAVE_UTF8_STRING
    Xutf8TextListToTextProperty(FbTk::App::instance()->display(),
                                names, number_of_desks, XUTF8StringStyle, &text);
    XSetTextProperty(FbTk::App::instance()->display(), screen.rootWindow().window(),
                     &text, m_net_desktop_names);

    XFree(text.value);

#else
    if (XStringListToTextProperty(names, number_of_desks, &text)) {
        XSetTextProperty(FbTk::App::instance()->display(), screen.rootWindow().window(),
			 &text, m_net_desktop_names);
        XFree(text.value);
    }
#endif

    for (size_t i = 0; i < number_of_desks; i++)
        delete [] names[i];

}

void Ewmh::updateCurrentWorkspace(BScreen &screen) {
    /* From Extended Window Manager Hints, draft 1.3:
     *
     * _NET_CURRENT_DESKTOP desktop, CARDINAL/32
     *
     * The index of the current desktop. This is always
     * an integer between 0 and _NET_NUMBER_OF_DESKTOPS - 1.
     * This MUST be set and updated by the Window Manager.
     *
     */
    unsigned long workspace = screen.currentWorkspaceID();
    screen.rootWindow().changeProperty(m_net_current_desktop,
                                       XA_CARDINAL, 32,
                                       PropModeReplace,
                                       (unsigned char *)&workspace, 1);

}

void Ewmh::updateWorkspaceCount(BScreen &screen) {
    /* From Extended Window Manager Hints, draft 1.3:
     *
     * _NET_NUMBER_OF_DESKTOPS, CARDINAL/32
     *
     * This property SHOULD be set and updated by the
     * Window Manager to indicate the number of virtual
     * desktops.
     */
    unsigned long numworkspaces = screen.numberOfWorkspaces();
    screen.rootWindow().changeProperty(m_net_number_of_desktops,
                                       XA_CARDINAL, 32,
                                       PropModeReplace,
                                       (unsigned char *)&numworkspaces, 1);
}

void Ewmh::updateViewPort(BScreen &screen) {
    /* From Extended Window Manager Hints, draft 1.3:
     *
     * _NET_DESKTOP_VIEWPORT x, y, CARDINAL[][2]/32
     *
     * Array of pairs of cardinals that define the
     * top left corner of each desktop's viewport.
     * For Window Managers that don't support large
     * desktops, this MUST always be set to (0,0).
     *
     */
    long value[2] = {0, 0}; // we dont support large desktops
    screen.rootWindow().changeProperty(m_net_desktop_viewport,
                                       XA_CARDINAL, 32,
                                       PropModeReplace,
                                       (unsigned char *)value, 2);
}

void Ewmh::updateGeometry(BScreen &screen) {
    /* From Extended Window Manager Hints, draft 1.3:
     *
     * _NET_DESKTOP_GEOMETRY width, height, CARDINAL[2]/32
     *
     * Array of two cardinals that defines the common size
     * of all desktops (this is equal to the screen size
     * if the Window Manager doesn't support large
     * desktops, otherwise it's equal to the virtual size
     * of the desktop). This property SHOULD be set by the
     * Window Manager.
     *
     */
    long value[2] = {screen.width(), screen.height()};
    screen.rootWindow().changeProperty(m_net_desktop_geometry,
                                       XA_CARDINAL, 32,
                                       PropModeReplace,
                                       (unsigned char *)value, 2);

}

void Ewmh::updateWorkarea(BScreen &screen) {
    /* From Extended Window Manager Hints, draft 1.3:
     *
     * _NET_WORKAREA, x, y, width, height CARDINAL[][4]/32
     *
     * This property MUST be set by the Window Manager upon
     * calculating the work area for each desktop. Contains a
     * geometry for each desktop. These geometries are
     * specified relative to the viewport on each desktop and
     * specify an area that is completely contained within the
     * viewport. Work area SHOULD be used by desktop applications
     * to place desktop icons appropriately.
     *
     */

    /* !!TODO
     * Not sure how to handle xinerama stuff here.
     * So i'm just doing this on the first head.
     */
    unsigned long *coords = new unsigned long[4*screen.numberOfWorkspaces()];
    for (unsigned int i=0; i < screen.numberOfWorkspaces()*4; i+=4) {
        // x, y
        coords[i] = screen.maxLeft(0);
        coords[i + 1] = screen.maxTop(0);
        // width, height
        coords[i + 2] = screen.maxRight(0) - screen.maxLeft(0);
        coords[i + 3] = screen.maxBottom(0) - screen.maxTop(0);

    }
    screen.rootWindow().changeProperty(m_net_workarea,
                                       XA_CARDINAL, 32,
                                       PropModeReplace,
                                       (unsigned char *)coords,
                                       4 * screen.numberOfWorkspaces());

    delete[] coords;
}

void Ewmh::updateState(FluxboxWindow &win) {
    

    updateActions(win);
    
    typedef std::vector<unsigned int> StateVec;

    StateVec state;

    if (win.isStuck())
        state.push_back(m_net_wm_state_sticky);
    if (win.isShaded())
        state.push_back(m_net_wm_state_shaded);
    if (win.layerNum() == Layer::BOTTOM)
        state.push_back(m_net_wm_state_below);
    if (win.layerNum() == Layer::ABOVE_DOCK)
        state.push_back(m_net_wm_state_above);
    if (win.isIconic())
        state.push_back(m_net_wm_state_hidden);
    if (win.isIconHidden())
        state.push_back(m_net_wm_state_skip_taskbar);
    if (win.isFullscreen())
        state.push_back(m_net_wm_state_fullscreen);
    if (win.winClient().isModal())
        state.push_back(m_net_wm_state_modal);

    FluxboxWindow::ClientList::iterator it = win.clientList().begin();
    FluxboxWindow::ClientList::iterator it_end = win.clientList().end();
    for (; it != it_end; ++it) {
        
        // search the old states for _NET_WM_STATE_SKIP_PAGER and append it
        // to the current state, so it wont get deleted by us.
        StateVec client_state(state);
        Atom ret_type;
        int fmt;
        unsigned long nitems, bytes_after;
        unsigned char *data = 0;

        (*it)->property(m_net_wm_state, 0, 0x7fffffff, False, XA_ATOM,
                                 &ret_type, &fmt, &nitems, &bytes_after,
                                 &data);
        if (data) {
            Atom *old_states = (Atom *)data;
            for (unsigned long i=0; i < nitems; ++i) {
                if (old_states[i] == m_net_wm_state_skip_pager) {
                    client_state.push_back(m_net_wm_state_skip_pager);
                }
            }
            XFree(data);
        }

        if (!client_state.empty()) {
            (*it)->changeProperty(m_net_wm_state, XA_ATOM, 32, PropModeReplace,
                                  reinterpret_cast<unsigned char*>(&client_state.front()), 
                                  client_state.size());
        } else
            (*it)->deleteProperty(m_net_wm_state);
    }
}

void Ewmh::updateLayer(FluxboxWindow &win) {
    //!! TODO _NET_WM_WINDOW_TYPE
    /*
    if (win.getLayer() == Fluxbox::instance()->getAboveDockLayer()) {
        // _NET_WM_STATE_BELOW

    } else if (win.getLayer() == Fluxbox::instance()->getBottomLayer()) {
        // _NET_WM_STATE_ABOVE
    }
    */
}

void Ewmh::updateHints(FluxboxWindow &win) {
}

void Ewmh::updateWorkspace(FluxboxWindow &win) {
    long workspace = win.isInitialized() ?
        win.workspaceNumber() : 
        win.screen().currentWorkspaceID();

    if (win.isStuck())
        workspace = -1; // appear on all desktops/workspaces

    FluxboxWindow::ClientList::iterator it = win.clientList().begin();
    FluxboxWindow::ClientList::iterator it_end = win.clientList().end();
    for (; it != it_end; ++it) {
        (*it)->changeProperty(m_net_wm_desktop, XA_CARDINAL, 32, PropModeReplace,
                              (unsigned char *)&workspace, 1);
    }

}


// return true if we did handle the atom here
bool Ewmh::checkClientMessage(const XClientMessageEvent &ce,
                              BScreen * screen, WinClient * const winclient) {

    if (ce.message_type == m_net_wm_desktop) {
        // ce.data.l[0] = workspace number
        // valid window

        if (winclient == 0 || winclient->fbwindow() == 0)
            return true;

        FluxboxWindow *fbwin = winclient->fbwindow();

        // if it's stick, make sure it is stuck.
        // otherwise, make sure it isn't stuck
        if (ce.data.l[0] == -1) {
            if (!fbwin->isStuck())
                fbwin->stick();
            return true;
        } else if (fbwin->isStuck())
            fbwin->stick();

        // the screen is the root window of the message,
        // which doesn't apply here (so borrow the variable :) )
        screen = &fbwin->screen();
        // valid workspace number?
        if (static_cast<unsigned int>
            (ce.data.l[0]) < screen->numberOfWorkspaces())
            screen->sendToWorkspace(ce.data.l[0], fbwin, false);

        return true;
    } else if (ce.message_type == m_net_wm_state) {
        if (winclient == 0 || winclient->fbwindow() == 0)
            return true;

        FluxboxWindow &win = *winclient->fbwindow();
        // ce.data.l[0] = the action (remove, add or toggle)
        // ce.data.l[1] = the first property to alter
        // ce.data.l[2] = second property to alter (can be zero)
        if (ce.data.l[0] == STATE_REMOVE) {
            setState(win, ce.data.l[1], false);
            setState(win, ce.data.l[2], false);
        } else if (ce.data.l[0] == STATE_ADD) {
            setState(win, ce.data.l[1], true);
            setState(win, ce.data.l[2], true);
        } else if (ce.data.l[0] == STATE_TOGGLE) {
            toggleState(win, ce.data.l[1]);
            toggleState(win, ce.data.l[2]);
        }
        return true;
    } else if (ce.message_type == m_net_number_of_desktops) {
        if (screen == 0)
            return true;
        // ce.data.l[0] = number of workspaces

        // no need to alter number of desktops if they are the same
        // or if requested number of workspace is less than zero
        if (screen->numberOfWorkspaces() == static_cast<unsigned int>(ce.data.l[0]) ||
            ce.data.l[0] < 0)
            return true;

        if (screen->numberOfWorkspaces() > static_cast<unsigned int>(ce.data.l[0])) {
            // remove last workspace until we have
            // the same number of workspaces
            while (screen->numberOfWorkspaces() != static_cast<unsigned int>(ce.data.l[0])) {
                screen->removeLastWorkspace();
                if (screen->numberOfWorkspaces() == 1) // must have at least one workspace
                    break;
            }
        } else { // add workspaces to screen until workspace count match the requested size
            while (screen->numberOfWorkspaces() != static_cast<unsigned int>(ce.data.l[0])) {
                screen->addWorkspace();
            }
        }

        return true;
    } else if (ce.message_type == m_net_current_desktop) {
        if (screen == 0)
            return true;
        // ce.data.l[0] = workspace number

        // prevent out of range value
        if (static_cast<unsigned int>(ce.data.l[0]) >= screen->numberOfWorkspaces())
            return true;
        screen->changeWorkspaceID(ce.data.l[0]);
        return true;
    } else if (ce.message_type == m_net_active_window) {

        // make sure we have a valid window
        if (winclient == 0)
            return true;
        // ce.window = window to focus

        winclient->focus();
        if (winclient->fbwindow()) {

            FluxboxWindow* fbwin = winclient->fbwindow();
            fbwin->raise();

            // if the raised window is on a different workspace
            // we do what the user wish: 
            //   either ignore|go to that workspace|get the window
            if (fbwin->screen().currentWorkspaceID() != fbwin->workspaceNumber()) {
                if (fbwin->screen().getFollowModel() == BScreen::FOLLOW_ACTIVE_WINDOW) {
                    fbwin->screen().changeWorkspaceID(fbwin->workspaceNumber());
                } else if (fbwin->screen().getFollowModel() == BScreen::FETCH_ACTIVE_WINDOW) {
                    fbwin->screen().sendToWorkspace(fbwin->screen().currentWorkspaceID(), fbwin);
                } // else we ignore it. my favourite mode :)
            }
        } 
        return true;
    } else if (ce.message_type == m_net_close_window) {
        if (winclient == 0)
            return true;
        // ce.window = window to close (which in this case is the win argument)
        winclient->sendClose();
        return true;
    } else if (ce.message_type == m_net_moveresize_window) {
        if (winclient == 0 || winclient->fbwindow() == 0)
            return true;
        // ce.data.l[0] = gravity and flags
        // ce.data.l[1] = x
        // ce.data.l[2] = y
        // ce.data.l[3] = width
        // ce.data.l[4] = height
        // TODO: flags
        int win_gravity=ce.data.l[0] & 0xFF;
        winclient->fbwindow()->moveResizeForClient(ce.data.l[1], ce.data.l[2],
                                          ce.data.l[3], ce.data.l[4], win_gravity);
        return true;
    } else if (ce.message_type == m_net_restack_window) {
#ifndef DEBUG
        cerr << "Ewmh: restack window" << endl;
#endif // DEBUG
        if (winclient == 0 || winclient->fbwindow() == 0)
            return true;

        // ce.data.l[0] = source indication
        // ce.data.l[1] = sibling window
        // ce.data.l[2] = detail
        

        WinClient *above_win = Fluxbox::instance()->searchWindow(ce.data.l[1]);
        if (above_win == 0 || above_win->fbwindow() == 0 ||
            above_win == winclient) // this would be very wrong :)
            return true;

        FbTk::XLayerItem &below_item = winclient->fbwindow()->layerItem();
        FbTk::XLayerItem &above_item = above_win->fbwindow()->layerItem();

        // this might break the transient_for layering

        // do restack if both items are on the same layer
        // else ignore restack
        if (&below_item.getLayer() == &above_item.getLayer())
            below_item.getLayer().stackBelowItem(&below_item, &above_item);


        return true;

    } else if (ce.message_type == m_net_wm_moveresize) {
        if (winclient == 0 || winclient->fbwindow() == 0)
            return true;
        // data.l[0] = x_root 
        // data.l[1] = y_root
        // data.l[2] = direction
        // data.l[3] = button
        // data.l[4] = source indication
        switch (ce.data.l[2] ) {
        case _NET_WM_MOVERESIZE_SIZE_TOPLEFT:
        case _NET_WM_MOVERESIZE_SIZE_TOP:
        case _NET_WM_MOVERESIZE_SIZE_TOPRIGHT:
        case _NET_WM_MOVERESIZE_SIZE_RIGHT:
        case _NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT:
        case _NET_WM_MOVERESIZE_SIZE_BOTTOM:
        case _NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT:
        case _NET_WM_MOVERESIZE_SIZE_LEFT:
        case _NET_WM_MOVERESIZE_SIZE_KEYBOARD:
            winclient->fbwindow()->startResizing(ce.data.l[0], ce.data.l[1],
                                                 static_cast<FluxboxWindow::ResizeDirection>
                                                 (ce.data.l[2]));
            break;
        case _NET_WM_MOVERESIZE_MOVE:
        case _NET_WM_MOVERESIZE_MOVE_KEYBOARD:
            winclient->fbwindow()->startMoving(ce.data.l[0], ce.data.l[1]);
            break;
        default:
            cerr << "Ewmh: Unknown move/resize direction: " << ce.data.l[2] << endl;
            break;
        }
        return true;
    }

    // we didn't handle the ce.message_type here
    return false;
}


bool Ewmh::propertyNotify(WinClient &winclient, Atom the_property) {
    if (the_property == m_net_wm_strut) {
        updateStrut(winclient);
        return true;
    } else if (the_property == m_net_wm_name) {
        FbTk::FbString newtitle = winclient.textProperty(the_property);
        if (!newtitle.empty())
            winclient.setTitle(newtitle);
        if (winclient.fbwindow())
            winclient.fbwindow()->titleSig().notify();
        return true;
    } else if (the_property == m_net_wm_icon_name) {
        FbTk::FbString newtitle = winclient.textProperty(the_property);
        if (!newtitle.empty())
            winclient.setIconTitle(newtitle);
        if (winclient.fbwindow())
            winclient.fbwindow()->titleSig().notify();
        return true;
    }

    return false;
}

void Ewmh::createAtoms() {

    Display *disp = FbTk::App::instance()->display();

    m_net_supported = XInternAtom(disp, "_NET_SUPPORTED", False);
    m_net_client_list = XInternAtom(disp, "_NET_CLIENT_LIST", False);
    m_net_client_list_stacking = XInternAtom(disp, "_NET_CLIENT_LIST_STACKING", False);
    m_net_number_of_desktops = XInternAtom(disp, "_NET_NUMBER_OF_DESKTOPS", False);
    m_net_desktop_geometry = XInternAtom(disp, "_NET_DESKTOP_GEOMETRY", False);
    m_net_desktop_viewport = XInternAtom(disp, "_NET_DESKTOP_VIEWPORT", False);
    m_net_current_desktop = XInternAtom(disp, "_NET_CURRENT_DESKTOP", False);
    m_net_desktop_names = XInternAtom(disp, "_NET_DESKTOP_NAMES", False);
    m_net_active_window = XInternAtom(disp, "_NET_ACTIVE_WINDOW", False);
    m_net_workarea = XInternAtom(disp, "_NET_WORKAREA", False);
    m_net_supporting_wm_check = XInternAtom(disp, "_NET_SUPPORTING_WM_CHECK", False);
    m_net_virtual_roots = XInternAtom(disp, "_NET_VIRTUAL_ROOTS", False);

    m_net_close_window = XInternAtom(disp, "_NET_CLOSE_WINDOW", False);
    m_net_moveresize_window = XInternAtom(disp, "_NET_MOVERESIZE_WINDOW", False);
    m_net_restack_window = XInternAtom(disp, "_NET_RESTACK_WINDOW", False);


    m_net_wm_moveresize = XInternAtom(disp, "_NET_WM_MOVERESIZE", False);

    m_net_properties = XInternAtom(disp, "_NET_PROPERTIES", False);
    m_net_wm_name = XInternAtom(disp, "_NET_WM_NAME", False);
    m_net_wm_icon_name = XInternAtom(disp, "_NET_WM_ICON_NAME", False);
    m_net_wm_desktop = XInternAtom(disp, "_NET_WM_DESKTOP", False);

    // type atoms
    m_net_wm_window_type = XInternAtom(disp, "_NET_WM_WINDOW_TYPE", False);
    m_net_wm_window_type_dock = XInternAtom(disp, "_NET_WM_WINDOW_TYPE_DOCK", False);
    m_net_wm_window_type_desktop = XInternAtom(disp, "_NET_WM_WINDOW_TYPE_DESKTOP", False);
    m_net_wm_window_type_splash = XInternAtom(disp, "_NET_WM_WINDOW_TYPE_SPLASH", False);
    m_net_wm_window_type_dialog = XInternAtom(disp, "_NET_WM_WINDOW_TYPE_DIALOG", False);
    m_net_wm_window_type_normal = XInternAtom(disp, "_NET_WM_WINDOW_TYPE_NORMAL", False);

    // state atom and the supported state atoms
    m_net_wm_state = XInternAtom(disp, "_NET_WM_STATE", False);
    m_net_wm_state_sticky = XInternAtom(disp, "_NET_WM_STATE_STICKY", False);
    m_net_wm_state_shaded = XInternAtom(disp, "_NET_WM_STATE_SHADED", False);
    m_net_wm_state_maximized_horz = XInternAtom(disp, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
    m_net_wm_state_maximized_vert = XInternAtom(disp, "_NET_WM_STATE_MAXIMIZED_VERT", False);
    m_net_wm_state_fullscreen = XInternAtom(disp, "_NET_WM_STATE_FULLSCREEN", False);
    m_net_wm_state_hidden = XInternAtom(disp, "_NET_WM_STATE_HIDDEN", False);
    m_net_wm_state_skip_taskbar = XInternAtom(disp, "_NET_WM_STATE_SKIP_TASKBAR", False);
    m_net_wm_state_skip_pager = XInternAtom(disp, "_NET_WM_STATE_SKIP_PAGER", False);
    m_net_wm_state_above = XInternAtom(disp, "_NET_WM_STATE_ABOVE", False);
    m_net_wm_state_below = XInternAtom(disp, "_NET_WM_STATE_BELOW", False);
    m_net_wm_state_modal = XInternAtom(disp, "_NET_WM_STATE_MODAL", False);
    m_net_wm_state_demands_attention = XInternAtom(disp, "_NET_WM_STATE_DEMANDS_ATTENTION", False);

    // allowed actions
    m_net_wm_allowed_actions = XInternAtom(disp, "_NET_WM_ALLOWED_ACTIONS", False);
    m_net_wm_action_move = XInternAtom(disp, "_NET_WM_ACTION_MOVE", False);
    m_net_wm_action_resize = XInternAtom(disp, "_NET_WM_ACTION_RESIZE", False);
    m_net_wm_action_minimize = XInternAtom(disp, "_NET_WM_ACTION_MINIMIZE", False);
    m_net_wm_action_shade = XInternAtom(disp, "_NET_WM_ACTION_SHADE", False);
    m_net_wm_action_stick = XInternAtom(disp, "_NET_WM_ACTION_STICK", False);
    m_net_wm_action_maximize_horz = XInternAtom(disp, "_NET_WM_ACTION_MAXIMIZE_HORZ", False);
    m_net_wm_action_maximize_vert = XInternAtom(disp, "_NET_WM_ACTION_MAXIMIZE_VERT", False);
    m_net_wm_action_fullscreen = XInternAtom(disp, "_NET_WM_ACTION_FULLSCREEN", False);
    m_net_wm_action_change_desktop = XInternAtom(disp, "_NET_WM_ACTION_CHANGE_DESKTOP", False);
    m_net_wm_action_close = XInternAtom(disp, "_NET_WM_ACTION_CLOSE", False);

    m_net_wm_strut = XInternAtom(disp, "_NET_WM_STRUT", False);
    m_net_wm_icon_geometry = XInternAtom(disp, "_NET_WM_ICON_GEOMETRY", False);
    m_net_wm_icon = XInternAtom(disp, "_NET_WM_ICON", False);
    m_net_wm_pid = XInternAtom(disp, "_NET_WM_PID", False);
    m_net_wm_handled_icons = XInternAtom(disp, "_NET_WM_HANDLED_ICONS", False);

    m_net_frame_extents = XInternAtom(disp, "_NET_FRAME_EXTENTS", False);

    m_net_wm_ping = XInternAtom(disp, "_NET_WM_PING", False);
    utf8_string = XInternAtom(disp, "UTF8_STRING", False);
}


void Ewmh::setFullscreen(FluxboxWindow &win, bool value) {
    // fullscreen implies maximised, above dock layer,
    // and no decorations (or decorations offscreen)
    // 
    // TODO: do we need the WindowState etc here anymore?
    //       FluxboxWindow::setFullscreen() remembering old values
    //       already and set them...
    //       only reason i can see is that the user manually
    //       moved the (fullscreened) window
    WindowState *saved_state = getState(win);
    if (value) {
        // fullscreen on
        if (!saved_state) { // not already fullscreen
            saved_state = new WindowState(win.x(), win.y(), win.width(),
                                          win.height(), win.layerNum(), win.decorationMask());
            saveState(win, saved_state);
            win.setFullscreen(true);
        }
    } else { // turn off fullscreen
        if (saved_state) { // no saved state, can't restore it
            win.setFullscreen(false);
            /*
            win.setDecorationMask(saved_state->decor);
            win.moveResize(saved_state->x, saved_state->y,
                           saved_state->width, saved_state->height);
            win.moveToLayer(saved_state->layer);
            */
            clearState(win);
            saved_state = 0;
        }
    }
}

// set window state
void Ewmh::setState(FluxboxWindow &win, Atom state, bool value) {
    if (state == m_net_wm_state_sticky) { // STICKY
        if (value && !win.isStuck() ||
            (!value && win.isStuck()))
            win.stick();
    } else if (state == m_net_wm_state_shaded) { // SHADED
        if ((value && !win.isShaded()) ||
            (!value && win.isShaded()))
            win.shade();
    }  else if (state == m_net_wm_state_maximized_horz ) { // maximized Horizontal
        if ((value && !win.isMaximized()) ||
            (!value && win.isMaximized()))
        win.maximizeHorizontal();
    } else if (state == m_net_wm_state_maximized_vert) { // maximized Vertical
        if ((value && !win.isMaximized()) ||
            (!value && win.isMaximized()))
        win.maximizeVertical();
    } else if (state == m_net_wm_state_fullscreen) { // fullscreen
        if ((value && !win.isFullscreen()) ||
            (!value && win.isFullscreen()))
        setFullscreen(win, value);
    } else if (state == m_net_wm_state_hidden) { // minimized
        if (value && !win.isIconic())
            win.iconify();
        else if (!value && win.isIconic())
            win.deiconify();
    } else if (state == m_net_wm_state_skip_taskbar) { // skip taskbar
        win.setIconHidden(value);
    } else if (state == m_net_wm_state_below) {  // bottom layer
        if (value)
            win.moveToLayer(Layer::BOTTOM);
        else
            win.moveToLayer(Layer::NORMAL);

    } else if (state == m_net_wm_state_above) { // above layer
        if (value)
            win.moveToLayer(Layer::ABOVE_DOCK);
        else
            win.moveToLayer(Layer::NORMAL);
    } else if (state == m_net_wm_state_demands_attention) {
        if (value) { // if add attention
            Fluxbox::instance()->attentionHandler().addAttention(win.winClient());
        } else { // erase it
            Fluxbox::instance()->attentionHandler().
                update(&win.winClient().focusSig());
        }
    }

    // Note: state == net_wm_state_modal, We should not change it
}

// toggle window state
void Ewmh::toggleState(FluxboxWindow &win, Atom state) {
    if (state == m_net_wm_state_sticky) { // sticky
        win.stick();
    } else if (state == m_net_wm_state_shaded){ // shaded
        win.shade();
    } else if (state == m_net_wm_state_maximized_horz ) { // maximized Horizontal
        win.maximizeHorizontal();
    } else if (state == m_net_wm_state_maximized_vert) { // maximized Vertical
        win.maximizeVertical();
    } else if (state == m_net_wm_state_fullscreen) { // fullscreen
        setFullscreen(win, getState(win) == 0); // toggle current state
    } else if (state == m_net_wm_state_hidden) { // minimized
        if(win.isIconic())
            win.deiconify();
        else
            win.iconify();
    } else if (state == m_net_wm_state_skip_taskbar) { // taskbar
        win.setIconHidden(!win.isIconHidden());
    } else if (state == m_net_wm_state_below) { // bottom layer
        if (win.layerNum() == Layer::BOTTOM)
            win.moveToLayer(Layer::NORMAL);
        else
            win.moveToLayer(Layer::BOTTOM);

    } else if (state == m_net_wm_state_above) { // top layer
        if (win.layerNum() == Layer::ABOVE_DOCK)
            win.moveToLayer(Layer::NORMAL);
        else
            win.moveToLayer(Layer::ABOVE_DOCK);
    }

}


void Ewmh::updateStrut(WinClient &winclient) {
    Atom ret_type = 0;
    int fmt = 0;
    unsigned long nitems = 0, bytes_after = 0;
    long *data = 0;
    if (winclient.property(m_net_wm_strut, 0, 4, False, XA_CARDINAL,
                                 &ret_type, &fmt, &nitems, &bytes_after,
                                 (unsigned char **) &data) && data) {

        int head = winclient.screen().getHead(winclient);
        winclient.setStrut(winclient.screen().requestStrut(head, 
                           data[0], data[1], 
                           data[2], data[3]));
        winclient.screen().updateAvailableWorkspaceArea();
    }
}



void Ewmh::updateActions(FluxboxWindow &win) {

    /* From Extended Window Manager Hints, draft 1.3:
     *
     * _NET_WM_ALLOWED_ACTIONS, ATOM[]
     *
     * A list of atoms indicating user operations that the 
     * Window Manager supports for this window. Atoms present  in the
     * list indicate allowed actions, atoms not present in the list 
     * indicate actions that are not supported for this window. The 
     * Window Manager MUST keep this property updated to reflect the 
     * actions which are currently "active" or "sensitive" for a window.
     * Taskbars, Pagers, and other tools use _NET_WM_ALLOWED_ACTIONS to 
     * decide which actions should be made available to the user. 
     */
    
    typedef std::vector<Atom> ActionsVector;
    ActionsVector actions;
    actions.reserve(10);
    // all windows can change desktop,
    // be shaded or be sticky
    actions.push_back(m_net_wm_action_change_desktop);
    actions.push_back(m_net_wm_action_shade);
    actions.push_back(m_net_wm_action_stick);
    
    if (win.isResizable())
        actions.push_back(m_net_wm_action_resize);
    if (win.isMoveable())
        actions.push_back(m_net_wm_action_move);
    if (win.isClosable())
        actions.push_back(m_net_wm_action_close);
    if (win.isIconifiable())
        actions.push_back(m_net_wm_action_minimize);

    unsigned int max_width, max_height;
    WinClientUtil::maxSize(win.clientList(), max_width, max_height);

    // if unlimited max width we can maximize horizontal
    if (max_width == 0) {
        actions.push_back(m_net_wm_action_maximize_horz);
    }
    // if unlimited max height we can maxmize vert
    if (max_height == 0) {
        actions.push_back(m_net_wm_action_maximize_vert);
    }

    // if we have unlimited size in all directions we can have this window
    // in fullscreen mode
    if (max_height == 0 && max_width == 0) {
        actions.push_back(m_net_wm_action_fullscreen);
    }



    FluxboxWindow::ClientList::iterator it = win.clientList().begin();
    FluxboxWindow::ClientList::iterator it_end = win.clientList().end();
    for (; it != it_end; ++it) {
        (*it)->changeProperty(m_net_wm_allowed_actions, XA_ATOM, 32, PropModeReplace,
                              reinterpret_cast<unsigned char*>(&actions.front()),
                              actions.size());        
    }

}

void Ewmh::setupState(FluxboxWindow &win) {
    /* From Extended Window Manager Hints, draft 1.3:
     *
     * _NET_WM_STATE, ATOM[]
     *
     * A list of hints describing the window state. Atoms present in
     * the list MUST be considered set, atoms not present in the list
     * MUST be considered not set. The Window Manager SHOULD honor
     * _NET_WM_STATE whenever a withdrawn window requests to be mapped.
     * A Client wishing to change the state of a window MUST send a
     * _NET_WM_STATE client message to the root window (see below).
     * The Window Manager MUST keep this property updated to reflect
     * the current state of the window.
     *
     * The Window Manager should remove the property whenever a window
     * is withdrawn, but it should leave the property in place when it
     * is shutting down, e.g. in response to losing ownership of the
     * WM_Sn manager selection.
     */
    Atom ret_type;
    int fmt;
    unsigned long nitems, bytes_after;
    unsigned char *data = 0;

    win.winClient().property(m_net_wm_state, 0, 0x7fffffff, False, XA_ATOM,
                             &ret_type, &fmt, &nitems, &bytes_after,
                             &data);
    if (data) {
        Atom *states = (Atom *)data;
        for (unsigned long i=0; i < nitems; ++i)
            setState(win, states[i], true);

        XFree(data);
    }
}

void Ewmh::updateFrameExtents(FluxboxWindow &win) {
    int extents[4];
    extents[0] = win.frame().x();
    extents[1] = win.frame().x() + win.frame().width();
    extents[2] = win.frame().y();
    extents[3] = win.frame().y() + win.frame().height();

    FluxboxWindow::ClientList::iterator it = win.clientList().begin();
    FluxboxWindow::ClientList::iterator it_end = win.clientList().end();
    for (; it != it_end; ++it) {
        (*it)->changeProperty(m_net_frame_extents,
                              XA_CARDINAL, 32, PropModeReplace,
                              (unsigned char *)extents, 4);
    }
}


Ewmh::WindowState::WindowState(int t_x, int t_y,
                               unsigned int t_width,
                               unsigned int t_height,
                               int t_layer, unsigned int t_decor) :
    x(t_x), y(t_y),
    layer(t_layer),
    width(t_width),
    height(t_height),
    decor(t_decor)
{}

Ewmh::WindowState *Ewmh::getState(FluxboxWindow &win) {
    SavedState::iterator it = m_savedstate.find(&win);
    if (it == m_savedstate.end())
        return 0;
    else
        return it->second;
}

void Ewmh::clearState(FluxboxWindow &win) {
    WindowState *state = 0;
    SavedState::iterator it = m_savedstate.find(&win);
    if (it == m_savedstate.end())
        return;

    state = it->second;

    m_savedstate.erase(it);
    delete state;
}

void Ewmh::saveState(FluxboxWindow &win, WindowState *state) {
    m_savedstate[&win] = state;
}


