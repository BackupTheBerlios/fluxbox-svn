// Gnome.cc for fluxbox
// Copyright (c) 2002 - 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

// $Id: Gnome.cc,v 1.25 2003/05/19 22:40:40 fluxgen Exp $

#include "Gnome.hh"

#include "fluxbox.hh"
#include "Window.hh"
#include "Screen.hh"
#include "WinClient.hh"
#include "Workspace.hh"

#include <iostream>
#include <new>
using namespace std;

Gnome::Gnome() {
    createAtoms();
    enableUpdate();
}

Gnome::~Gnome() {
    // destroy gnome windows
    while (!m_gnomewindows.empty()) {
        XDestroyWindow(FbTk::App::instance()->display(), m_gnomewindows.back());
        m_gnomewindows.pop_back();		
    }
}


void Gnome::initForScreen(BScreen &screen) {
    Display *disp = FbTk::App::instance()->display();
    // create the GNOME window
    Window gnome_win = XCreateSimpleWindow(disp,
                                           screen.rootWindow().window(), 0, 0, 5, 5, 0, 0, 0);
    // supported WM check
    screen.rootWindow().changeProperty(m_gnome_wm_supporting_wm_check, 
                                       XA_CARDINAL, 32, 
                                       PropModeReplace, (unsigned char *) &gnome_win, 1);

    XChangeProperty(disp, gnome_win, 
                    m_gnome_wm_supporting_wm_check, 
                    XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &gnome_win, 1);

    Atom gnomeatomlist[] = {
        m_gnome_wm_supporting_wm_check,
        m_gnome_wm_win_workspace_names,
        m_gnome_wm_win_client_list,
        m_gnome_wm_win_state,
        m_gnome_wm_win_hints,
        m_gnome_wm_win_layer
    };

    //list atoms that we support
    screen.rootWindow().changeProperty(m_gnome_wm_prot, 
                                       XA_ATOM, 32, PropModeReplace,
                                       (unsigned char *)gnomeatomlist, 
                                       (sizeof gnomeatomlist)/sizeof gnomeatomlist[0]);

    m_gnomewindows.push_back(gnome_win);

    updateClientList(screen);
    updateWorkspaceNames(screen);
    updateWorkspaceCount(screen);
    updateCurrentWorkspace(screen);
	
}

void Gnome::setupWindow(FluxboxWindow &win) {
    // load gnome state atom
    Display *disp = FbTk::App::instance()->display();
    Atom ret_type;
    int fmt;
    unsigned long nitems, bytes_after;
    long flags, *data = 0;

    if (win.winClient().property(m_gnome_wm_win_state, 0, 1, False, XA_CARDINAL, 
                                 &ret_type, &fmt, &nitems, &bytes_after, 
                                 (unsigned char **) &data) && data) {
        flags = *data;
        setState(&win, flags);
        XFree (data);
    }

    // load gnome layer atom
    if (XGetWindowProperty(disp, win.clientWindow(), 
                           m_gnome_wm_win_layer, 0, 1, False, XA_CARDINAL, 
                           &ret_type, &fmt, &nitems, &bytes_after, 
                           (unsigned char **) &data) ==  Success && data) {
        flags = *data;
        setLayer(&win, flags);
        XFree (data);
    }

    // load gnome workspace atom
    if (XGetWindowProperty(disp, win.clientWindow(), 
                           m_gnome_wm_win_workspace, 0, 1, False, XA_CARDINAL, 
                           &ret_type, &fmt, &nitems, &bytes_after, 
                           (unsigned char **) &data) ==  Success && data) {
        unsigned int workspace_num = *data;
        if (win.workspaceNumber() != workspace_num) 
            win.screen().reassociateWindow(&win, workspace_num, false);
        XFree (data);
    }

}

