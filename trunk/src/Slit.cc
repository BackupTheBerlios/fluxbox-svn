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

// $Id: Slit.cc,v 1.96 2004/06/07 21:36:06 fluxgen Exp $

#include "Slit.hh"

//use GNU extensions
#ifndef	 _GNU_SOURCE
#define	 _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "Screen.hh"
#include "ImageControl.hh"
#include "RefCount.hh"
#include "BoolMenuItem.hh"
#include "EventManager.hh"
#include "SimpleCommand.hh"
#include "MacroCommand.hh"
#include "FbCommands.hh"
#include "LayerMenu.hh"
#include "fluxbox.hh"
#include "XLayer.hh"
#include "RootTheme.hh"
#include "FbTk/Theme.hh"
#include "FbMenu.hh"
#include "Transparent.hh"
#include "IntResMenuItem.hh"
#include "Xinerama.hh"
#include "SlitTheme.hh"
#include "SlitClient.hh"
#include "Xutil.hh"
#include "FbAtoms.hh"
#include "FbTk/StringUtil.hh"
#include "FbTk/I18n.hh"

#include <algorithm>
#include <iostream>
#include <cassert>

#ifdef HAVE_SYS_STAT_H
#include <sys/types.h>
#include <sys/stat.h>
#endif // HAVE_SYS_STAT_H

#include <X11/Xatom.h>
#include <X11/Xlib.h>

#include <iostream>
#include <algorithm>
using namespace std;

template<>
void FbTk::Resource<Slit::Placement>::setFromString(const char *strval) {
    if (strcasecmp(strval, "TopLeft")==0)
        m_value = Slit::TOPLEFT;
    else if (strcasecmp(strval, "LeftCenter")==0
             || strcasecmp(strval, "CenterLeft")==0)
        m_value = Slit::LEFTCENTER;
    else if (strcasecmp(strval, "BottomLeft")==0)
        m_value = Slit::BOTTOMLEFT;
    else if (strcasecmp(strval, "TopCenter")==0)
        m_value = Slit::TOPCENTER;
    else if (strcasecmp(strval, "BottomCenter")==0)
        m_value = Slit::BOTTOMCENTER;
    else if (strcasecmp(strval, "TopRight")==0)
        m_value = Slit::TOPRIGHT;
    else if (strcasecmp(strval, "RightCenter")==0
             || strcasecmp(strval, "CenterRight")==0)
        m_value = Slit::RIGHTCENTER;
    else if (strcasecmp(strval, "BottomRight")==0)
        m_value = Slit::BOTTOMRIGHT;
    else
        setDefaultValue();
}

template<>
void FbTk::Resource<Slit::Direction>::setFromString(const char *strval) {
    if (strcasecmp(strval, "Vertical") == 0) 
        m_value = Slit::VERTICAL;
    else if (strcasecmp(strval, "Horizontal") == 0) 
        m_value = Slit::HORIZONTAL;
    else
        setDefaultValue();
}

template<>
string FbTk::Resource<Slit::Placement>::getString() {
    switch (m_value) {
    case Slit::TOPLEFT:
        return string("TopLeft");
        break;
    case Slit::LEFTCENTER:
        return string("LeftCenter");
        break;
    case Slit::BOTTOMLEFT:
        return string("BottomLeft");
        break;
    case Slit::TOPCENTER:
        return string("TopCenter");
        break;			
    case Slit::BOTTOMCENTER:
        return string("BottomCenter");
        break;
    case Slit::TOPRIGHT:
        return string("TopRight");
        break;
    case Slit::RIGHTCENTER:
        return string("RightCenter");
        break;
    case Slit::BOTTOMRIGHT:
        return string("BottomRight");
        break;
    }
    //default string
    return string("BottomRight");
}

template<>
string FbTk::Resource<Slit::Direction>::getString() {
    switch (m_value) {
    case Slit::VERTICAL:
        return string("Vertical");
        break;
    case Slit::HORIZONTAL:
        return string("Horizontal");
        break;
    }
    // default string
    return string("Vertical");
}

