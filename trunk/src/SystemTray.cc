// SystemTray.cc
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#include "SystemTray.hh"

#include "FbTk/EventManager.hh"
#include "FbTk/ImageControl.hh"

#include "AtomHandler.hh"
#include "fluxbox.hh"
#include "WinClient.hh"
#include "Screen.hh"

#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include <string>

using std::string;

#ifdef DEBUG
#include <iostream>
using std::cerr;
using std::endl;
using std::hex;
using std::dec;
#endif // DEBUG

/// helper class for tray windows, so we dont call XDestroyWindow
class TrayWindow: public FbTk::FbWindow {
public:
    TrayWindow(Window win):FbTk::FbWindow(win), m_visible(false) {
        setEventMask(PropertyChangeMask);
    }
    bool isVisible() { return m_visible; }
    void show() {
        if (!m_visible) {
            m_visible = true;
            FbTk::FbWindow::show();
        }
    }
    void hide() {
        if (m_visible) {
            m_visible = false;
            FbTk::FbWindow::hide();
        }
    }
private:
    bool m_visible;
};

/// handles clientmessage event and notifies systemtray
class SystemTrayHandler: public AtomHandler {
public:
    SystemTrayHandler(SystemTray &tray):m_tray(tray) {
    }
    // client message is the only thing we care about
    bool checkClientMessage(const XClientMessageEvent &ce,
                            BScreen * screen, WinClient * const winclient) {
        // must be on the same screen
        if ((screen && screen->screenNumber() != m_tray.window().screenNumber()) ||
            (winclient && winclient->screenNumber() != m_tray.window().screenNumber()) )
            return false;
        return m_tray.clientMessage(ce);
    }

    void initForScreen(BScreen &screen) { };
    void setupFrame(FluxboxWindow &win) { };
    void setupClient(WinClient &winclient) {
        // must be on the same screen
        if (winclient.screenNumber() != m_tray.window().screenNumber())
            return;

        // we dont want a managed window
        if (winclient.fbwindow() != 0)
            return;
        // if not kde dockapp...
        if (!winclient.screen().isKdeDockapp(winclient.window()))
            return;
        // if not our screen...
        if (winclient.screenNumber() != m_tray.window().screenNumber())
            return;
        winclient.setEventMask(StructureNotifyMask |
                               SubstructureNotifyMask | EnterWindowMask);
        m_tray.addClient(winclient.window());

    };

    void updateWorkarea(BScreen &) { }
    void updateFocusedWindow(BScreen &, Window) { }
    void updateClientList(BScreen &screen) { };
    void updateWorkspaceNames(BScreen &screen) { };
    void updateCurrentWorkspace(BScreen &screen) { };
    void updateWorkspaceCount(BScreen &screen) { };

    void updateFrameClose(FluxboxWindow &win) { };
    void updateClientClose(WinClient &winclient) { };
    void updateWorkspace(FluxboxWindow &win) { };
    void updateState(FluxboxWindow &win) { };
    void updateHints(FluxboxWindow &win) { };
    void updateLayer(FluxboxWindow &win) { };

    virtual bool propertyNotify(WinClient &winclient, Atom the_property) { return false; }

private:
    SystemTray &m_tray;
};

