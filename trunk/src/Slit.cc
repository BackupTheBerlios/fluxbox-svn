// Slit.cc for fluxbox
// Copyright (c) 2002 - 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
//
// Slit.cc for Blackbox - an X11 Window manager
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

// $Id: Slit.cc,v 1.61 2003/06/18 13:49:43 fluxgen Exp $

#include "Slit.hh"

//use GNU extensions
#ifndef	 _GNU_SOURCE
#define	 _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "I18n.hh"
#include "Screen.hh"
#include "ImageControl.hh"
#include "RefCount.hh"
#include "SimpleCommand.hh"
#include "BoolMenuItem.hh"
#include "EventManager.hh"
#include "MacroCommand.hh"
#include "LayerMenu.hh"
#include "fluxbox.hh"
#include "XLayer.hh"
#include "RootTheme.hh"
#include "FbTk/Theme.hh"
#include "FbMenu.hh"
#include "Transparent.hh"
#include "IntResMenuItem.hh"
#include "Xinerama.hh"

#include <algorithm>
#include <iostream>
#include <cassert>

#ifdef HAVE_SYS_STAT_H
#include <sys/types.h>
#include <sys/stat.h>
#endif // HAVE_SYS_STAT_H

#include <X11/Xatom.h>

#include <iostream>
#include <algorithm>
using namespace std;
namespace {

void getWMName(BScreen *screen, Window window, std::string& name) {
    name = "";

    if (screen == 0 || window == None)
        return;

    Display *display = FbTk::App::instance()->display();

    XTextProperty text_prop;
    char **list;
    int num;
    I18n *i18n = I18n::instance();

    if (XGetWMName(display, window, &text_prop)) {
        if (text_prop.value && text_prop.nitems > 0) {
            if (text_prop.encoding != XA_STRING) {
				
                text_prop.nitems = strlen((char *) text_prop.value);
				
                if ((XmbTextPropertyToTextList(display, &text_prop,
                                               &list, &num) == Success) &&
                    (num > 0) && *list) {
                    name = static_cast<char *>(*list);
                    XFreeStringList(list);
                } else
                    name = (char *)text_prop.value;
					
            } else				
                name = (char *)text_prop.value;
        } else { // default name
            name = i18n->getMessage(
                                    FBNLS::WindowSet, FBNLS::WindowUnnamed,
                                    "Unnamed");
        }
    } else {
        // default name
        name = i18n->getMessage(
                                FBNLS::WindowSet, FBNLS::WindowUnnamed,
                                "Unnamed");
    }

}

};

/// holds slit client info
class SlitClient {
public:
    /// For adding an actual window
    SlitClient(BScreen *screen, Window win) {
        initialize(screen, win);
    }
    /// For adding a placeholder
    explicit SlitClient(const char *name) { 
        initialize();
        match_name = (name == 0 ? "" : name);

    }

    // Now we pre-initialize a list of slit clients with names for
    // comparison with incoming client windows.  This allows the slit
    // to maintain a sorted order based on a saved window name list.
    // Incoming windows not found in the list are appended.  Matching
    // duplicates are inserted after the last found instance of the
    // matching name.
    std::string match_name;

    Window window, client_window, icon_window;

    int x, y;
    unsigned int width, height;
    bool visible; ///< wheter the client should be visible or not

    void initialize(BScreen *screen = 0, Window win= None) {
        client_window = win;
        window = icon_window = None;
        x = y = 0;
        width = height = 0;
        if (match_name.size() == 0)
            getWMName(screen, client_window, match_name);
        visible = true;        
    }
    void disableEvents() {
        if (window == 0)
            return;
        Display *disp = FbTk::App::instance()->display();
        XSelectInput(disp, window, NoEventMask);
    }
    void enableEvents() {
        if (window == 0)
            return;
        Display *disp = FbTk::App::instance()->display();
        XSelectInput(disp, window, StructureNotifyMask |
                 SubstructureNotifyMask | EnterWindowMask);
    }
};

namespace { 

class SlitClientMenuItem: public FbTk::MenuItem {
public:
    explicit SlitClientMenuItem(SlitClient &client, FbTk::RefCount<FbTk::Command> &cmd):
        FbTk::MenuItem(client.match_name.c_str(), cmd), m_client(client) {
        FbTk::MenuItem::setSelected(client.visible);
    }
    const std::string &label() const {
        return m_client.match_name;
    }
    bool isSelected() const {
        return m_client.visible;
    }
    void click(int button, int time) { 
        m_client.visible = !m_client.visible;
        FbTk::MenuItem::click(button, time);
        Fluxbox::instance()->save_rc();
    }
private:
    SlitClient &m_client;
};

class SlitDirMenuItem: public FbTk::MenuItem {
public:
    SlitDirMenuItem(const char *label, Slit &slit, FbTk::RefCount<FbTk::Command> &cmd)
        :FbTk::MenuItem(label,cmd), 
         m_slit(slit), 
         m_label(label ? label : "") { 
        setLabel(m_label.c_str()); // update label
    }
    void click(int button, int time) {
        // toggle direction
        if (m_slit.direction() == Slit::HORIZONTAL)
            m_slit.setDirection(Slit::VERTICAL);
        else
            m_slit.setDirection(Slit::HORIZONTAL);
        setLabel(m_label.c_str());
        FbTk::MenuItem::click(button, time);
    }