void Gnome::updateClientList(BScreen &screen) {
    size_t num=0;

    // count window clients in each workspace
    BScreen::Workspaces::const_iterator workspace_it = 
        screen.getWorkspacesList().begin();
    BScreen::Workspaces::const_iterator workspace_it_end = 
        screen.getWorkspacesList().end();
    for (; workspace_it != workspace_it_end; ++workspace_it) {
        Workspace::Windows::iterator win_it = 
            (*workspace_it)->windowList().begin();
        Workspace::Windows::iterator win_it_end = 
            (*workspace_it)->windowList().end();
        for (; win_it != win_it_end; ++win_it)
            num += (*win_it)->numClients();
    }
	
    Window *wl = new (nothrow) Window[num];
    if (wl == 0) {
        cerr<<"Fatal: Out of memory, can't allocate for gnome client list"<<endl;
        return;
    }

    //add client windows to buffer
    workspace_it = screen.getWorkspacesList().begin();
    int win=0;
    for (; workspace_it != workspace_it_end; ++workspace_it) {
	
        // Fill in array of window ID's
        Workspace::Windows::const_iterator it = 
            (*workspace_it)->windowList().begin();
        Workspace::Windows::const_iterator it_end = 
            (*workspace_it)->windowList().end();		
        for (; it != it_end; ++it) {
            // TODO!
            //check if the window don't want to be visible in the list
            //if (! ( (*it)->getGnomeHints() & WIN_STATE_HIDDEN) ) {
            std::list<WinClient *>::iterator client_it = 
                (*it)->clientList().begin();
            std::list<WinClient *>::iterator client_it_end = 
                (*it)->clientList().end();
            for (; client_it != client_it_end; ++client_it)
                wl[win++] = (*client_it)->window();

        }
    }
    //number of windows to show in client list
    num = win;
    screen.rootWindow().changeProperty(m_gnome_wm_win_client_list, 
                                       XA_CARDINAL, 32,
                                       PropModeReplace, (unsigned char *)wl, num);
	
    delete[] wl;
}

void Gnome::updateWorkspaceNames(BScreen &screen) {
    XTextProperty	text;
    int number_of_desks = screen.getWorkspaceNames().size();
	
    char s[1024];
    char *names[number_of_desks];		
	
    for (int i = 0; i < number_of_desks; i++) {		
        sprintf(s, "Desktop %i", i);
        names[i] = new char[strlen(s) + 1];
        strcpy(names[i], s);
    }
	
    if (XStringListToTextProperty(names, number_of_desks, &text)) {
        XSetTextProperty(FbTk::App::instance()->display(), screen.rootWindow().window(),
			 &text, m_gnome_wm_win_workspace_names);
        XFree(text.value);
    }
	
    for (int i = 0; i < number_of_desks; i++)
        delete [] names[i];
}

void Gnome::updateCurrentWorkspace(BScreen &screen) {
    int workspace = screen.currentWorkspaceID();
    screen.rootWindow().changeProperty(m_gnome_wm_win_workspace, XA_CARDINAL, 32, PropModeReplace,
                                       (unsigned char *)&workspace, 1);

    updateClientList(screen); // make sure the client list is updated too
}

void Gnome::updateWorkspaceCount(BScreen &screen) {
    int numworkspaces = screen.getCount();
    screen.rootWindow().changeProperty(m_gnome_wm_win_workspace_count, XA_CARDINAL, 32, PropModeReplace,
                                       (unsigned char *)&numworkspaces, 1);
}

void Gnome::updateWorkspace(FluxboxWindow &win) {
    int val = win.workspaceNumber(); 
#ifdef DEBUG
    cerr<<__FILE__<<"("<<__LINE__<<"): setting workspace("<<val<<
        ") for window("<<&win<<")"<<endl;
#endif // DEBUG
    win.winClient().changeProperty(m_gnome_wm_win_workspace, 
                                   XA_CARDINAL, 32, PropModeReplace,
                                   (unsigned char *)&val, 1);
}