SystemTray::SystemTray(const FbTk::FbWindow& parent, ButtonTheme& theme, BScreen& screen):
    ToolbarItem(ToolbarItem::FIXED),
    m_window(parent, 0, 0, 1, 1, ExposureMask | ButtonPressMask | ButtonReleaseMask |
             SubstructureNotifyMask | SubstructureRedirectMask),
    m_theme(theme),
    m_screen(screen),
    m_pixmap(0), m_num_visible_clients(0) {

    FbTk::EventManager::instance()->add(*this, m_window);
    m_theme.reconfigSig().attach(this);

    Fluxbox* fluxbox = Fluxbox::instance();
    Display *disp = fluxbox->display();

    // setup atom name to _NET_SYSTEM_TRAY_S<screen number>
    char intbuff[16];
    sprintf(intbuff, "%d", m_window.screenNumber());
    string atom_name("_NET_SYSTEM_TRAY_S");
    atom_name += intbuff; // append number

    // get selection owner and see if it's free
    Atom tray_atom = XInternAtom(disp, atom_name.c_str(), False);
    Window owner = XGetSelectionOwner(disp, tray_atom);
    if (owner != 0) {
#ifdef DEBUG
        cerr<<__FILE__<<"(SystemTray(const FbTk::FbWindow)): can't set owner!"<<endl;
#endif // DEBUG
        return;  // the're can't be more than one owner
    }

    // ok, it was free. Lets set owner
#ifdef DEBUG
    cerr<<__FILE__<<"(SystemTray(const FbTk::FbWindow)): SETTING OWNER!"<<endl;
#endif // DEBUG
    // set owner
    XSetSelectionOwner(disp, tray_atom, m_window.window(), CurrentTime);

    m_handler.reset(new SystemTrayHandler(*this));

    fluxbox->addAtomHandler(m_handler.get(), atom_name);


    // send selection owner msg
    Window root_window = m_screen.rootWindow().window();
    XEvent ce;
    ce.xclient.type = ClientMessage;
    ce.xclient.message_type = XInternAtom(disp, "MANAGER", False);
    ce.xclient.display = disp;
    ce.xclient.window = root_window;
    ce.xclient.format = 32;
    ce.xclient.data.l[0] = CurrentTime; // timestamp
    ce.xclient.data.l[1] = tray_atom; // manager selection atom
    ce.xclient.data.l[2] = m_window.window(); // the window owning the selection
    ce.xclient.data.l[3] = 0l; // selection specific data
    ce.xclient.data.l[4] = 0l; // selection specific data

    XSendEvent(disp, root_window, false, StructureNotifyMask, &ce);

    update(0);
}

SystemTray::~SystemTray() {
    // remove us, else fluxbox might delete the memory too
    Fluxbox::instance()->removeAtomHandler(m_handler.get());
    removeAllClients();

    if (m_pixmap)
        m_screen.imageControl().removeImage(m_pixmap);

    // ~FbWindow cleans EventManager
}

void SystemTray::move(int x, int y) {
    m_window.move(x, y);
}

void SystemTray::resize(unsigned int width, unsigned int height) {
    if (width != m_window.width() ||
        height != m_window.height()) {
        m_window.resize(width, height);
        if (m_num_visible_clients)
            rearrangeClients();
        resizeSig().notify();
    }
}

void SystemTray::moveResize(int x, int y,
                            unsigned int width, unsigned int height) {
    if (width != m_window.width() ||
        height != m_window.height()) {
        m_window.moveResize(x, y, width, height);
        if (m_num_visible_clients)
            rearrangeClients();
        resizeSig().notify();
    } else {
        move(x, y);
    }
}

void SystemTray::hide() {
    m_window.hide();
}

void SystemTray::show() {

    update(0);
    m_window.show();
}

unsigned int SystemTray::width() const {
    if (orientation() == FbTk::ROT90 || orientation() == FbTk::ROT270)
        return m_window.width();

    return m_num_visible_clients * (height() - 2 * m_theme.border().width());
}

unsigned int SystemTray::height() const {
    if (orientation() == FbTk::ROT0 || orientation() == FbTk::ROT180)
        return m_window.height();

    return m_num_visible_clients * (width() - 2 * m_theme.border().width());
}

unsigned int SystemTray::borderWidth() const {
    return m_window.borderWidth();
}