    void setLabel(const char *label) {
        I18n *i18n = I18n::instance();
        m_label = (label ? label : "");
        std::string reallabel = m_label + " " + 
            ( m_slit.direction() == Slit::HORIZONTAL ? 
              i18n->getMessage(
                               FBNLS::CommonSet, FBNLS::CommonDirectionHoriz,
                               "Horizontal") :
              i18n->getMessage(
                               FBNLS::CommonSet, FBNLS::CommonDirectionVert,
                               "Vertical") );
        FbTk::MenuItem::setLabel(reallabel.c_str());
    }
private:
    Slit &m_slit;
    std::string m_label;
};

class PlaceSlitMenuItem: public FbTk::MenuItem {
public:
    PlaceSlitMenuItem(const char *label, Slit &slit, Slit::Placement place, FbTk::RefCount<FbTk::Command> &cmd):
        FbTk::MenuItem(label, cmd), m_slit(slit), m_place(place) {
     
    }
    bool isEnabled() const { return m_slit.placement() != m_place; }
    void click(int button, int time) {
        m_slit.setPlacement(m_place);
        FbTk::MenuItem::click(button, time);
    }
private:
    Slit &m_slit;
    Slit::Placement m_place;
};


}; // End anonymous namespace

class SlitTheme:public FbTk::Theme {
public:
    explicit SlitTheme(Slit &slit):FbTk::Theme(slit.screen().screenNumber()), 
                          m_slit(slit),
                          m_texture(*this, "slit", "Slit") { 
        // default texture type
        m_texture->setType(FbTk::Texture::SOLID);
    }
    void reconfigTheme() {
        m_slit.reconfigure();
    }
    const FbTk::Texture &texture() const { return *m_texture; }
private:
    Slit &m_slit;
    FbTk::ThemeItem<FbTk::Texture> m_texture;
};

unsigned int Slit::s_eventmask = SubstructureRedirectMask |  ButtonPressMask | 
                                 EnterWindowMask | LeaveWindowMask | ExposureMask;

Slit::Slit(BScreen &scr, FbTk::XLayer &layer, const char *filename)
    : m_screen(scr), m_timer(this), 
      m_slitmenu(*scr.menuTheme(), 
                 scr.screenNumber(), 
                 scr.imageControl(),
                 *scr.layerManager().getLayer(Fluxbox::instance()->getMenuLayer())),
      m_placement_menu(*scr.menuTheme(),
                       scr.screenNumber(),
                       scr.imageControl(),
                       *scr.layerManager().getLayer(Fluxbox::instance()->getMenuLayer())),
      m_clientlist_menu(*scr.menuTheme(),
                        scr.screenNumber(),
                        scr.imageControl(),
                        *scr.layerManager().getLayer(Fluxbox::instance()->getMenuLayer())),
      m_layermenu(new LayerMenu<Slit>(*scr.menuTheme(),
                                      scr.screenNumber(),
                                      scr.imageControl(),
                                      *scr.layerManager().getLayer(Fluxbox::instance()->getMenuLayer()), 
                                      this,
                                      true)),
      //For KDE dock applets
      m_kwm1_dockwindow(XInternAtom(FbTk::App::instance()->display(), 
                                    "KWM_DOCKWINDOW", False)), //KDE v1.x
      m_kwm2_dockwindow(XInternAtom(FbTk::App::instance()->display(), 
                                    "_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR", False)), //KDE v2.x

      m_layeritem(0),
      m_slit_theme(new SlitTheme(*this)),
      m_strut(0) {

    // default placement and direction
    m_direction = screen().getSlitDirection();
    m_placement = screen().getSlitPlacement();
    m_hidden = m_do_auto_hide = screen().doSlitAutoHide();

    frame.pixmap = None;

    m_timer.setTimeout(200); // default timeout
    m_timer.fireOnce(true);

    XSetWindowAttributes attrib;
    unsigned long create_mask = CWBackPixmap | CWBackPixel | CWBorderPixel |
        CWColormap | CWOverrideRedirect | CWEventMask;
    attrib.background_pixmap = None;
    attrib.background_pixel = attrib.border_pixel =
        screen().rootTheme().borderColor().pixel();
    attrib.colormap = screen().rootWindow().colormap();
    attrib.override_redirect = True;
    attrib.event_mask = s_eventmask;

    frame.x = frame.y = 0;
    frame.width = frame.height = 1;
    Display *disp = FbTk::App::instance()->display();
    frame.window =
        XCreateWindow(disp, screen().rootWindow().window(), frame.x, frame.y,
                      frame.width, frame.height, screen().rootTheme().borderWidth(),
                      screen().rootWindow().depth(), InputOutput, screen().rootWindow().visual(),
                      create_mask, &attrib);

    FbTk::EventManager::instance()->add(*this, frame.window);
    m_transp.reset(new FbTk::Transparent(screen().rootPixmap(), frame.window.drawable(), 
                                         *screen().slitAlphaResource(), 
                                         screen().screenNumber()));

    m_layeritem.reset(new FbTk::XLayerItem(frame.window, layer));

    // Get client list for sorting purposes
    loadClientList(filename);

    setupMenu();

    reconfigure();
}