void Gnome::updateState(FluxboxWindow &win) {
    //translate to gnome win state
    int state=0;
    if (win.isStuck())
        state |= WIN_STATE_STICKY;
    if (win.isIconic())
        state |= WIN_STATE_MINIMIZED;
    if (win.isShaded())
        state |= WIN_STATE_SHADED;
	
    win.winClient().changeProperty(m_gnome_wm_win_state,
                                   XA_CARDINAL, 32, 
                                   PropModeReplace, (unsigned char *)&state, 1);
}

void Gnome::updateLayer(FluxboxWindow &win) {
    //TODO - map from flux layers to gnome ones
    // our layers are in the opposite direction to GNOME
    int layernum = Fluxbox::instance()->getDesktopLayer() - win.layerNum();
    win.winClient().changeProperty(m_gnome_wm_win_layer,
                                   XA_CARDINAL, 32, PropModeReplace, 
                                   (unsigned char *)&layernum, 1);
    
}

void Gnome::updateHints(FluxboxWindow &win) {
    //TODO
	
}

bool Gnome::checkClientMessage(const XClientMessageEvent &ce, BScreen * screen, FluxboxWindow * const win) {
    if (ce.message_type == m_gnome_wm_win_workspace) {
#ifdef DEBUG
        cerr<<__FILE__<<"("<<__LINE__<<"): Got workspace atom="<<ce.data.l[0]<<endl;
#endif//!DEBUG
        if ( win !=0 && // the message sent to client window?
             ce.data.l[0] >= 0 &&
             ce.data.l[0] < (signed)win->screen().getCount()) {
            win->screen().changeWorkspaceID(ce.data.l[0]);
					
        } else if (screen!=0 && //the message sent to root window?
                   ce.data.l[0] >= 0 &&
                   ce.data.l[0] < (signed)screen->getCount())
            screen->changeWorkspaceID(ce.data.l[0]);
        return true;
    } else if (win == 0)
        return false; 
		

    if (ce.message_type == m_gnome_wm_win_state) {
#ifdef DEBUG
        cerr<<__FILE__<<"("<<__LINE__<<"): _WIN_STATE"<<endl;
#endif // DEBUG
			
#ifdef DEBUG
        cerr<<__FILE__<<"("<<__LINE__<<"): Mask of members to change:"<<
            hex<<ce.data.l[0]<<dec<<endl; // mask_of_members_to_change
        cerr<<"New members:"<<ce.data.l[1]<<endl;
#endif // DEBUG
	
        //get new states			
        int flag = ce.data.l[0] & ce.data.l[1];
        //don't update this when when we set new state
        disableUpdate();
        // convert to Fluxbox state
        setState(win, flag);
        // enable update of atom states
        enableUpdate();
			
    } else if (ce.message_type == m_gnome_wm_win_hints) {
#ifdef DEBUG
        cerr<<__FILE__<<"("<<__LINE__<<"): _WIN_HINTS"<<endl;
#endif // DEBUG

    } else if (ce.message_type == m_gnome_wm_win_layer) {
#ifdef DEBUG
        cerr<<__FILE__<<"("<<__LINE__<<"): _WIN_LAYER"<<endl;
#endif // DEBUG

        setLayer(win, ce.data.l[0]);
    } else
        return false; //the gnome atom wasn't found or not supported

    return true; // we handled the atom
}