bool SystemTray::clientMessage(const XClientMessageEvent &event) {
    static const int SYSTEM_TRAY_REQUEST_DOCK  =  0;
    //    static const int SYSTEM_TRAY_BEGIN_MESSAGE =  1;
    //    static const int SYSTEM_TRAY_CANCEL_MESSAGE = 2;

    if (event.message_type ==
        XInternAtom(FbTk::App::instance()->display(), "_NET_SYSTEM_TRAY_OPCODE", False)) {

        int type = event.data.l[1];
        if (type == SYSTEM_TRAY_REQUEST_DOCK) {
#ifdef DEBUG
            cerr<<"SystemTray::clientMessage(const XClientMessageEvent): SYSTEM_TRAY_REQUEST_DOCK"<<endl;
            cerr<<"window = event.data.l[2] = "<<event.data.l[2]<<endl;
#endif // DEBUG
            addClient(event.data.l[2]);
        }
        /*
        else if (type == SYSTEM_TRAY_BEGIN_MESSAGE)
            cerr<<"BEGIN MESSAGE"<<endl;
        else if (type == SYSTEM_TRAY_CANCEL_MESSAGE)
            cerr<<"CANCEL MESSAGE"<<endl;
        */

        return true;
    }

    return false;
}

SystemTray::ClientList::iterator SystemTray::findClient(Window win) {

    ClientList::iterator it = m_clients.begin();
    ClientList::iterator it_end = m_clients.end();
    for (; it != it_end; ++it) {
        if ((*it)->window() == win)
            break;
    }

    return it;
}

void SystemTray::addClient(Window win) {
    if (win == 0)
        return;

    ClientList::iterator it = findClient(win);
    if (it != m_clients.end())
        return;

    // make sure we have the same screen number
    XWindowAttributes attr;
    attr.screen = 0;
    if (XGetWindowAttributes(FbTk::App::instance()->display(),
                             win, &attr) != 0 &&
        attr.screen != 0 &&
        XScreenNumberOfScreen(attr.screen) != window().screenNumber()) {
        return;
    }

    TrayWindow *traywin = new TrayWindow(win);

#ifdef DEBUG
    cerr<<"SystemTray::addClient(Window): 0x"<<hex<<win<<dec<<endl;
#endif // DEBUG

    m_clients.push_back(traywin);
    FbTk::EventManager::instance()->add(*this, win);
    FbTk::EventManager::instance()->addParent(*this, window());
    XChangeSaveSet(FbTk::App::instance()->display(), win, SetModeInsert);
    traywin->reparent(m_window, 0, 0);
    showClient(traywin);
}

void SystemTray::removeClient(Window win) {
    ClientList::iterator tray_it = findClient(win);
    if (tray_it == m_clients.end())
        return;

#ifdef DEBUG
    cerr<<__FILE__<<"(SystemTray::removeClient(Window)): 0x"<<hex<<win<<dec<<endl;
#endif // DEBUG
    TrayWindow *traywin = *tray_it;
    m_clients.erase(tray_it);
    hideClient(traywin);
    delete traywin;
}

void SystemTray::exposeEvent(XExposeEvent &event) {
    m_window.clear();
}

void SystemTray::handleEvent(XEvent &event) {
    if (event.type == DestroyNotify) {
        removeClient(event.xdestroywindow.window);
    } else if (event.type == ReparentNotify && event.xreparent.parent != m_window.window()) {
        removeClient(event.xreparent.window);
    } else if (event.type == UnmapNotify && event.xany.send_event) {
        // we ignore server-generated events, which can occur
        // on restart. The ICCCM says that a client must send
        // a synthetic event for the withdrawn state
        ClientList::iterator it = findClient(event.xunmap.window);
        if (it != m_clients.end())
            hideClient(*it);
    } else if (event.type == ConfigureNotify) {
        // we got configurenotify from an client
        // check and see if we need to update it's size
        // and we must reposition and resize them to fit
        // our toolbar
        ClientList::iterator it = findClient(event.xconfigure.window);
        if (it != m_clients.end()) {
            if (static_cast<unsigned int>(event.xconfigure.width) != (*it)->width() ||
                static_cast<unsigned int>(event.xconfigure.height) != (*it)->height()) {
                // the position might differ so we update from our local
                // copy of position
                XMoveResizeWindow(FbTk::App::instance()->display(), (*it)->window(),
                                  (*it)->x(), (*it)->y(),
                                  (*it)->width(), (*it)->height());

                // this was why gaim wasn't centring the icon
                (*it)->sendConfigureNotify(0, 0, (*it)->width(), (*it)->height());
                // so toolbar know that we changed size
                // done inside this loop, because otherwise we can get into nasty looping
                resizeSig().notify();
            }
        }

    } else if (event.type == PropertyNotify) {
        ClientList::iterator it = findClient(event.xproperty.window);
        if (it != m_clients.end()) {
            Atom embed_info = XInternAtom(FbTk::App::instance()->display(),"_XEMBED_INFO",false);
            if (event.xproperty.atom == embed_info) {

/* Flags for _XEMBED_INFO */
#define XEMBED_MAPPED                   (1 << 0)

                TrayWindow *traywin = *it;
                Atom actual_type;
                int actual_format;
                unsigned long nitems, bytes_after;
                unsigned long *prop;
                if (traywin->property(embed_info, 0l, 2l, false, embed_info,
                    &actual_type, &actual_format, &nitems, &bytes_after,
                    (unsigned char **) &prop)) {

                    bool mapped = (bool)(static_cast<unsigned long>(prop[1]) & XEMBED_MAPPED);
                    XFree(static_cast<void *>(prop));

#ifdef DEBUG
    cerr<<__FILE__<<"(SystemTray::handleEvent(XEvent)): XEMBED_MAPPED = "<<mapped<<endl;
#endif // DEBUG

                    if (mapped)
                        showClient(traywin);
                    else
                        hideClient(traywin);
                }
            }
        }
    }
}