Slit::~Slit() {
    clearStrut();
    if (frame.pixmap != 0)
        screen().imageControl().removeImage(frame.pixmap);
}

void Slit::clearStrut() {
    if (m_strut != 0) {
        screen().clearStrut(m_strut);
        m_strut = 0;
    }
}

void Slit::updateStrut() {
    clearStrut();
    // no need for area if we're autohiding
    if (doAutoHide())
        return;

    int left = 0, right = 0, top = 0, bottom = 0;
    switch (placement()) {
    case TOPLEFT:
        top = height();
        left = width();
        break;
    case TOPCENTER:
        top = height();
        break;
    case TOPRIGHT:
        right = width();
        top = height();
        break;
    case BOTTOMLEFT:
        bottom = height();
        left = width();
        break;
    case BOTTOMCENTER:
        // would be strange to have it request size on vertical direction
        // each time we add a client
        if (direction() == HORIZONTAL)
            bottom = height();
        break;
    case BOTTOMRIGHT:
        if (direction() == HORIZONTAL)
            bottom = height();
        else
            right = width();
        break;
    case CENTERLEFT:
        if (direction() == VERTICAL)
            left = width();        
        break;
    case CENTERRIGHT:
        if (direction() == VERTICAL)
            right = width();
        break;
    }
    m_strut = screen().requestStrut(left, right, top, bottom);
    screen().updateAvailableWorkspaceArea();
}

void Slit::addClient(Window w) {
#ifdef DEBUG
    cerr<<__FILE__": addClient(w = 0x"<<hex<<w<<dec<<")"<<endl;
#endif // DEBUG
    // Can't add non existent window
    if (w == None)
        return;

    // Look for slot in client list by name
    SlitClient *client = 0;
    std::string match_name;
    ::getWMName(&screen(), w, match_name);
    SlitClients::iterator it = m_client_list.begin();
    SlitClients::iterator it_end = m_client_list.end();
    bool found_match = false;
    for (; it != it_end; ++it) {
        // If the name matches...
        if ((*it)->match_name == match_name) {
            // Use the slot if no window is assigned
            if ((*it)->window == None) {
                client = (*it);
                client->initialize(&screen(), w);
                break;
            }
            // Otherwise keep looking for an unused match or a non-match
            found_match = true;		// Possibly redundant
			
        } else if (found_match) {
            // Insert before first non-match after a previously found match?
            client = new SlitClient(&screen(), w);
            m_client_list.insert(it, client);
            break;
        }
    }
    // Append to client list?
    if (client == 0) {
        client = new SlitClient(&screen(), w);
        m_client_list.push_back(client);
    }

    Display *disp = FbTk::App::instance()->display();
    XWMHints *wmhints = XGetWMHints(disp, w);

    if (wmhints != 0) {
        if ((wmhints->flags & IconWindowHint) &&
            (wmhints->icon_window != None)) {
            XMoveWindow(disp, client->client_window, screen().width() + 10,
                        screen().height() + 10);
            XMapWindow(disp, client->client_window);				
            client->icon_window = wmhints->icon_window;
            client->window = client->icon_window;
        } else {
            client->icon_window = None;
            client->window = client->client_window;
        }

        XFree(wmhints);
    } else {
        client->icon_window = None;
        client->window = client->client_window;
    }

    XWindowAttributes attrib;
    
#ifdef KDE

    //Check and see if new client is a KDE dock applet
    //If so force reasonable size
    bool iskdedockapp = false;
    Atom ajunk;
    int ijunk;
    unsigned long *data = 0, uljunk;

    // Check if KDE v2.x dock applet
    if (XGetWindowProperty(disp, w,
                           m_kwm2_dockwindow, 0l, 1l, False,
                           m_kwm2_dockwindow,
                           &ajunk, &ijunk, &uljunk, &uljunk,
                           (unsigned char **) &data) == Success) {
        iskdedockapp = (data && data[0] != 0);
        XFree((char *) data);
    }

    // Check if KDE v1.x dock applet
    if (!iskdedockapp) {
        if (XGetWindowProperty(disp, w,
                               m_kwm1_dockwindow, 0l, 1l, False,
                               m_kwm1_dockwindow,
                               &ajunk, &ijunk, &uljunk, &uljunk,
                               (unsigned char **) &data) == Success) {
            iskdedockapp = (data && data[0] != 0);
            XFree((char *) data);
        }
    }

    if (iskdedockapp)
        client->width = client->height = 24;
    else
#endif // KDE
    
        {
            if (XGetWindowAttributes(disp, client->window, &attrib)) {
                client->width = attrib.width;
                client->height = attrib.height;
            } else { // set default size if we failed to get window attributes
                client->width = client->height = 64;
            }
        }

    // disable border for client window
    XSetWindowBorderWidth(disp, client->window, 0);

    // disable events to frame.window
    frame.window.setEventMask(NoEventMask);
    client->disableEvents();
    

    XReparentWindow(disp, client->window, frame.window.window(), 0, 0);
    XMapRaised(disp, client->window);
    XChangeSaveSet(disp, client->window, SetModeInsert);
    // reactivate events for frame.window
    frame.window.setEventMask(s_eventmask);
    // setup event for slit client window
    client->enableEvents();
    
    // flush events
    XFlush(disp);

    // add slit client to eventmanager
    FbTk::EventManager::instance()->add(*this, client->client_window);
    FbTk::EventManager::instance()->add(*this, client->icon_window);

    frame.window.show();
    clearWindow();
    reconfigure();

    updateClientmenu();

    saveClientList();

}