void Gnome::setState(FluxboxWindow *win, int state) {
#ifdef DEBUG
    cerr<<"Gnome: state=0x"<<hex<<state<<dec<<endl;
#endif // DEBUG

    if (state & WIN_STATE_STICKY) {
#ifdef DEBUG
        cerr<<"Gnome state: Sticky"<<endl;
#endif // DEBUG
        if (!win->isStuck())
            win->stick();
    } else if (win->isStuck())
        win->stick();
			
    if (state & WIN_STATE_MINIMIZED) {
#ifdef DEBUG
        cerr<<"Gnome state: Minimized"<<endl;
#endif // DEBUG
        if (win->isIconic())
            win->iconify();
    } else if (win->isIconic())
        win->deiconify(true, true);

    if (state & WIN_STATE_SHADED) {
#ifdef DEBUG
        cerr<<"Gnome state: Shade"<<endl;
#endif // DEBUG
        if (!win->isShaded())
            win->shade();
    } else if (win->isShaded())
        win->shade();

    /* TODO	
       if (state & WIN_STATE_MAXIMIZED_VERT)
       cerr<<"Maximize Vert"<<endl;
       if (state & WIN_STATE_MAXIMIZED_HORIZ)
       cerr<<"Maximize Horiz"<<endl;
       if (state & WIN_STATE_HIDDEN)
       cerr<<"Hidden"<<endl;
       if (state & WIN_STATE_HID_WORKSPACE)
       cerr<<"HID Workspace"<<endl;
       if (state & WIN_STATE_HID_TRANSIENT)
       cerr<<"HID Transient"<<endl;
       if (state & WIN_STATE_FIXED_POSITION)
       cerr<<"Fixed Position"<<endl;
       if (state & WIN_STATE_ARRANGE_IGNORE)
       cerr<<"Arrange Ignore"<<endl;			
    */
}

void Gnome::setLayer(FluxboxWindow *win, int layer) {
    if (!win) return;
    
    
    switch (layer) {
    case WIN_LAYER_DESKTOP:
        layer = Fluxbox::instance()->getDesktopLayer();
        break;
    case WIN_LAYER_BELOW:
        layer = Fluxbox::instance()->getBottomLayer();
        break;
    case WIN_LAYER_NORMAL:
        layer = Fluxbox::instance()->getNormalLayer();
        break;		
    case WIN_LAYER_ONTOP:
        layer = Fluxbox::instance()->getTopLayer();
        break;
    case WIN_LAYER_DOCK:
        layer = Fluxbox::instance()->getDockLayer();
        break;
    case WIN_LAYER_ABOVE_DOCK:
        layer = Fluxbox::instance()->getAboveDockLayer();
        break;
    case WIN_LAYER_MENU:
        layer = Fluxbox::instance()->getMenuLayer();
        break;
    default:
        // our windows are in the opposite direction to gnome
        layer = Fluxbox::instance()->getDesktopLayer() - layer;
        break;
    }
    win->moveToLayer(layer);

}

void Gnome::createAtoms() {
    Display *disp = FbTk::App::instance()->display();
    m_gnome_wm_win_layer = XInternAtom(disp, "_WIN_LAYER", False);
    m_gnome_wm_win_state = XInternAtom(disp, "_WIN_STATE", False);
    m_gnome_wm_win_hints = XInternAtom(disp, "_WIN_HINTS", False);
    m_gnome_wm_win_app_state = XInternAtom(disp, "_WIN_APP_STATE", False);
    m_gnome_wm_win_expanded_size = XInternAtom(disp, "_WIN_EXPANDED_SIZE", False);
    m_gnome_wm_win_icons = XInternAtom(disp, "_WIN_ICONS", False);
    m_gnome_wm_win_workspace = XInternAtom(disp, "_WIN_WORKSPACE", False);
    m_gnome_wm_win_workspace_count = XInternAtom(disp, "_WIN_WORKSPACE_COUNT", False);
    m_gnome_wm_win_workspace_names = XInternAtom(disp, "_WIN_WORKSPACE_NAMES", False);
    m_gnome_wm_win_client_list = XInternAtom(disp, "_WIN_CLIENT_LIST", False);
    m_gnome_wm_prot = XInternAtom(disp, "_WIN_PROTOCOLS", False);
    m_gnome_wm_supporting_wm_check = XInternAtom(disp, "_WIN_SUPPORTING_WM_CHECK", False);
}