namespace { 

class SlitClientMenuItem: public FbTk::MenuItem {
public:
    explicit SlitClientMenuItem(SlitClient &client, FbTk::RefCount<FbTk::Command> &cmd):
        FbTk::MenuItem(client.matchName().c_str(), cmd), m_client(client) {
        FbTk::MenuItem::setSelected(client.visible());
        // save resources as default click action
        FbTk::RefCount<FbTk::Command> save_rc(new FbCommands::SaveResources());
        setCommand(save_rc);
    }
    const std::string &label() const {
        return m_client.matchName();
    }
    bool isSelected() const {
        return m_client.visible();
    }
    void click(int button, int time) { 
        m_client.setVisible(!m_client.visible());
        FbTk::MenuItem::click(button, time);
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
        _FB_USES_NLS;
        m_label = (label ? label : "");
        std::string reallabel = m_label + " " + 
            ( m_slit.direction() == Slit::HORIZONTAL ? 
              
              _FBTEXT(Align, Horizontal, "Horizontal", "Horizontal"):
              _FBTEXT(Align, Vertical,   "Vertical",   "Vertical"));
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

unsigned int Slit::s_eventmask = SubstructureRedirectMask |  ButtonPressMask | 
                                 EnterWindowMask | LeaveWindowMask | ExposureMask;

Slit::Slit(BScreen &scr, FbTk::XLayer &layer, const char *filename)
    : m_hidden(false),
      m_screen(scr),
      m_slitmenu(scr.menuTheme(), 
                 scr.imageControl(),
                 *scr.layerManager().getLayer(Fluxbox::instance()->getMenuLayer())),
      m_placement_menu(scr.menuTheme(),
                       scr.imageControl(),
                       *scr.layerManager().getLayer(Fluxbox::instance()->getMenuLayer())),
      m_clientlist_menu(scr.menuTheme(),
                        scr.imageControl(),
                        *scr.layerManager().getLayer(Fluxbox::instance()->getMenuLayer())),
       //For KDE dock applets
      m_kwm1_dockwindow(XInternAtom(FbTk::App::instance()->display(), 
                                    "KWM_DOCKWINDOW", False)), //KDE v1.x
      m_kwm2_dockwindow(XInternAtom(FbTk::App::instance()->display(), 
                                    "_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR", False)), //KDE v2.x

      m_layeritem(0),
      m_slit_theme(new SlitTheme(scr.rootWindow().screenNumber())),
      m_strut(0),
      // resources
      // lock in first resource
      m_rc_auto_hide(scr.resourceManager().lock(), false, 
                     scr.name() + ".slit.autoHide", scr.altName() + ".Slit.AutoHide"),
      // TODO: this resource name must change
      m_rc_maximize_over(scr.resourceManager(), false,
                         scr.name() + ".slit.maxOver", scr.altName() + ".Slit.MaxOver"),
      m_rc_placement(scr.resourceManager(), BOTTOMRIGHT,
                     scr.name() + ".slit.placement", scr.altName() + ".Slit.Placement"),
      m_rc_direction(scr.resourceManager(), VERTICAL, 
                     scr.name() + ".slit.direction", scr.altName() + ".Slit.Direction"),
      m_rc_alpha(scr.resourceManager(), 255, 
                 scr.name() + ".slit.alpha", scr.altName() + ".Slit.Alpha"),
      m_rc_on_head(scr.resourceManager(), 0,
                   scr.name() + ".slit.onhead", scr.altName() + ".Slit.onHead"),
      m_rc_layernum(scr.resourceManager(), Fluxbox::Layer(Fluxbox::instance()->getDockLayer()), 
                    scr.name() + ".slit.layer", scr.altName() + ".Slit.Layer") {

    _FB_USES_NLS;

    // attach to theme and root window change signal
    m_slit_theme->reconfigSig().attach(this);
    scr.resizeSig().attach(this);
    scr.reconfigureSig().attach(this); // if alpha changed (we disablethis signal when we get theme change sig)

    frame.pixmap = None;
    // setup timer
    m_timer.setTimeout(200); // default timeout
    m_timer.fireOnce(true);
    FbTk::RefCount<FbTk::Command> toggle_hidden(new FbTk::SimpleCommand<Slit>(*this, &Slit::toggleHidden));
    m_timer.setCommand(toggle_hidden);

    // create main window
    XSetWindowAttributes attrib;
    unsigned long create_mask = CWBackPixmap | CWBackPixel | CWBorderPixel |
        CWColormap | CWOverrideRedirect | CWEventMask;
    attrib.background_pixmap = None;
    attrib.background_pixel = attrib.border_pixel = theme().borderColor().pixel();
    attrib.colormap = screen().rootWindow().colormap();
    attrib.override_redirect = True;
    attrib.event_mask = s_eventmask;

    frame.x = frame.y = 0;
    frame.width = frame.height = 1;
    Display *disp = FbTk::App::instance()->display();
    frame.window =
        XCreateWindow(disp, screen().rootWindow().window(), frame.x, frame.y,
                      frame.width, frame.height, theme().borderWidth(),
                      screen().rootWindow().depth(), InputOutput, screen().rootWindow().visual(),
                      create_mask, &attrib);

    FbTk::EventManager::instance()->add(*this, frame.window);
    
    frame.window.setAlpha(*m_rc_alpha);
    m_layeritem.reset(new FbTk::XLayerItem(frame.window, layer));
    m_layermenu.reset(new LayerMenu<Slit>(scr.menuTheme(),
                                    scr.imageControl(),
                                    *scr.layerManager().
                                    getLayer(Fluxbox::instance()->getMenuLayer()), 
                                          this,
                                          true));
    moveToLayer((*m_rc_layernum).getNum());

    if (m_layermenu.get())
        m_layermenu->setLabel(_FBTEXT(Slit, Layer, "Slit Layer", "Title of Slit Layer Menu"));

    // Get client list for sorting purposes
    loadClientList(filename);

    setupMenu();

    scr.resourceManager().unlock();
}


Slit::~Slit() {
    clearStrut();
    if (frame.pixmap != 0)
        screen().imageControl().removeImage(frame.pixmap);

    shutdown();
}

void Slit::clearStrut() {
    if (m_strut != 0) {
        screen().clearStrut(m_strut);
        m_strut = 0;
    }
}

void Slit::updateStrut() {
    bool had_strut = m_strut ? true : false;
    clearStrut();
    // no need for area if we're autohiding or set maximize over
    if (doAutoHide() || *m_rc_maximize_over) {
        // update screen area if we had a strut before
        if (had_strut)
            screen().updateAvailableWorkspaceArea();
        return;
    }


    int left = 0, right = 0, top = 0, bottom = 0;
    switch (placement()) {
    case TOPLEFT:
        if (direction() == HORIZONTAL)
            top = height();
        else
            left = width();
        break;
    case TOPCENTER:
        if (direction() == HORIZONTAL)
            top = height();
        break;
    case TOPRIGHT:
        if (direction() == HORIZONTAL)
            top = height();
        else
            right = width();
        break;
    case BOTTOMLEFT:
        if (direction() == HORIZONTAL)
            bottom = height();
        else
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
    case LEFTCENTER:
        if (direction() == VERTICAL)
            left = width();        
        break;
    case RIGHTCENTER:
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
    match_name = Xutil::getWMClassName(w);
    SlitClients::iterator it = m_client_list.begin();
    SlitClients::iterator it_end = m_client_list.end();
    bool found_match = false;
    for (; it != it_end; ++it) {
        // If the name matches...
        if ((*it)->matchName() == match_name) {
            // Use the slot if no window is assigned
            if ((*it)->window() == None) {
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
            XMoveWindow(disp, client->clientWindow(), screen().width() + 10,
                        screen().height() + 10);            
            XMapWindow(disp, client->clientWindow());
            client->setIconWindow(wmhints->icon_window);
            client->setWindow(client->iconWindow());
        } else {
            client->setIconWindow(None);
            client->setWindow(client->clientWindow());
        }

        XFree((void *) wmhints);
    } else {
        client->setIconWindow(None);
        client->setWindow(client->clientWindow());
    }

    Atom *proto = 0;
    int num_return = 0;
    FbAtoms *fbatoms = FbAtoms::instance();

    if (XGetWMProtocols(disp, w, &proto, &num_return)) {
        
        for (int i = 0; i < num_return; ++i) {
            if (proto[i] == fbatoms->getFluxboxStructureMessagesAtom())
                screen().addNetizen(w);
        }
        
        XFree((void *) proto);
    } else {
        cerr<<"Warning: Failed to read WM Protocols. "<<endl;
    }

    XWindowAttributes attrib;
    
#ifdef KDE
    if (screen().isKdeDockapp(w))   
        client->resize(24, 24);
    else
#endif // KDE
    
        {
            if (XGetWindowAttributes(disp, client->window(), &attrib)) {
                client->resize(attrib.width, attrib.height);
            } else { // set default size if we failed to get window attributes
                client->resize(64, 64);
            }
        }

    // disable border for client window
    XSetWindowBorderWidth(disp, client->window(), 0);

    // disable events to frame.window
    frame.window.setEventMask(NoEventMask);
    client->disableEvents();
    

    XReparentWindow(disp, client->window(), frame.window.window(), 0, 0);
    XMapRaised(disp, client->window());
    XChangeSaveSet(disp, client->window(), SetModeInsert);
    // reactivate events for frame.window
    frame.window.setEventMask(s_eventmask);
    // setup event for slit client window
    client->enableEvents();
    
    // flush events
    //    XFlush(disp);

    // add slit client to eventmanager
    FbTk::EventManager::instance()->add(*this, client->clientWindow());
    FbTk::EventManager::instance()->add(*this, client->iconWindow());

    //    frame.window.show();
    clearWindow();
    reconfigure();

    updateClientmenu();

    saveClientList();

}

void Slit::setDirection(Direction dir) {
    *m_rc_direction = dir;
    reconfigure();
}

void Slit::setPlacement(Placement place) {
    *m_rc_placement = place;
    reconfigure();
}

void Slit::removeClient(SlitClient *client, bool remap, bool destroy) {
    if (client == 0)
        return;

    // remove from event manager
    if (client->clientWindow() != 0)
        FbTk::EventManager::instance()->remove(client->clientWindow());
    if (client->iconWindow() != 0)
        FbTk::EventManager::instance()->remove(client->iconWindow());

    // Destructive removal?
    if (destroy)
        m_client_list.remove(client);
    else // Clear the window info, but keep around to help future sorting?
        client->initialize();

    screen().removeNetizen(client->window());

    if (remap && client->window() != 0) {
        Display *disp = FbTk::App::instance()->display();

        if (!client->visible())
            client->show();

        client->disableEvents();
        // stop events to frame.window temporarly
        frame.window.setEventMask(NoEventMask);
        XReparentWindow(disp, client->window(), screen().rootWindow().window(),
			client->x(), client->y());
        XChangeSaveSet(disp, client->window(), SetModeDelete);
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

    if (w == frame.window)
        return;

    bool reconf = false;

    SlitClients::iterator it = m_client_list.begin();
    SlitClients::iterator it_end = m_client_list.end();
    for (; it != it_end; ++it) {
        if ((*it)->window() == w) {
            removeClient((*it), remap, false);
            reconf = true;

            break;
        }
    }
    if (reconf)
        reconfigure();

}


void Slit::reconfigure() {

    frame.width = 0;
    frame.height = 0;

    // Need to count windows because not all client list entries
    // actually correspond to mapped windows.
    int num_windows = 0;
    const int bevel_width = theme().bevelWidth();
    // determine width or height increase
    bool height_inc = false;
    switch (direction()) {
    case VERTICAL:
        height_inc = true;
        break;
    case HORIZONTAL: // already false
        break;
    }

    SlitClients::iterator client_it = m_client_list.begin();
    SlitClients::iterator client_it_end = m_client_list.end();
    for (; client_it != client_it_end; ++client_it) {
        // client created window?
        if ((*client_it)->window() != None && (*client_it)->visible()) {
            num_windows++;
            if (height_inc) {
                // increase height of slit for each client (VERTICAL mode)
                frame.height += (*client_it)->height() + bevel_width;		
                // the slit should always have the width of the largest client
                if (frame.width < (*client_it)->width()) 
                    frame.width = (*client_it)->width();
            } else {
                // increase width of slit for each client (HORIZONTAL mode)
                frame.width += (*client_it)->width() + bevel_width;
                // the slit should always have the width of the largest client
                if (frame.height < (*client_it)->height())
                    frame.height = (*client_it)->height();
            }
        }
    }

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

    frame.window.setBorderWidth(theme().borderWidth());
    frame.window.setBorderColor(theme().borderColor());
    // did we actually use slit slots
    if (num_windows == 0)
        frame.window.hide();
    else
        frame.window.show();

    Pixmap tmp = frame.pixmap;
    FbTk::ImageControl &image_ctrl = screen().imageControl();
    const FbTk::Texture &texture = m_slit_theme->texture();
    if (!texture.usePixmap()) {
        frame.pixmap = 0;
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

    frame.window.setAlpha(*m_rc_alpha);
    clearWindow();

    int x = 0, y = 0;
    height_inc = false;
    switch (direction()) {
    case VERTICAL:
        x = 0;
        y = bevel_width;
        height_inc = true;
        break;

    case HORIZONTAL:
        x = bevel_width;
        y = 0;
        break;
    }

    client_it = m_client_list.begin();
    for (; client_it != client_it_end; ++client_it) {
        if ((*client_it)->window() == None)
            continue;

        //client created window?
        if ((*client_it)->visible()) 
            (*client_it)->show();
        else {
            (*client_it)->disableEvents();
            (*client_it)->hide();
            (*client_it)->enableEvents();
            continue;
        }

        if (height_inc)
            x = (frame.width - (*client_it)->width()) / 2;
        else
            y = (frame.height - (*client_it)->height()) / 2;

        XMoveResizeWindow(disp, (*client_it)->window(), x, y,
                          (*client_it)->width(), (*client_it)->height());

        // for ICCCM compliance
        (*client_it)->move(x, y);

        XEvent event;
        event.type = ConfigureNotify;

        event.xconfigure.display = disp;
        event.xconfigure.event = (*client_it)->window();
        event.xconfigure.window = (*client_it)->window();
        event.xconfigure.x = (*client_it)->x();
        event.xconfigure.y = (*client_it)->y();
        event.xconfigure.width = (*client_it)->width();
        event.xconfigure.height = (*client_it)->height();
        event.xconfigure.border_width = 0;
        event.xconfigure.above = frame.window.window();
        event.xconfigure.override_redirect = False;

        XSendEvent(disp, (*client_it)->window(), False, StructureNotifyMask,
                   &event);

        if (height_inc)
            y += (*client_it)->height() + bevel_width;
        else
            x += (*client_it)->width() + bevel_width;
    } // end for

    if (doAutoHide() && !isHidden() && !m_timer.isTiming()) 
        m_timer.start();
    else if (!doAutoHide() && isHidden())
        toggleHidden(); // restore visible

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
        int head = *m_rc_on_head;
        head_x = screen().getHeadX(head);
        head_y = screen().getHeadY(head);
        head_w = screen().getHeadWidth(head);
        head_h = screen().getHeadHeight(head);
    } else {
        head_w = screen().width();
        head_h = screen().height();
    }

    int border_width = theme().borderWidth();
    int bevel_width = theme().bevelWidth();
    // make sure at leaste one pixel is visible
    if (border_width >= bevel_width)
        bevel_width = border_width + 1;
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

    case LEFTCENTER:
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

    case RIGHTCENTER:
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
    } else if (event.type == UnmapNotify && event.xany.send_event) {
        // we ignore server-generated events, which can occur
        // on restart. The ICCCM says that a client must send
        // a synthetic event for the withdrawn state
        removeClient(event.xunmap.window);
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
        if ((*it)->window() == event.window) {
            if ((*it)->width() != ((unsigned) event.width) ||
                (*it)->height() != ((unsigned) event.height)) {
                (*it)->resize(event.width, event.height);;

                reconf = true; //requires reconfiguration

                break;
            }
        }
    }

    if (reconf) 
        reconfigure();
}

void Slit::exposeEvent(XExposeEvent &ev) {
    // we don't need to clear the entire window 
    // just the are that gets exposed
    frame.window.clearArea(ev.x, ev.y, ev.width, ev.height);
    frame.window.updateTransparent(ev.x, ev.y, ev.width, ev.height);
}

void Slit::update(FbTk::Subject *subj) {
    reconfigure();
}

void Slit::clearWindow() {
    frame.window.clear();
    frame.window.updateTransparent();
}

void Slit::toggleHidden() {
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
    if (filename == 0 || filename[0] == '\0')
        return;

    // save filename so we can save client list later
    m_filename = filename;
    std::string real_filename= FbTk::StringUtil::expandFilename(filename); 

    struct stat buf;
    if (stat(real_filename.c_str(), &buf) == 0) {
        std::ifstream file(real_filename.c_str());
        std::string name;
        while (! file.eof()) {
            name = "";
            std::getline(file, name); // get the entire line
            if (name.empty())
                continue;
             
            // remove whitespaces from start and end
            FbTk::StringUtil::removeFirstWhitespace(name);

            // the cleaned string could still be a comment, or blank
            if ( name.empty() || name[0] == '#' || name[0] == '!' )
                continue;

            // trailing whitespace won't affect the above test
            FbTk::StringUtil::removeTrailingWhitespace(name);

            SlitClient *client = new SlitClient(name.c_str());
            m_client_list.push_back(client);
        }
    }
}

void Slit::updateClientmenu() {
    if (screen().isShuttingdown()) 
        return;
    _FB_USES_NLS;

    // clear old items
    m_clientlist_menu.removeAll();
    m_clientlist_menu.setLabel(_FBTEXT(Slit, ClientsMenu, "Clients", "Slit client menu"));

    FbTk::RefCount<FbTk::Command> cycle_up(new FbTk::SimpleCommand<Slit>(*this, &Slit::cycleClientsUp));
    FbTk::RefCount<FbTk::Command> cycle_down(new FbTk::SimpleCommand<Slit>(*this, &Slit::cycleClientsDown));
    m_clientlist_menu.insert(_FBTEXT(Slit, CycleUp, "Cycle Up", "Cycle clients upwards"), cycle_up);
    m_clientlist_menu.insert(_FBTEXT(Slit, CycleDown, "Cycle Down", "Cycle clients downwards"), cycle_down);

    FbTk::MenuItem *separator = new FbTk::MenuItem("---");
    separator->setEnabled(false);
    m_clientlist_menu.insert(separator);

    FbTk::RefCount<FbTk::Command> reconfig(new FbTk::SimpleCommand<Slit>(*this, &Slit::reconfigure));
    SlitClients::iterator it = m_client_list.begin();
    for (; it != m_client_list.end(); ++it) {
        if ((*it) != 0 && (*it)->window() != 0)
            m_clientlist_menu.insert(new SlitClientMenuItem(*(*it), reconfig));
    }

    m_clientlist_menu.update();
}

void Slit::saveClientList() {

    std::ofstream file(FbTk::StringUtil::expandFilename(m_filename).c_str());
    SlitClients::iterator it = m_client_list.begin();
    SlitClients::iterator it_end = m_client_list.end();
    std::string prevName;
    std::string name;
    for (; it != it_end; ++it) {
        name = (*it)->matchName();
        if (name != prevName)
            file << name.c_str() << std::endl;

        prevName = name;
    }
}

void Slit::setAutoHide(bool val) {
    *m_rc_auto_hide = val;
    if (doAutoHide()) {
        if (! m_timer.isTiming()) {
            if (m_slitmenu.isVisible())
                m_timer.fireOnce(false);
            m_timer.start();
        }
    }
}

void Slit::setupMenu() {
    _FB_USES_NLS;
    using namespace FbTk;

    FbTk::MacroCommand *s_a_reconf_macro = new FbTk::MacroCommand();
    FbTk::MacroCommand *s_a_reconf_slit_macro = new FbTk::MacroCommand();
    FbTk::RefCount<FbTk::Command> saverc_cmd(new FbCommands::SaveResources());
    FbTk::RefCount<FbTk::Command> reconf_cmd(new FbCommands::ReconfigureFluxboxCmd());
    FbTk::RefCount<FbTk::Command> reconf_slit_cmd(new FbTk::SimpleCommand<Slit>(*this, &Slit::reconfigure));

    s_a_reconf_macro->add(saverc_cmd);
    s_a_reconf_macro->add(reconf_cmd);

    s_a_reconf_slit_macro->add(saverc_cmd);
    s_a_reconf_slit_macro->add(reconf_slit_cmd);

    FbTk::RefCount<FbTk::Command> save_and_reconfigure(s_a_reconf_macro);
    FbTk::RefCount<FbTk::Command> save_and_reconfigure_slit(s_a_reconf_slit_macro);

    // setup base menu
    m_slitmenu.setLabel(_FBTEXT(Slit, Slit, "Slit", "The Slit"));
    m_slitmenu.insert(_FBTEXT(Menu, Placement, "Placement", "Title of Placement menu"),
                      &m_placement_menu);

    m_slitmenu.insert(_FBTEXT(Menu, Layer, "Layer...", "Title of Layer menu"), m_layermenu.get());

#ifdef XINERAMA
    if (screen().hasXinerama()) {
        m_slitmenu.insert(_FBTEXT(Menu, OnHead, "On Head...", "Title of On Head menu"),
                          new XineramaHeadMenu<Slit>(
                              screen().menuTheme(),
                              screen(),
                              screen().imageControl(),
                              *screen().layerManager().getLayer(Fluxbox::instance()->getMenuLayer()),
                              *this,
                              _FBTEXT(Slit, OnHead, "Slit on Head", "Title of Slits On Head menu")
                              ));
    }
                    
#endif //XINERAMA
    m_slitmenu.insert(new BoolMenuItem(_FBTEXT(Common, AutoHide, "Auto hide", "This thing automatically hides when not close by"),
                                       *m_rc_auto_hide,
                                       save_and_reconfigure_slit));

    m_slitmenu.insert(new BoolMenuItem(_FBTEXT(Common, MaximizeOver,"Maximize Over", "Maximize over this thing when maximizing"), 
                                       *m_rc_maximize_over, 
                                       save_and_reconfigure_slit));

    // this saves resources and clears the slit window to update alpha value
    FbTk::MenuItem *alpha_menuitem = 
        new IntResMenuItem(_FBTEXT(Common, Alpha, "Alpha", "Transparency level"),
                           m_rc_alpha,
                           0, 255);
    // setup command for alpha value
    MacroCommand *alpha_macrocmd = new MacroCommand(); 
    RefCount<Command> clear_cmd(new SimpleCommand<Slit>(*this, &Slit::clearWindow));
    alpha_macrocmd->add(saverc_cmd);
    alpha_macrocmd->add(clear_cmd);
    RefCount<Command> set_alpha_cmd(alpha_macrocmd);
    alpha_menuitem->setCommand(set_alpha_cmd);

    m_slitmenu.insert(alpha_menuitem);

    m_slitmenu.insert(new SlitDirMenuItem(_FBTEXT(Slit, Direction, "Slit Direction", "Orientation of slit"), 
                                          *this,
                                          save_and_reconfigure));
    m_slitmenu.insert(_FBTEXT(Slit, ClientsMenu, "Clients", "Slit client menu"), &m_clientlist_menu);
    m_slitmenu.update();

    // setup sub menu
    m_placement_menu.setLabel(_FBTEXT(Slit, Placement, "Slit Placement", "Slit Placement"));
    m_placement_menu.setMinimumSublevels(3);
    m_layermenu->setInternalMenu();
    m_clientlist_menu.setInternalMenu();
   
    typedef list<pair<const char *, Slit::Placement> > Placements;
    Placements place_menu;

    // menu is 3 wide, 5 down
    place_menu.push_back(make_pair(_FBTEXT(Align, TopLeft, "Top Left", "Top Left"), Slit::TOPLEFT));
    place_menu.push_back(make_pair(_FBTEXT(Align, LeftCenter, "Left Center", "Left Center"), Slit::LEFTCENTER));
    place_menu.push_back(make_pair(_FBTEXT(Align, BottomLeft, "Bottom Left", "Bottom Left"), Slit::BOTTOMLEFT));
    place_menu.push_back(make_pair(_FBTEXT(Align, TopCenter, "Top Center", "Top Center"), Slit::TOPCENTER));
    place_menu.push_back(make_pair((const char *)0, Slit::TOPLEFT));
    place_menu.push_back(make_pair(_FBTEXT(Align, BottomCenter, "Bottom Center", "Bottom Center"), Slit::BOTTOMCENTER));
    place_menu.push_back(make_pair(_FBTEXT(Align, TopRight, "Top Right", "Top Right"), Slit::TOPRIGHT));
    place_menu.push_back(make_pair(_FBTEXT(Align, RightCenter, "Right Center", "Right Center"), Slit::RIGHTCENTER));
    place_menu.push_back(make_pair(_FBTEXT(Align, BottomRight, "Bottom Right", "Bottom Right"), Slit::BOTTOMRIGHT));
    

    // create items in sub menu
    for (size_t i=0; i<9; ++i) {
        const char *str = place_menu.front().first;
        Slit::Placement placement = place_menu.front().second;

        if (str == 0) {
            m_placement_menu.insert("");
            m_placement_menu.setItemEnabled(i, false);
        } else {
            m_placement_menu.insert(new PlaceSlitMenuItem(str, *this,
                                                          placement, 
                                                          save_and_reconfigure));
                                                              
        }
        place_menu.pop_front();
    }

    // finaly update sub menu
    m_placement_menu.update();
}

void Slit::moveToLayer(int layernum) {
    m_layeritem->moveToLayer(layernum);
    *m_rc_layernum = layernum;
}

void Slit::saveOnHead(int head) {
    m_rc_on_head = head;
    // reposition
    reconfigure();
}