void Slit::setDirection(Direction dir) {
    m_direction = dir;
    screen().saveSlitDirection(dir);
    reconfigure();
}

void Slit::setPlacement(Placement place) {
    m_placement = place;
    screen().saveSlitPlacement(place);
    reconfigure();
}

void Slit::removeClient(SlitClient *client, bool remap, bool destroy) {
#ifdef DEBUG
    cerr<<"Slit::removeClient( client->client_window = 0x"<<hex<<client->client_window<<
        ", client->icon_window)"<<endl;
#endif // DEBUG
    // remove from event manager
    if (client->client_window != 0)
        FbTk::EventManager::instance()->remove(client->client_window);
    if (client->icon_window != 0)
        FbTk::EventManager::instance()->remove(client->icon_window);

    // Destructive removal?
    if (destroy)
        m_client_list.remove(client);
    else // Clear the window info, but keep around to help future sorting?
        client->initialize();

    screen().removeNetizen(client->window);

    if (remap && client->window != 0) {
        Display *disp = FbTk::App::instance()->display();

        if (!client->visible)
            XMapWindow(disp, client->window);

        client->disableEvents();
        // stop events to frame.window temporarly
        frame.window.setEventMask(NoEventMask);
        XReparentWindow(disp, client->window, screen().rootWindow().window(),
			client->x, client->y);
        XChangeSaveSet(disp, client->window, SetModeDelete);
        // reactivate events to frame.window
        frame.window.setEventMask(s_eventmask);
        XFlush(disp);
    }

    // Destructive removal?
    if (destroy)
        delete client;

    updateClientmenu();
}


void Slit::removeClient(Window w, bool remap) {
#ifdef DEBUG
    cerr<<"Slit::removeClient(Window w = 0x"<<hex<<w<<dec<<", remap = "<<remap<<")"<<endl;
#endif // DEBUG
    if (w == frame.window)
        return;

    bool reconf = false;

    SlitClients::iterator it = m_client_list.begin();
    SlitClients::iterator it_end = m_client_list.end();
    for (; it != it_end; ++it) {
        if ((*it)->window == w) {
            removeClient((*it), remap, false);
            reconf = true;

            break;
        }
    }
    if (reconf)
        reconfigure();

}