void SystemTray::rearrangeClients() {
    unsigned int w_rot0 = width(), h_rot0 = height();
    const unsigned int bw = m_theme.border().width();
    FbTk::translateSize(orientation(), w_rot0, h_rot0);
    unsigned int trayw = m_num_visible_clients*h_rot0 + bw, trayh = h_rot0;
    FbTk::translateSize(orientation(), trayw, trayh);
    resize(trayw, trayh);
    update(0);

    // move and resize clients
    ClientList::iterator client_it = m_clients.begin();
    ClientList::iterator client_it_end = m_clients.end();
    int next_x = bw;
    for (; client_it != client_it_end; ++client_it) {
        if (!(*client_it)->isVisible())
            continue;
        int x = next_x, y = bw;
        next_x += h_rot0+bw;
        translateCoords(orientation(), x, y, w_rot0, h_rot0);
        translatePosition(orientation(), x, y, h_rot0, h_rot0, 0);

        (*client_it)->moveResize(x, y, h_rot0, h_rot0);
        (*client_it)->sendConfigureNotify(x, y, h_rot0, h_rot0);
    }
}

void SystemTray::removeAllClients() {
    BScreen *screen = Fluxbox::instance()->findScreen(window().screenNumber());
    while (!m_clients.empty()) {
        if (screen)
            m_clients.back()->reparent(screen->rootWindow(), 0, 0);
        m_clients.back()->hide();
        delete m_clients.back();
        m_clients.pop_back();
    }
    m_num_visible_clients = 0;
}

void SystemTray::hideClient(TrayWindow *traywin) {
    if (!traywin || !traywin->isVisible())
        return;

    traywin->hide();
    m_num_visible_clients--;
    rearrangeClients();
}

void SystemTray::showClient(TrayWindow *traywin) {
    if (!traywin || traywin->isVisible())
        return;

    if (!m_num_visible_clients)
        show();

    traywin->show();
    m_num_visible_clients++;
    rearrangeClients();
}

void SystemTray::update(FbTk::Subject* subject) {

    if (!m_theme.texture().usePixmap()) {
        m_window.setBackgroundColor(m_theme.texture().color());
    }
    else {
        if(m_pixmap)
            m_screen.imageControl().removeImage(m_pixmap);
        m_pixmap = m_screen.imageControl().renderImage(width(), height(),
                                                       m_theme.texture(), orientation());
        m_window.setBackgroundPixmap(m_pixmap);
    }

    ClientList::iterator client_it = m_clients.begin();
    ClientList::iterator client_it_end = m_clients.end();
    for (; client_it != client_it_end; ++client_it) {

        // maybe not the best solution (yet), force a refresh of the
        // background of the client
        if (!(*client_it)->isVisible())
            continue;
        (*client_it)->hide();
        (*client_it)->show();
    }

}