void Slit::reconfigure() {
    m_transp->setAlpha(*screen().slitAlphaResource());

    frame.width = 0;
    frame.height = 0;

    // be sure to sync slit auto hide up with the screen's menu resource
    m_do_auto_hide = screen().doSlitAutoHide();

    // Need to count windows because not all client list entries
    // actually correspond to mapped windows.
    int num_windows = 0;
    const int bevel_width = screen().rootTheme().bevelWidth();

    switch (direction()) {
    case VERTICAL: {
        SlitClients::iterator it = m_client_list.begin();
        SlitClients::iterator it_end = m_client_list.end();
        for (; it != it_end; ++it) {
            // client created window?
            if ((*it)->window != None && (*it)->visible) {
                num_windows++;
                frame.height += (*it)->height + bevel_width;					
                
                if (frame.width < (*it)->width) 
                    frame.width = (*it)->width;
            }
        }
    }

        break;

    case HORIZONTAL: {
        SlitClients::iterator it = m_client_list.begin();
        SlitClients::iterator it_end = m_client_list.end();
        for (; it != it_end; ++it) {
            //client created window?
            if ((*it)->window != None && (*it)->visible) {
                num_windows++;
                frame.width += (*it)->width + bevel_width;
                //frame height < client height?
                if (frame.height < (*it)->height)
                    frame.height = (*it)->height;
            }
        }
     
    }
        break;
    } // end switch

    if (frame.width < 1)
        frame.width = 1;
    else
        frame.width += bevel_width;

    if (frame.height < 1)
        frame.height = 1;
    else
        frame.height += bevel_width*2;

    reposition();
    Display *disp = FbTk::App::instance()->display();

    frame.window.setBorderWidth(screen().rootTheme().borderWidth());
    frame.window.setBorderColor(screen().rootTheme().borderColor());
    // did we actually use slit slots
    if (num_windows == 0)
        frame.window.hide();
    else
        frame.window.show();

    Pixmap tmp = frame.pixmap;
    FbTk::ImageControl &image_ctrl = screen().imageControl();
    const FbTk::Texture &texture = m_slit_theme->texture();
    if (texture.type() == (FbTk::Texture::FLAT | FbTk::Texture::SOLID) &&
        texture.pixmap().drawable() == 0) {
        frame.pixmap = None;
        frame.window.setBackgroundColor(texture.color());
    } else {
        frame.pixmap = image_ctrl.renderImage(frame.width, frame.height,
                                               texture);
        if (frame.pixmap == 0)
            frame.window.setBackgroundColor(texture.color());
        else
            frame.window.setBackgroundPixmap(frame.pixmap);
    }

    if (tmp) 
        image_ctrl.removeImage(tmp);

    clearWindow();
    int x, y;

    switch (direction()) {
    case VERTICAL:
        x = 0;
        y = bevel_width;

        {
            SlitClients::iterator it = m_client_list.begin();
            SlitClients::iterator it_end = m_client_list.end();
            for (; it != it_end; ++it) {
                if ((*it)->window == None)
                    continue;

                //client created window?
                if ((*it)->visible) 
                    XMapWindow(disp, (*it)->window);
                else {
                    (*it)->disableEvents();
                    XUnmapWindow(disp, (*it)->window);
                    (*it)->enableEvents();
                    continue;
                }

                x = (frame.width - (*it)->width) / 2;                

                XMoveResizeWindow(disp, (*it)->window, x, y,
                                  (*it)->width, (*it)->height);

                // for ICCCM compliance
                (*it)->x = x;
                (*it)->y = y;

                XEvent event;
                event.type = ConfigureNotify;

                event.xconfigure.display = disp;
                event.xconfigure.event = (*it)->window;
                event.xconfigure.window = (*it)->window;
                event.xconfigure.x = x;
                event.xconfigure.y = y;
                event.xconfigure.width = (*it)->width;
                event.xconfigure.height = (*it)->height;
                event.xconfigure.border_width = 0;
                event.xconfigure.above = frame.window.window();
                event.xconfigure.override_redirect = False;

                XSendEvent(disp, (*it)->window, False, StructureNotifyMask,
                           &event);

                y += (*it)->height + bevel_width;
            } // end for
        }

        break;

    case HORIZONTAL:
        x = bevel_width;
        y = 0;

        {
            SlitClients::iterator it = m_client_list.begin();
            SlitClients::iterator it_end = m_client_list.end();
            for (; it != it_end; ++it) {
                //client created window?
                if ((*it)->window == None)
                    continue;

                if ((*it)->visible) { 
                    XMapWindow(disp, (*it)->window);
                } else {
                    (*it)->disableEvents();
                    XUnmapWindow(disp, (*it)->window);
                    (*it)->enableEvents();
                    continue;
                }

                y = (frame.height - (*it)->height) / 2;

                XMoveResizeWindow(disp, (*it)->window, x, y,
                                  (*it)->width, (*it)->height);





                // for ICCCM compliance
                (*it)->x = x;
                (*it)->y = y;

                XEvent event;
                event.type = ConfigureNotify;

                event.xconfigure.display = disp;
                event.xconfigure.event = (*it)->window;
                event.xconfigure.window = (*it)->window;
                event.xconfigure.x = frame.x + x + screen().rootTheme().borderWidth();
                event.xconfigure.y = frame.y + y + screen().rootTheme().borderWidth();
                event.xconfigure.width = (*it)->width;
                event.xconfigure.height = (*it)->height;
                event.xconfigure.border_width = 0;
                event.xconfigure.above = frame.window.window();
                event.xconfigure.override_redirect = False;

                XSendEvent(disp, (*it)->window, False, StructureNotifyMask,
                           &event);

                x += (*it)->width + bevel_width;
            }
        }

        break;
    }

    if (doAutoHide() && !isHidden() && !m_timer.isTiming()) 
        m_timer.start();

    m_slitmenu.reconfigure();
    updateClientmenu();
    updateStrut();
}


void Slit::reposition() {
    int head_x = 0,
        head_y = 0,
        head_w,
        head_h;

    if (screen().hasXinerama()) {
        int head = screen().getSlitOnHead();
        head_x = screen().getHeadX(head);
        head_y = screen().getHeadY(head);
        head_w = screen().getHeadWidth(head);
        head_h = screen().getHeadHeight(head);
    } else {
        head_w = screen().width();
        head_h = screen().height();
    }

    int border_width = screen().rootTheme().borderWidth();
    int bevel_width = screen().rootTheme().bevelWidth();

    // place the slit in the appropriate place
    switch (placement()) {
    case TOPLEFT:
        frame.x = head_x;
        frame.y = head_y;
        if (direction() == VERTICAL) {
            frame.x_hidden = bevel_width -
                border_width - frame.width;
            frame.y_hidden = head_y;
        } else {
            frame.x_hidden = head_x;
            frame.y_hidden = bevel_width -
                border_width - frame.height;
        }
        break;

    case CENTERLEFT:
        frame.x = head_x;
        frame.y = head_y + (head_h - frame.height) / 2;
        frame.x_hidden = head_x + bevel_width -
            border_width - frame.width;
        frame.y_hidden = frame.y;
        break;

    case BOTTOMLEFT:
        frame.x = head_x;
        frame.y = head_h - frame.height - border_width*2;
        if (direction() == VERTICAL) {
            frame.x_hidden = head_x + bevel_width -
                border_width - frame.width;
            frame.y_hidden = frame.y;
        } else {
            frame.x_hidden = head_x;
            frame.y_hidden = head_y + head_h -
                bevel_width - border_width;
        }
        break;

    case TOPCENTER:
        frame.x = head_x + ((head_w - frame.width) / 2);
        frame.y = head_y;
        frame.x_hidden = frame.x;
        frame.y_hidden = head_y + bevel_width -
            border_width - frame.height;
        break;

    case BOTTOMCENTER:
        frame.x = head_x + ((head_w - frame.width) / 2);
        frame.y = head_y + head_h - frame.height - border_width*2;
        frame.x_hidden = frame.x;
        frame.y_hidden = head_y + head_h -
            bevel_width - border_width;
        break;

    case TOPRIGHT:
        frame.x = head_x + head_w - frame.width - border_width*2;
        frame.y = head_y;
        if (direction() == VERTICAL) {
            frame.x_hidden = head_x + head_w -
                bevel_width - border_width;
            frame.y_hidden = head_y;
        } else {
            frame.x_hidden = frame.x;
            frame.y_hidden = head_y + bevel_width -
                border_width - frame.height;
        }
        break;

    case CENTERRIGHT:
    default:
        frame.x = head_x + head_w - frame.width - border_width*2;
        frame.y = head_y + ((head_h - frame.height) / 2);
        frame.x_hidden = head_x + head_w -
            bevel_width - border_width;
        frame.y_hidden = frame.y;
        break;

    case BOTTOMRIGHT:
        frame.x = head_x + head_w - frame.width - border_width*2;
        frame.y = head_y + head_h - frame.height - border_width*2;
        if (direction() == VERTICAL) {
            frame.x_hidden = head_x + head_w - 
                bevel_width - border_width;
            frame.y_hidden = frame.y;
        } else {
            frame.x_hidden = frame.x;
            frame.y_hidden = head_y + head_h - 
                bevel_width - border_width;
        }
        break;
    }

    if (isHidden()) {
        frame.window.moveResize(frame.x_hidden, frame.y_hidden,
                                frame.width, frame.height);
    } else {
        frame.window.moveResize(frame.x,  frame.y,
                                frame.width, frame.height);
    }
}


void Slit::shutdown() {
    saveClientList();
    while (!m_client_list.empty())
        removeClient(m_client_list.front(), true, true);
}

void Slit::cycleClientsUp() {
    if (m_client_list.size() < 2)
        return;

    // rotate client list up, ie the first goes last
    SlitClients::iterator it = m_client_list.begin();
    SlitClient *client = *it;
    m_client_list.erase(it);
    m_client_list.push_back(client);
    reconfigure();
}

void Slit::cycleClientsDown() {
    if (m_client_list.size() < 2)
        return;

    // rotate client list down, ie the last goes first
    SlitClient *client = m_client_list.back();
    m_client_list.remove(client);
    m_client_list.push_front(client);
    reconfigure();
}

void Slit::handleEvent(XEvent &event) {
    if (event.type == ConfigureRequest) {
        configureRequestEvent(event.xconfigurerequest);
    } else if (event.type == DestroyNotify) {
        removeClient(event.xdestroywindow.window, false);
    } else if (event.type == UnmapNotify) {        
       removeClient(event.xunmap.window);
    } else if (event.type == MapRequest) {
#ifdef KDE
        //Check and see if client is KDE dock applet.
        //If so add to Slit
        bool iskdedockapp = false;
        Atom ajunk;
        int ijunk;
        unsigned long *data = (unsigned long *) 0, uljunk;
        Display *disp = FbTk::App::instance()->display();
        // Check if KDE v2.x dock applet
        if (XGetWindowProperty(disp, event.xmaprequest.window,
                               m_kwm2_dockwindow, 0l, 1l, False,
                               XA_WINDOW, &ajunk, &ijunk, &uljunk,
                               &uljunk, (unsigned char **) &data) == Success) {
					
            if (data)
                iskdedockapp = True;
            XFree((char *) data);
	
        }

        // Check if KDE v1.x dock applet
        if (!iskdedockapp) {
            if (XGetWindowProperty(disp, event.xmaprequest.window,
                                   m_kwm1_dockwindow, 0l, 1l, False,
                                   m_kwm1_dockwindow, &ajunk, &ijunk, &uljunk,
                                   &uljunk, (unsigned char **) &data) == Success) {
                iskdedockapp = (data && data[0] != 0);
                XFree((char *) data);
            }
        }

        if (iskdedockapp) {
            XSelectInput(disp, event.xmaprequest.window, StructureNotifyMask);
            addClient(event.xmaprequest.window);
        }
#endif //KDE
        
    }
}

void Slit::buttonPressEvent(XButtonEvent &e) {
    if (e.window != frame.window.window()) 
        return;

    if (e.button == Button3) {
        if (! m_slitmenu.isVisible()) {
            int x = e.x_root - (m_slitmenu.width() / 2),
                y = e.y_root - (m_slitmenu.height() / 2); 

            if (x < 0)
                x = 0;
            else if (x + m_slitmenu.width() > screen().width())
                x = screen().width() - m_slitmenu.width();

            if (y < 0)
                y = 0;
            else if (y + m_slitmenu.height() > screen().height())
                y = screen().height() - m_slitmenu.height();

            m_slitmenu.move(x, y);
            m_slitmenu.show();
        } else
            m_slitmenu.hide();
    }
}


void Slit::enterNotifyEvent(XCrossingEvent &) {
    if (! doAutoHide())
        return;

    if (isHidden()) {
        if (! m_timer.isTiming()) 
            m_timer.start();
    } else {
        if (m_timer.isTiming()) 
            m_timer.stop();
    }
}


void Slit::leaveNotifyEvent(XCrossingEvent &ev) {
    if (! doAutoHide())
        return;

    if (isHidden()) {
        if (m_timer.isTiming()) 
            m_timer.stop();
    } else {
        if (! m_timer.isTiming()) {
            // the menu is open, keep it firing until it closes
            if (m_slitmenu.isVisible()) 
                m_timer.fireOnce(false);
            m_timer.start();
        }
    }

}


void Slit::configureRequestEvent(XConfigureRequestEvent &event) {
    bool reconf = false;
    XWindowChanges xwc;

    xwc.x = event.x;
    xwc.y = event.y;
    xwc.width = event.width;
    xwc.height = event.height;
    xwc.border_width = 0;
    xwc.sibling = event.above;
    xwc.stack_mode = event.detail;

    XConfigureWindow(FbTk::App::instance()->display(), 
                     event.window, event.value_mask, &xwc);

    SlitClients::iterator it = m_client_list.begin();
    SlitClients::iterator it_end = m_client_list.end();
    for (; it != it_end; ++it) {
        if ((*it)->window == event.window) {
            if ((*it)->width != ((unsigned) event.width) ||
                (*it)->height != ((unsigned) event.height)) {
                (*it)->width = (unsigned) event.width;
                (*it)->height = (unsigned) event.height;

                reconf = true; //requires reconfiguration

                break;
            }
        }
    }

    if (reconf) 
        reconfigure();
}

void Slit::exposeEvent(XExposeEvent &ev) {
    clearWindow();
}

void Slit::clearWindow() {
    frame.window.clear();
    if (frame.pixmap != 0) {
        if (screen().rootPixmap() != m_transp->source())
            m_transp->setSource(screen().rootPixmap(), screen().screenNumber());

        m_transp->render(frame.window.x(), frame.window.y(),
                         0, 0,
                         frame.window.width(), frame.window.height());
        
    }

}

void Slit::timeout() {
    if (doAutoHide()) {
        if (!m_slitmenu.isVisible()) {
            m_timer.fireOnce(true);
        } else 
            return;
    } else 
        if (!isHidden()) return;
        
    m_hidden = ! m_hidden; // toggle hidden state
    if (isHidden())
        frame.window.move(frame.x_hidden, frame.y_hidden);
    else
        frame.window.move(frame.x, frame.y);
}

void Slit::loadClientList(const char *filename) {
    if (filename == 0)
        return;

    m_filename = filename; // save filename so we can save client list later

    struct stat buf;
    if (!stat(filename, &buf)) {
        std::ifstream file(filename);
        std::string name;
        while (! file.eof()) {
            name = "";
            std::getline(file, name); // get the entire line
            if (name.size() > 0) { // don't add client unless we have a valid line
                SlitClient *client = new SlitClient(name.c_str());
                m_client_list.push_back(client);
            }
        }
    }
}

void Slit::updateClientmenu() {
    // clear old items
    m_clientlist_menu.removeAll();
    m_clientlist_menu.setLabel("Clients");

    FbTk::RefCount<FbTk::Command> cycle_up(new FbTk::SimpleCommand<Slit>(*this, &Slit::cycleClientsUp));
    FbTk::RefCount<FbTk::Command> cycle_down(new FbTk::SimpleCommand<Slit>(*this, &Slit::cycleClientsDown));
    m_clientlist_menu.insert("Cycle Up", cycle_up);
    m_clientlist_menu.insert("Cycle Down", cycle_down);

    FbTk::MenuItem *separator = new FbTk::MenuItem("-------");
    separator->setEnabled(false);
    m_clientlist_menu.insert(separator);

    FbTk::RefCount<FbTk::Command> reconfig(new FbTk::SimpleCommand<Slit>(*this, &Slit::reconfigure));
    SlitClients::iterator it = m_client_list.begin();
    for (; it != m_client_list.end(); ++it) {
        if ((*it) != 0 && (*it)->window != 0)
            m_clientlist_menu.insert(new SlitClientMenuItem(*(*it), reconfig));
    }

    m_clientlist_menu.update();
}

void Slit::saveClientList() {

    std::ofstream file(m_filename.c_str());
    SlitClients::iterator it = m_client_list.begin();
    SlitClients::iterator it_end = m_client_list.end();
    std::string prevName;
    std::string name;
    for (; it != it_end; ++it) {
        name = (*it)->match_name;
        if (name != prevName)
            file << name.c_str() << std::endl;

        prevName = name;
    }
}

void Slit::setAutoHide(bool val) {
    m_do_auto_hide = val;
    screen().saveSlitAutoHide(val);
    if (doAutoHide()) {
        if (! m_timer.isTiming()) {
            if (m_slitmenu.isVisible())
                m_timer.fireOnce(false);
            m_timer.start();
        }
    }
}

void Slit::setupMenu() {
    I18n *i18n = I18n::instance();
    using namespace FBNLS;
    using namespace FbTk;

    FbTk::MacroCommand *s_a_reconf_macro = new FbTk::MacroCommand();
    FbTk::MacroCommand *s_a_reconf_slit_macro = new FbTk::MacroCommand();
    FbTk::RefCount<FbTk::Command> saverc_cmd(new FbTk::SimpleCommand<Fluxbox>(*Fluxbox::instance(), 
                                                                              &Fluxbox::save_rc));
    FbTk::RefCount<FbTk::Command> reconf_cmd(new FbCommands::ReconfigureFluxboxCmd());
    FbTk::RefCount<FbTk::Command> reconf_slit_cmd(new FbTk::SimpleCommand<Slit>(*this, &Slit::reconfigure));

    s_a_reconf_macro->add(saverc_cmd);
    s_a_reconf_macro->add(reconf_cmd);

    s_a_reconf_slit_macro->add(saverc_cmd);
    s_a_reconf_slit_macro->add(reconf_slit_cmd);

    FbTk::RefCount<FbTk::Command> save_and_reconfigure(s_a_reconf_macro);
    FbTk::RefCount<FbTk::Command> save_and_reconfigure_slit(s_a_reconf_slit_macro);

    // setup base menu
    m_slitmenu.setLabel("Slit");
    m_slitmenu.insert(i18n->getMessage(
                                       CommonSet, CommonPlacementTitle,
                                       "Placement"),
                      &m_placement_menu);

    m_slitmenu.insert("Layer...", m_layermenu.get());

#ifdef XINERAMA
    if (screen().hasXinerama()) {
        m_slitmenu.insert("On Head...", new XineramaHeadMenu<Slit>(
                        *screen().menuTheme(),
                        screen(),
                        screen().imageControl(),
                        *screen().layerManager().getLayer(Fluxbox::instance()->getMenuLayer()),
                        this
                        ));
    }
                    
#endif //XINERAMA
    m_slitmenu.insert(new BoolMenuItem(i18n->getMessage(
                                                        CommonSet, CommonAutoHide,
                                                        "Auto hide"),
                                       screen().doSlitAutoHide(),
                                       save_and_reconfigure_slit));

    FbTk::MenuItem *alpha_menuitem = new IntResMenuItem("Alpha", 
                                                        screen().slitAlphaResource(),
                                                        0, 255);
    alpha_menuitem->setCommand(save_and_reconfigure_slit);
    m_slitmenu.insert(alpha_menuitem);

    m_slitmenu.insert(new SlitDirMenuItem(i18n->getMessage(
                                                           SlitSet, SlitSlitDirection,
                                                           "Slit Direction"), 
                                          *this,
                                          save_and_reconfigure));
    m_slitmenu.insert("Clients", &m_clientlist_menu);
    m_slitmenu.update();

    // setup sub menu
    m_placement_menu.setLabel(i18n->getMessage(
                                             SlitSet, SlitSlitPlacement,
                                             "Slit Placement"));
    m_placement_menu.setMinimumSublevels(3);
    m_layermenu->setInternalMenu();
    m_clientlist_menu.setInternalMenu();
   

    // setup items in sub menu
    struct {
        int set;
        int base;
        const char *default_str;
        Placement slit_placement;
    } place_menu[]  = {
        {CommonSet, CommonPlacementTopLeft, "Top Left", Slit::TOPLEFT},
        {CommonSet, CommonPlacementCenterLeft, "Center Left", Slit::CENTERLEFT},
        {CommonSet, CommonPlacementBottomLeft, "Bottom Left", Slit::BOTTOMLEFT},
        {CommonSet, CommonPlacementTopCenter, "Top Center", Slit::TOPCENTER},
        {0, 0, 0, Slit::TOPLEFT}, // middle item, empty
        {CommonSet, CommonPlacementBottomCenter, "Bottom Center", Slit::BOTTOMCENTER},
        {CommonSet, CommonPlacementTopRight, "Top Right", Slit::TOPRIGHT},
        {CommonSet, CommonPlacementCenterRight, "Center Right", Slit::CENTERRIGHT},
        {CommonSet, CommonPlacementBottomRight, "Bottom Right", Slit::BOTTOMRIGHT}
    };
    // create items in sub menu
    for (size_t i=0; i<9; ++i) {
        if (place_menu[i].default_str == 0) {
            m_placement_menu.insert("");
        } else {
            const char *i18n_str = i18n->getMessage(place_menu[i].set, 
                                                    place_menu[i].base,
                                                    place_menu[i].default_str);
            m_placement_menu.insert(new PlaceSlitMenuItem(i18n_str, *this,
                                                        place_menu[i].slit_placement,
                                                        save_and_reconfigure));
        }
    }
    // finaly update sub menu
    m_placement_menu.update();
}

void Slit::moveToLayer(int layernum) {
    m_layeritem->moveToLayer(layernum);
    m_screen.saveSlitLayer((Fluxbox::Layer) layernum);
}
