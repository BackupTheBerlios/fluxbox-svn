// fluxbox.cc for Fluxbox Window Manager
// Copyright (c) 2001 - 2005 Henrik Kinnunen (fluxgen at users.sourceforge.net)
//
// blackbox.cc for blackbox - an X11 Window manager
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

// $Id$

#include "fluxbox.hh"

#include "Screen.hh"
#include "Window.hh"
#include "Workspace.hh"
#include "AtomHandler.hh"
#include "FbCommands.hh"
#include "WinClient.hh"
#include "Keys.hh"
#include "FbAtoms.hh"
#include "defaults.hh"

#include "FbTk/I18n.hh"
#include "FbTk/Image.hh"
#include "FbTk/FileUtil.hh"
#include "FbTk/KeyUtil.hh"
#include "FbTk/ImageControl.hh"
#include "FbTk/EventManager.hh"
#include "FbTk/StringUtil.hh"
#include "FbTk/Resource.hh"
#include "FbTk/SimpleCommand.hh"
#include "FbTk/XrmDatabaseHelper.hh"
#include "FbTk/Command.hh"
#include "FbTk/RefCount.hh"
#include "FbTk/SimpleCommand.hh"
#include "FbTk/CompareEqual.hh"
#include "FbTk/Transparent.hh"

//Use GNU extensions
#ifndef	 _GNU_SOURCE
#define	 _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef SLIT
#include "Slit.hh"
#endif // SLIT
#ifdef USE_GNOME
#include "Gnome.hh"
#endif // USE_GNOME
#ifdef USE_NEWWMSPEC
#include "Ewmh.hh"
#endif // USE_NEWWMSPEC
#ifdef REMEMBER
#include "Remember.hh"
#endif // REMEMBER
#ifdef USE_TOOLBAR
#include "Toolbar.hh"
#else
class Toolbar { };
#endif // USE_TOOLBAR

// X headers
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>

// X extensions
#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif // SHAPE
#ifdef HAVE_RANDR
#include <X11/extensions/Xrandr.h>
#endif // HAVE_RANDR

// system headers

#ifdef HAVE_CSTDIO
  #include <cstdio>
#else
  #include <stdio.h>
#endif
#ifdef HAVE_CSTDLIB
  #include <cstdlib>
#else
  #include <stdlib.h>
#endif
#ifdef HAVE_CSTRING
  #include <cstring>
#else
  #include <string.h>
#endif

#ifdef HAVE_UNISTD_H
#include <sys/types.h>
#include <unistd.h>
#endif // HAVE_UNISTD_H

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif // HAVE_SYS_PARAM_H

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif // HAVE_SYS_SELECT_H

#ifdef HAVE_SYS_STAT_H
#include <sys/types.h>
#include <sys/stat.h>
#endif // HAVE_SYS_STAT_H

#ifdef  TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else // !TIME_WITH_SYS_TIME
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else // !HAVE_SYS_TIME_H
#include <time.h>
#endif // HAVE_SYS_TIME_H
#endif // TIME_WITH_SYS_TIME

#include <sys/wait.h>

#include <iostream>
#include <string>
#include <memory>
#include <algorithm>
#include <typeinfo>

using namespace std;
using namespace FbTk;

namespace {

Window last_bad_window = None;
 
int handleXErrors(Display *d, XErrorEvent *e) {
    if (e->error_code == BadWindow)
        last_bad_window = e->resourceid;
#ifdef DEBUG
    else {
        // ignore bad window ones, they happen a lot
        // when windows close themselves
        char errtxt[128];

        XGetErrorText(d, e->error_code, errtxt, 128);
        cerr<<"Fluxbox: X Error: "<<errtxt<<"("<<(int)e->error_code<<") opcodes "<<
            (int)e->request_code<<"/"<<(int)e->minor_code<<" resource 0x"<<hex<<(int)e->resourceid<<dec<<endl;
    }
#endif // !DEBUG

    return False;
}

} // end anonymous

//static singleton var
Fluxbox *Fluxbox::s_singleton=0;

//default values for titlebar left and right
//don't forget to change last value in m_rc_titlebar_* if you add more to these
Fluxbox::Titlebar Fluxbox::s_titlebar_left[] = {STICK};
Fluxbox::Titlebar Fluxbox::s_titlebar_right[] = {MINIMIZE, MAXIMIZE, CLOSE};

Fluxbox::Fluxbox(int argc, char **argv, const char *dpy_name, const char *rcfilename)
    : FbTk::App(dpy_name),
      m_fbatoms(new FbAtoms()),
      m_resourcemanager(rcfilename, true),
      // TODO: shouldn't need a separate one for screen
      m_screen_rm(m_resourcemanager),
      m_rc_tabs(m_resourcemanager, true, "session.tabs", "Session.Tabs"),
      m_rc_tabs_padding(m_resourcemanager, 0, "session.tabPadding", "Session.TabPadding"),
      m_rc_focused_tab_min_width(m_resourcemanager, 0, "session.focusTabMinWidth",
              "Session.FocusTabMinWidth"),
      m_rc_ignoreborder(m_resourcemanager, false, "session.ignoreBorder", "Session.IgnoreBorder"),
      m_rc_pseudotrans(m_resourcemanager, false, "session.forcePseudoTransparency", "Session.forcePseudoTransparency"),
      m_rc_colors_per_channel(m_resourcemanager, 4,
                              "session.colorsPerChannel", "Session.ColorsPerChannel"),
      m_rc_numlayers(m_resourcemanager, 13, "session.numLayers", "Session.NumLayers"),
      m_rc_double_click_interval(m_resourcemanager, 250, "session.doubleClickInterval", "Session.DoubleClickInterval"),
      m_rc_update_delay_time(m_resourcemanager, 0, "session.updateDelayTime", "Session.UpdateDelayTime"),
      m_rc_stylefile(m_resourcemanager, DEFAULTSTYLE, "session.styleFile", "Session.StyleFile"),
      m_rc_menufile(m_resourcemanager, DEFAULTMENU, "session.menuFile", "Session.MenuFile"),
      m_rc_keyfile(m_resourcemanager, DEFAULTKEYSFILE, "session.keyFile", "Session.KeyFile"),
      m_rc_slitlistfile(m_resourcemanager, "~/.fluxbox/slitlist", "session.slitlistFile", "Session.SlitlistFile"),
      m_rc_groupfile(m_resourcemanager, "~/.fluxbox/groups", "session.groupFile", "Session.GroupFile"),
      m_rc_appsfile(m_resourcemanager, "~/.fluxbox/apps", "session.appsFile", "Session.AppsFile"),
      m_rc_titlebar_left(m_resourcemanager, 
                         TitlebarList(&s_titlebar_left[0], &s_titlebar_left[1]), 
                         "session.titlebar.left", "Session.Titlebar.Left"),
      m_rc_titlebar_right(m_resourcemanager,
                          TitlebarList(&s_titlebar_right[0], &s_titlebar_right[3]),
                          "session.titlebar.right", "Session.Titlebar.Right"),
      m_rc_tabs_attach_area(m_resourcemanager, ATTACH_AREA_WINDOW, "session.tabsAttachArea", "Session.TabsAttachArea"),
      m_rc_cache_life(m_resourcemanager, 5, "session.cacheLife", "Session.CacheLife"),
      m_rc_cache_max(m_resourcemanager, 200, "session.cacheMax", "Session.CacheMax"),
      m_rc_auto_raise_delay(m_resourcemanager, 250, "session.autoRaiseDelay", "Session.AutoRaiseDelay"),
      m_rc_use_mod1(m_resourcemanager, true, "session.useMod1", "Session.UseMod1"),
      m_focused_window(0), m_masked_window(0),
      m_mousescreen(0),
      m_keyscreen(0),
      m_watching_screen(0), m_watch_keyrelease(0),
      m_last_time(0),
      m_masked(0),
      m_rc_file(rcfilename ? rcfilename : ""),
      m_argv(argv), m_argc(argc),
      m_starting(true),
      m_restarting(false),
      m_shutdown(false),
      m_server_grabs(0),
      m_randr_event_type(0),
      m_RC_PATH("fluxbox"),
      m_RC_INIT_FILE("init") {

    _FB_USES_NLS;
    if (s_singleton != 0)
        throw string(_FBTEXT(Fluxbox, FatalSingleton, "Fatal! There can only one instance of fluxbox class.", "Error displayed on weird error where an instance of the Fluxbox class already exists!"));

    if (display() == 0) {
        throw string(_FBTEXT(Fluxbox, NoDisplay,
                             "Can not connect to X server.\nMake sure you started X before you start Fluxbox.",
                             "Error message when no X display appears to exist"));
    }

    Display *disp = FbTk::App::instance()->display();
    // For KDE dock applets
    // KDE v1.x
    m_kwm1_dockwindow = XInternAtom(disp,
                                    "KWM_DOCKWINDOW", False);
    // KDE v2.x
    m_kwm2_dockwindow = XInternAtom(disp,
                                    "_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR", False);
    // setup X error handler
    XSetErrorHandler((XErrorHandler) handleXErrors);

    //catch system signals
    SignalHandler &sigh = SignalHandler::instance();
    sigh.registerHandler(SIGSEGV, this);
    sigh.registerHandler(SIGFPE, this);
    sigh.registerHandler(SIGTERM, this);
    sigh.registerHandler(SIGINT, this);
    sigh.registerHandler(SIGCHLD, this);
    sigh.registerHandler(SIGHUP, this);
    sigh.registerHandler(SIGUSR1, this);
    sigh.registerHandler(SIGUSR2, this);
    //
    // setup timer
    // This timer is used to we can issue a safe reconfig command.
    // Because when the command is executed we shouldn't do reconfig directly
    // because it could affect ongoing menu stuff so we need to reconfig in
    // the next event "round".
    FbTk::RefCount<FbTk::Command> reconfig_cmd(new FbTk::SimpleCommand<Fluxbox>(*this, &Fluxbox::timed_reconfigure));
    timeval to;
    to.tv_sec = 0;
    to.tv_usec = 1;
    m_reconfig_timer.setTimeout(to);
    m_reconfig_timer.setCommand(reconfig_cmd);
    m_reconfig_timer.fireOnce(true);
    //XSynchronize(disp, True);

    s_singleton = this;
    m_have_shape = false;
    m_shape_eventbase = 0;
#ifdef SHAPE
    int shape_err;
    m_have_shape = XShapeQueryExtension(disp, &m_shape_eventbase, &shape_err);
#endif // SHAPE

#ifdef HAVE_RANDR
    // get randr event type
    int randr_error_base;
    XRRQueryExtension(disp, &m_randr_event_type, &randr_error_base);
#endif // HAVE_RANDR

    load_rc();
    // setup theme manager to have our style file ready to be scanned
    FbTk::ThemeManager::instance().load(getStyleFilename());

    // setup atom handlers before we create any windows
#ifdef REMEMBER
    addAtomHandler(new Remember(), "remember"); // for remembering window attribs
#endif // REMEMBER
#ifdef USE_NEWWMSPEC
    addAtomHandler(new Ewmh(), "ewmh"); // for Extended window manager atom support
#endif // USE_NEWWMSPEC
#ifdef USE_GNOME
    addAtomHandler(new Gnome(), "gnome"); // for gnome 1 atom support
#endif //USE_GNOME

    grab();

    setupConfigFiles();

    if (! XSupportsLocale())
        cerr<<_FBTEXT(Fluxbox, WarningLocale, "Warning: X server does not support locale", "XSupportsLocale returned false")<<endl;

    if (XSetLocaleModifiers("") == 0)
        cerr<<_FBTEXT(Fluxbox, WarningLocaleModifiers, "Warning: cannot set locale modifiers", "XSetLocaleModifiers returned false")<<endl;


#ifdef HAVE_GETPID
    m_fluxbox_pid = XInternAtom(disp, "_BLACKBOX_PID", False);
#endif // HAVE_GETPID

    // Allocate screens
    for (int i = 0; i < ScreenCount(display()); i++) {
        char scrname[128], altscrname[128];
        sprintf(scrname, "session.screen%d", i);
        sprintf(altscrname, "session.Screen%d", i);
        BScreen *screen = new BScreen(m_screen_rm.lock(),
                                      scrname, altscrname,
                                      i, getNumberOfLayers());
        if (! screen->isScreenManaged()) {
            delete screen;
            continue;
        }
        // add to our list
        m_screen_list.push_back(screen);

        // now we can create menus (which needs this screen to be in screen_list)
        screen->initMenus();

#ifdef HAVE_GETPID
        pid_t bpid = getpid();

        screen->rootWindow().changeProperty(getFluxboxPidAtom(), XA_CARDINAL,
                                            sizeof(pid_t) * 8, PropModeReplace,
                                            (unsigned char *) &bpid, 1);
#endif // HAVE_GETPID

#ifdef HAVE_RANDR
        // setup RANDR for this screens root window
        // we need to determine if we should use old randr select input function or not
#ifdef X_RRScreenChangeSelectInput
        // use old set randr event
        XRRScreenChangeSelectInput(disp, screen->rootWindow().window(), True);
#else
        XRRSelectInput(disp, screen->rootWindow().window(),
                       RRScreenChangeNotifyMask);
#endif // X_RRScreenChangeSelectInput

#endif // HAVE_RANDR


#ifdef USE_TOOLBAR
        m_toolbars.push_back(new Toolbar(*screen,
                                         *screen->layerManager().
                                         getLayer(Fluxbox::instance()->getNormalLayer())));
#endif // USE_TOOLBAR

        // must do this after toolbar is created
        screen->initWindows();

        // attach screen signals to this
        screen->currentWorkspaceSig().attach(this);
        screen->workspaceCountSig().attach(this);
        screen->workspaceNamesSig().attach(this);
        screen->workspaceAreaSig().attach(this);
        screen->clientListSig().attach(this);

        // initiate atomhandler for screen specific stuff
        for (AtomHandlerContainerIt it= m_atomhandler.begin();
             it != m_atomhandler.end();
             it++) {
            (*it).first->initForScreen(*screen);
        }

        revertFocus(*screen); // make sure focus style is correct
#ifdef SLIT
        if (screen->slit())
            screen->slit()->show();
#endif // SLIT


    } // end init screens
    XAllowEvents(disp, ReplayPointer, CurrentTime);


    m_keyscreen = m_mousescreen = m_screen_list.front();

    if (m_screen_list.empty()) {
        throw string(_FBTEXT(Fluxbox, ErrorNoScreens,
                             "Couldn't find screens to manage.\nMake sure you don't have another window manager running.", "Error message when no unmanaged screens found - usually means another window manager is running"));
    }

    // setup theme manager to have our style file ready to be scanned
    FbTk::ThemeManager::instance().load(FbTk::StringUtil::expandFilename(getStyleFilename()));

    XSynchronize(disp, False);
    //XSynchronize(disp, True);
    sync(false);

    m_reconfigure_wait = m_reread_menu_wait = false;

    // Create keybindings handler and load keys file
    m_key.reset(new Keys(StringUtil::expandFilename(*m_rc_keyfile).c_str()));

    m_resourcemanager.unlock();
    ungrab();

#ifdef DEBUG
    if (m_resourcemanager.lockDepth() != 0)
        cerr<<"--- resource manager lockdepth = "<<m_resourcemanager.lockDepth()<<endl;
#endif //DEBUG
    m_starting = false;
    //
    // For dumping theme items
    // FbTk::ThemeManager::instance().listItems();
    //
    //    m_resourcemanager.dump();

#ifdef USE_TOOLBAR
    // finally, show toolbar
    Toolbars::iterator toolbar_it = m_toolbars.begin();
    Toolbars::iterator toolbar_it_end = m_toolbars.end();
    for (; toolbar_it != toolbar_it_end; ++toolbar_it)
        (*toolbar_it)->updateVisibleState();
#endif // USE_TOOLBAR

}


Fluxbox::~Fluxbox() {

    // destroy toolbars
    while (!m_toolbars.empty()) {
        delete m_toolbars.back();
        m_toolbars.pop_back();
    }
   
    // destroy screens
    while (!m_screen_list.empty()) {
        delete m_screen_list.back();
        m_screen_list.pop_back();
    }

    // destroy atomhandlers
    for (AtomHandlerContainerIt it= m_atomhandler.begin();
         it != m_atomhandler.end();
         it++) {
        delete (*it).first;
    }
    m_atomhandler.clear();

    clearMenuFilenames();
}

void Fluxbox::eventLoop() {
    Display *disp = display();
    while (!m_shutdown) {
        if (XPending(disp)) {
            XEvent e;
            XNextEvent(disp, &e);

            if (last_bad_window != None && e.xany.window == last_bad_window &&
                e.type != DestroyNotify) { // we must let the actual destroys through
#ifdef DEBUG
                cerr<<"Fluxbox::eventLoop(): removing bad window from event queue"<<endl;
#endif // DEBUG
            } else {
                last_bad_window = None;
                handleEvent(&e);
            }
        } else {
            FbTk::Timer::updateTimers(ConnectionNumber(disp)); //handle all timers
        }
    }
}

bool Fluxbox::validateWindow(Window window) const {
    XEvent event;
    if (XCheckTypedWindowEvent(display(), window, DestroyNotify, &event)) {
        XPutBackEvent(display(), &event);
        return false;
    }

    return true;
}

void Fluxbox::grab() {
    if (! m_server_grabs++)
       XGrabServer(display());
}

void Fluxbox::ungrab() {
    if (! --m_server_grabs)
        XUngrabServer(display());

    if (m_server_grabs < 0)
        m_server_grabs = 0;
}

/**
 setup the configutation files in
 home directory
*/
void Fluxbox::setupConfigFiles() {

    bool create_init = false, create_keys = false, create_menu = false;

    string dirname = getenv("HOME") + string("/.") + string(m_RC_PATH) + "/";
    string init_file, keys_file, menu_file, slitlist_file;
    init_file = dirname + m_RC_INIT_FILE;
    keys_file = dirname + "keys";
    menu_file = dirname + "menu";

    struct stat buf;

    // is file/dir already there?
    if (! stat(dirname.c_str(), &buf)) {

        // check if anything with those name exists, if not create new
        if (stat(init_file.c_str(), &buf))
            create_init = true;
        if (stat(keys_file.c_str(), &buf))
            create_keys = true;
        if (stat(menu_file.c_str(), &buf))
            create_menu = true;

    } else {
#ifdef DEBUG
        cerr <<__FILE__<<"("<<__LINE__<<"): Creating dir: " << dirname.c_str() << endl;
#endif // DEBUG
        _FB_USES_NLS;
        // create directory with perm 700
        if (mkdir(dirname.c_str(), 0700)) {
            fprintf(stderr, _FBTEXT(Fluxbox, ErrorCreatingDirectory,
                                    "Can't create %s directory", "Can't create a directory, one %s for directory name"), dirname.c_str());
            cerr<<endl;
            return;
        }

        //mark creation of files
        create_init = create_keys = create_menu = true;
    }


    // copy key configuration
    if (create_keys)
        FbTk::FileUtil::copyFile(DEFAULTKEYSFILE, keys_file.c_str());

    // copy menu configuration
    if (create_menu)
        FbTk::FileUtil::copyFile(DEFAULTMENU, menu_file.c_str());

    // copy init file
    if (create_init)
        FbTk::FileUtil::copyFile(DEFAULT_INITFILE, init_file.c_str());

}

void Fluxbox::handleEvent(XEvent * const e) {
    _FB_USES_NLS;
    m_last_event = *e;

    // it is possible (e.g. during moving) for a window
    // to mask all events to go to it
    if ((m_masked == e->xany.window) && m_masked_window) {
        if (e->type == MotionNotify) {
            m_last_time = e->xmotion.time;
            m_masked_window->motionNotifyEvent(e->xmotion);
            return;
        } else if (e->type == ButtonRelease) {
            e->xbutton.window = m_masked_window->fbWindow().window();
        }

    }

    // update key/mouse screen and last time before we enter other eventhandlers
    if (e->type == KeyPress ||
        e->type == KeyRelease) {
        m_keyscreen = searchScreen(e->xkey.root);
    } else if (e->type == ButtonPress ||
               e->type == ButtonRelease ||
               e->type == MotionNotify ) {
        if (e->type == MotionNotify)
            m_last_time = e->xmotion.time;
        else
            m_last_time = e->xbutton.time;

        m_mousescreen = searchScreen(e->xbutton.root);
    } else if (e->type == EnterNotify ||
               e->type == LeaveNotify) {
        m_last_time = e->xcrossing.time;
        m_mousescreen = searchScreen(e->xcrossing.root);
    } else if (e->type == PropertyNotify)
        m_last_time = e->xproperty.time;

    // we need to check focus out for menus before
    // we call FbTk eventhandler
    // so we can get FbTk::Menu::focused() before it sets to 0

    if (e->type == FocusOut &&
        e->xfocus.mode != NotifyGrab &&
        e->xfocus.detail != NotifyPointer &&
        e->xfocus.detail != NotifyInferior &&
        FbTk::Menu::focused() != 0 &&
        FbTk::Menu::focused()->window() == e->xfocus.window) {

        // find screen num
        BScreen *screen = 0;
        ScreenList::iterator it = m_screen_list.begin();
        ScreenList::iterator it_end = m_screen_list.end();
        for (; it != it_end; ++it) {
            if ( (*it)->screenNumber() ==
                 FbTk::Menu::focused()->fbwindow().screenNumber()) {
                screen = (*it);
                break; // found the screen, no more search
            }
        }

        if (screen != 0)
            revertFocus(*screen);
    }

    // try FbTk::EventHandler first
    FbTk::EventManager::instance()->handleEvent(*e);

    switch (e->type) {
    case ButtonRelease:
    case ButtonPress:
        handleButtonEvent(e->xbutton);
        break;
    case ConfigureRequest: {

        if (!searchWindow(e->xconfigurerequest.window)) {

            grab();

            if (validateWindow(e->xconfigurerequest.window)) {
                XWindowChanges xwc;

                xwc.x = e->xconfigurerequest.x;
                xwc.y = e->xconfigurerequest.y;
                xwc.width = e->xconfigurerequest.width;
                xwc.height = e->xconfigurerequest.height;
                xwc.border_width = e->xconfigurerequest.border_width;
                xwc.sibling = e->xconfigurerequest.above;
                xwc.stack_mode = e->xconfigurerequest.detail;

                XConfigureWindow(FbTk::App::instance()->display(),
                                 e->xconfigurerequest.window,
                                 e->xconfigurerequest.value_mask, &xwc);
            }

            ungrab();
        } // else already handled in FluxboxWindow::handleEvent

    }
        break;
    case MapRequest: {

#ifdef DEBUG
        cerr<<"MapRequest for 0x"<<hex<<e->xmaprequest.window<<dec<<endl;

#endif // DEBUG

        WinClient *winclient = searchWindow(e->xmaprequest.window);
        FluxboxWindow *win = 0;

        if (! winclient) {
            BScreen *screen = 0;
            int screen_num;
            XWindowAttributes attr;
            // find screen
            if (XGetWindowAttributes(display(),
                                     e->xmaprequest.window,
                                     &attr) && attr.screen != 0) {
                screen_num = XScreenNumberOfScreen(attr.screen);

                // find screen
                ScreenList::iterator screen_it = find_if(m_screen_list.begin(),
                                                         m_screen_list.end(),
                                                         FbTk::CompareEqual<BScreen>(&BScreen::screenNumber, screen_num));
                if (screen_it != m_screen_list.end())
                    screen = *screen_it;
            }
            // try with parent if we failed to find screen num
            if (screen == 0)
               screen = searchScreen(e->xmaprequest.parent);

            if (screen == 0) {
                cerr<<"Fluxbox "<<_FBTEXT(Fluxbox, CantMapWindow, "Warning! Could not find screen to map window on!", "")<<endl;
            } else
                win = screen->createWindow(e->xmaprequest.window);

        } else {
            win = winclient->fbwindow();
        }

        // we don't handle MapRequest in FluxboxWindow::handleEvent
        if (win)
            win->mapRequestEvent(e->xmaprequest);
    }
        break;
    case MapNotify:
        // handled directly in FluxboxWindow::handleEvent
        break;
    case UnmapNotify:
        handleUnmapNotify(e->xunmap);
	break;
    case MappingNotify:
        // Update stored modifier mapping
#ifdef DEBUG
        cerr<<__FILE__<<"("<<__FUNCTION__<<"): MappingNotify"<<endl;
#endif // DEBUG

        FbTk::KeyUtil::instance().init(); // reinitialise the key utils
        // reconfigure keys (if the mapping changes, they don't otherwise update
        m_key->reconfigure(StringUtil::expandFilename(*m_rc_keyfile).c_str());

        break;
    case CreateNotify:
	break;
    case DestroyNotify: {
        WinClient *winclient = searchWindow(e->xdestroywindow.window);
        if (winclient != 0) {
            FluxboxWindow *win = winclient->fbwindow();
            if (win)
                win->destroyNotifyEvent(e->xdestroywindow);

            delete winclient;

            if (win && win->numClients() == 0)
                delete win;
        }

    }
        break;
    case MotionNotify:
        m_last_time = e->xmotion.time;
        break;
    case PropertyNotify: {
        m_last_time = e->xproperty.time;
        WinClient *winclient = searchWindow(e->xproperty.window);
        if (winclient == 0)
            break;
        // most of them are handled in FluxboxWindow::handleEvent
        // but some special cases like ewmh propertys needs to be checked
        for (AtomHandlerContainerIt it= m_atomhandler.begin();
             it != m_atomhandler.end(); it++) {
            if ( (*it).first->propertyNotify(*winclient, e->xproperty.atom))
                break;
        }
    } break;
    case EnterNotify: {

        m_last_time = e->xcrossing.time;
        BScreen *screen = 0;

        if (e->xcrossing.mode == NotifyGrab)
            break;

        if ((e->xcrossing.window == e->xcrossing.root) &&
            (screen = searchScreen(e->xcrossing.window))) {
            screen->imageControl().installRootColormap();

        }

    } break;
    case LeaveNotify:

        m_last_time = e->xcrossing.time;
        break;
    case Expose:
        break;
    case KeyRelease:
    case KeyPress:
        handleKeyEvent(e->xkey);
	break;
    case ColormapNotify: {
        BScreen *screen = searchScreen(e->xcolormap.window);

        if (screen != 0) {
            screen->setRootColormapInstalled((e->xcolormap.state ==
                                              ColormapInstalled) ? true : false);
        }
    } break;
    case FocusIn: {

        // a grab is something of a pseudo-focus event, so we ignore
        // them, here we ignore some window receiving it
        if (e->xfocus.mode == NotifyGrab ||
            e->xfocus.detail == NotifyPointer ||
            e->xfocus.detail == NotifyInferior)
            break;
        WinClient *winclient = searchWindow(e->xfocus.window);
        if (winclient && m_focused_window != winclient)
            setFocusedWindow(winclient);

    } break;
    case FocusOut:{
        // and here we ignore some window losing the special grab focus
        if (e->xfocus.mode == NotifyGrab ||
            e->xfocus.detail == NotifyPointer ||
            e->xfocus.detail == NotifyInferior)
            break;

        WinClient *winclient = searchWindow(e->xfocus.window);
        if (winclient == 0 && FbTk::Menu::focused() == 0) {
#ifdef DEBUG
            cerr<<__FILE__<<"("<<__FUNCTION__<<") Focus out is not a FluxboxWindow !!"<<endl;
#endif // DEBUG
        } else if (winclient && winclient == m_focused_window &&
                   (winclient->fbwindow() == 0
                    || !winclient->fbwindow()->isMoving()))
            // we don't unfocus a moving window
            setFocusedWindow(0);
    }
	break;
    case ClientMessage:
        handleClientMessage(e->xclient);
	break;
    default: {

#ifdef HAVE_RANDR
        if (e->type == m_randr_event_type) {
            // update root window size in screen
            BScreen *scr = searchScreen(e->xany.window);
            if (scr != 0)
                scr->updateSize();
        }
#endif // HAVE_RANDR

    }

    }
}

void Fluxbox::handleButtonEvent(XButtonEvent &be) {

    switch (be.type) {
    case ButtonPress: {
        m_last_time = be.time;

        BScreen *screen = searchScreen(be.window);
        if (screen == 0)
            break; // end case

        screen->hideMenus();

        // strip num/caps/scroll-lock and
        // see if we're using any other modifier,
        // if we're we shouldn't show the root menu
        // this could happen if we're resizing aterm for instance
        if (FbTk::KeyUtil::instance().cleanMods(be.state) != 0)
            return;

        if (be.button == 1) {
            if (! screen->isRootColormapInstalled())
                screen->imageControl().installRootColormap();
            // hide menus
            if (screen->getRootmenu().isVisible())
                screen->getRootmenu().hide();
            if (screen->getWorkspacemenu().isVisible())
                screen->getWorkspacemenu().hide();

        } else if (be.button == 2) {
            FbCommands::ShowWorkspaceMenuCmd cmd;
            cmd.execute();
        } else if (be.button == 3) {
            FbCommands::ShowRootMenuCmd cmd;
            cmd.execute();
        } else if (screen->isDesktopWheeling() && be.button == 4) {
            screen->nextWorkspace(1);
        } else if (screen->isDesktopWheeling() && be.button == 5) {
            screen->prevWorkspace(1);
        }

    } break;
    case ButtonRelease:
        m_last_time = be.time;
        break;
    default:
        break;
    }
}

void Fluxbox::handleUnmapNotify(XUnmapEvent &ue) {

    BScreen *screen = searchScreen(ue.event);
    
    if (ue.event != ue.window && (!screen || !ue.send_event)) {
        return;
    }

    WinClient *winclient = 0;

    if ((winclient = searchWindow(ue.window)) != 0) {

        if (winclient != 0) {
            FluxboxWindow *win = winclient->fbwindow();

            if (!win) {
                delete winclient;
                return;
            }

            // this should delete client and adjust m_focused_window if necessary
            win->unmapNotifyEvent(ue);

            winclient = 0; // it's invalid now when win destroyed the client

            // finally destroy window if empty
            if (win->numClients() == 0) {
                delete win;
                win = 0;
            }
        }

    // according to http://tronche.com/gui/x/icccm/sec-4.html#s-4.1.4
    // a XWithdrawWindow is 
    //   1) unmapping the window (which leads to the upper branch
    //   2) sends an synthetic unampevent (which is handled below)
    } else if (screen && ue.send_event) {
        XDeleteProperty(display(), ue.window, FbAtoms::instance()->getWMStateAtom());
        XUngrabButton(display(), AnyButton, AnyModifier, ue.window);
    }

}

/**
 * Handles XClientMessageEvent
 */
void Fluxbox::handleClientMessage(XClientMessageEvent &ce) {

#ifdef DEBUG
    char * atom = 0;
    if (ce.message_type)
        atom = XGetAtomName(FbTk::App::instance()->display(), ce.message_type);

    cerr<<__FILE__<<"("<<__LINE__<<"): ClientMessage. data.l[0]=0x"<<hex<<ce.data.l[0]<<
	"  message_type=0x"<<ce.message_type<<dec<<" = \""<<atom<<"\""<<endl;

    if (ce.message_type && atom) XFree((char *) atom);
#endif // DEBUG


    if (ce.format != 32)
        return;

    if (ce.message_type == m_fbatoms->getWMChangeStateAtom()) {
        WinClient *winclient = searchWindow(ce.window);
        if (! winclient || !winclient->fbwindow() || ! winclient->validateClient())
            return;

        if (ce.data.l[0] == IconicState)
            winclient->fbwindow()->iconify();
        if (ce.data.l[0] == NormalState)
            winclient->fbwindow()->deiconify();
    } else if (ce.message_type == m_fbatoms->getFluxboxChangeWorkspaceAtom()) {
        BScreen *screen = searchScreen(ce.window);

        if (screen && ce.data.l[0] >= 0 &&
            ce.data.l[0] < (signed)screen->getCount())
            screen->changeWorkspaceID(ce.data.l[0]);

    } else if (ce.message_type == m_fbatoms->getFluxboxChangeWindowFocusAtom()) {
        WinClient *winclient = searchWindow(ce.window);
        if (winclient) {
            FluxboxWindow *win = winclient->fbwindow();
            if (win && win->isVisible())
                win->setCurrentClient(*winclient, true);
        }
    } else if (ce.message_type == m_fbatoms->getFluxboxCycleWindowFocusAtom()) {
        BScreen *screen = searchScreen(ce.window);

        if (screen) {
            if (! ce.data.l[0])
                screen->prevFocus();
            else
                screen->nextFocus();
        }
    } else if (ce.message_type == m_fbatoms->getFluxboxChangeAttributesAtom()) {
        WinClient *winclient = searchWindow(ce.window);
        FluxboxWindow *win = 0;
        if (winclient && (win = winclient->fbwindow()) && winclient->validateClient()) {
            FluxboxWindow::BlackboxHints net;
            net.flags = ce.data.l[0];
            net.attrib = ce.data.l[1];
            net.workspace = ce.data.l[2];
            net.stack = ce.data.l[3];
            net.decoration = static_cast<int>(ce.data.l[4]);
            win->changeBlackboxHints(net);
        }
    } else {
        WinClient *winclient = searchWindow(ce.window);
        BScreen *screen = searchScreen(ce.window);
        // note: we dont need screen nor winclient to be non-null,
        // it's up to the atomhandler to check that
        for (AtomHandlerContainerIt it= m_atomhandler.begin();
             it != m_atomhandler.end(); it++) {
            (*it).first->checkClientMessage(ce, screen, winclient);
        }

    }
}

/**
 Handles KeyRelease and KeyPress events
*/
void Fluxbox::handleKeyEvent(XKeyEvent &ke) {

    if (keyScreen() == 0 || mouseScreen() == 0)
        return;


    switch (ke.type) {
    case KeyPress:
        m_key->doAction(ke);
        break;
    case KeyRelease: {
        // we ignore most key releases unless we need to use
        // a release to stop something (e.g. window cycling).

        // we notify if _all_ of the watched modifiers are released
        if (m_watching_screen && m_watch_keyrelease) {
            // mask the mod of the released key out
            // won't mask anything if it isn't a mod
            ke.state &= ~FbTk::KeyUtil::instance().keycodeToModmask(ke.keycode);

            if ((m_watch_keyrelease & ke.state) == 0) {

                m_watching_screen->notifyReleasedKeys(ke);
                XUngrabKeyboard(FbTk::App::instance()->display(), CurrentTime);

                // once they are released, we drop the watch
                m_watching_screen = 0;
                m_watch_keyrelease = 0;
            }
        }

        break;
    }
    default:
        break;
    }


}

/// handle system signals
void Fluxbox::handleSignal(int signum) {
    _FB_USES_NLS;

    static int re_enter = 0;

    switch (signum) {
    case SIGCHLD: // we don't want the child process to kill us
        waitpid(-1, 0, WNOHANG | WUNTRACED);
        break;
    case SIGHUP:
        load_rc();
        break;
    case SIGUSR1:
        reload_rc();
        break;
    case SIGUSR2:
        rereadMenu();
        break;
    case SIGSEGV:
        abort();
        break;
    case SIGFPE:
    case SIGINT:
    case SIGTERM:
        shutdown();
        break;
    default:
        fprintf(stderr,
                _FBTEXT(BaseDisplay, SignalCaught, "%s:      signal %d caught\n", "signal catch debug message. Include %s for command and %d for signal number"),
                m_argv[0], signum);

        if (! m_starting && ! re_enter) {
            re_enter = 1;
            fprintf(stderr,
                    _FBTEXT(BaseDisplay, ShuttingDown, "Shutting Down\n", "Quitting because of signal, end with newline"));
            shutdown();
        }


        fprintf(stderr,
                _FBTEXT(BaseDisplay, Aborting, "Aborting... dumping core\n", "Aboring and dumping core, end with newline"));
        abort();
        break;
    }
}


void Fluxbox::update(FbTk::Subject *changedsub) {
    //TODO: fix signaling, this does not look good
    if (typeid(*changedsub) == typeid(FluxboxWindow::WinSubject)) {
        FluxboxWindow::WinSubject *winsub = dynamic_cast<FluxboxWindow::WinSubject *>(changedsub);
        FluxboxWindow &win = winsub->win();
        if ((&(win.hintSig())) == changedsub) { // hint signal
            for (AtomHandlerContainerIt it= m_atomhandler.begin();
                 it != m_atomhandler.end(); ++it) {
                if ( (*it).first->update())
                    (*it).first->updateHints(win);
            }
        } else if ((&(win.stateSig())) == changedsub) { // state signal
            for (AtomHandlerContainerIt it= m_atomhandler.begin();
                 it != m_atomhandler.end(); ++it) {
                if ((*it).first->update())
                    (*it).first->updateState(win);
            }
            // if window changed to iconic state
            // add to icon list
            if (win.isIconic()) {
                Workspace *space = win.screen().getWorkspace(win.workspaceNumber());
                if (space != 0)
                    space->removeWindow(&win, true);
                win.screen().addIcon(&win);
            }

            if (win.isStuck()) {
                // if we're sticky then reassociate window
                // to all workspaces
                BScreen &scr = win.screen();
                if (scr.currentWorkspaceID() != win.workspaceNumber()) {
                    scr.reassociateWindow(&win,
                                          scr.currentWorkspaceID(),
                                          true);
                }
            }
        } else if ((&(win.layerSig())) == changedsub) { // layer signal

            for (AtomHandlerContainerIt it= m_atomhandler.begin();
                 it != m_atomhandler.end(); ++it) {
                if ((*it).first->update())
                    (*it).first->updateLayer(win);
            }
        } else if ((&(win.dieSig())) == changedsub) { // window death signal
            
            for (AtomHandlerContainerIt it= m_atomhandler.begin();
                it != m_atomhandler.end(); ++it) {
                if ((*it).first->update())
                    (*it).first->updateFrameClose(win);
            }

            // make sure each workspace get this
            BScreen &scr = win.screen();
            scr.removeWindow(&win);
            if (m_focused_window == &win.winClient())
                m_focused_window = 0;

        } else if ((&(win.workspaceSig())) == changedsub) {  // workspace signal
            for (AtomHandlerContainerIt it= m_atomhandler.begin();
                 it != m_atomhandler.end(); ++it) {
                if ((*it).first->update())
                    (*it).first->updateWorkspace(win);
            }
        } else {
#ifdef DEBUG
            cerr<<__FILE__<<"("<<__LINE__<<"): WINDOW uncought signal from "<<&win<<endl;
#endif // DEBUG
        }

    } else if (typeid(*changedsub) == typeid(BScreen::ScreenSubject)) {
        BScreen::ScreenSubject *subj = dynamic_cast<BScreen::ScreenSubject *>(changedsub);
        BScreen &screen = subj->screen();
        if ((&(screen.workspaceCountSig())) == changedsub) {
            for (AtomHandlerContainerIt it= m_atomhandler.begin();
                 it != m_atomhandler.end(); ++it) {
                if ((*it).first->update())
                    (*it).first->updateWorkspaceCount(screen);
            }
        } else if ((&(screen.workspaceNamesSig())) == changedsub) {
            for (AtomHandlerContainerIt it= m_atomhandler.begin();
                 it != m_atomhandler.end(); ++it) {
                if ((*it).first->update())
                    (*it).first->updateWorkspaceNames(screen);
            }
        } else if ((&(screen.currentWorkspaceSig())) == changedsub) {
            for (AtomHandlerContainerIt it= m_atomhandler.begin();
                 it != m_atomhandler.end(); ++it) {
                if ((*it).first->update())
                    (*it).first->updateCurrentWorkspace(screen);
            }
        } else if ((&(screen.workspaceAreaSig())) == changedsub) {
            for (AtomHandlerContainerIt it= m_atomhandler.begin();
                 it != m_atomhandler.end(); ++it) {
                if ((*it).first->update())
                    (*it).first->updateWorkarea(screen);
            }
        } else if ((&(screen.clientListSig())) == changedsub) {
            for (AtomHandlerContainerIt it= m_atomhandler.begin();
                 it != m_atomhandler.end(); ++it) {
                if ((*it).first->update())
                    (*it).first->updateClientList(screen);
            }
        }
    } else if (typeid(*changedsub) == typeid(WinClient::WinClientSubj)) {

        WinClient::WinClientSubj *subj = dynamic_cast<WinClient::WinClientSubj *>(changedsub);
        WinClient &client = subj->winClient();

        // TODO: don't assume it is diesig (need to fix as soon as another signal appears)
        for (AtomHandlerContainerIt it= m_atomhandler.begin();
             it != m_atomhandler.end(); ++it) {
            if ((*it).first->update())
                (*it).first->updateClientClose(client);
        }

        BScreen &screen = client.screen();

        screen.removeClient(client);
        // finaly send notify signal
        screen.updateNetizenWindowDel(client.window());

        // At this point, we trust that this client is no longer in the
        // client list of its frame (but it still has reference to the frame)
        // We also assume that any remaining active one is the last focused one

        // This is where we revert focus on window close
        // NOWHERE ELSE!!!
        if (m_focused_window == &client)
            unfocusWindow(client);

        // failed to revert focus?
        if (m_focused_window == &client)
            m_focused_window = 0;
    }
}

void Fluxbox::attachSignals(FluxboxWindow &win) {
    win.hintSig().attach(this);
    win.stateSig().attach(this);
    win.workspaceSig().attach(this);
    win.layerSig().attach(this);
    win.dieSig().attach(this);
    for (AtomHandlerContainerIt it= m_atomhandler.begin();
         it != m_atomhandler.end(); ++it) {
        (*it).first->setupFrame(win);
    }
}

void Fluxbox::attachSignals(WinClient &winclient) {
    winclient.dieSig().attach(this);

    for (AtomHandlerContainerIt it= m_atomhandler.begin();
         it != m_atomhandler.end(); ++it) {
        (*it).first->setupClient(winclient);
    }
}

BScreen *Fluxbox::searchScreen(Window window) {

    ScreenList::iterator it = m_screen_list.begin();
    ScreenList::iterator it_end = m_screen_list.end();
    for (; it != it_end; ++it) {
        if (*it && (*it)->rootWindow() == window)
            return *it;
    }

    return 0;
}


AtomHandler* Fluxbox::getAtomHandler(const std::string &name) {
    if ( name != "" ) {
        for (AtomHandlerContainerIt it= m_atomhandler.begin();
             it != m_atomhandler.end(); it++ ) {
            if ( name == (*it).second )
                return (*it).first;
        }
    }
    return 0;
}
void Fluxbox::addAtomHandler(AtomHandler *atomh, const std::string &name) {
    m_atomhandler[atomh]= name;;
}

void Fluxbox::removeAtomHandler(AtomHandler *atomh) {

    for (AtomHandlerContainerIt it= m_atomhandler.begin();
         it != m_atomhandler.end();
         ++it) {
        if ((*it).first == atomh) {
            m_atomhandler.erase(it);
            return;
        }
    }
}

WinClient *Fluxbox::searchWindow(Window window) {
    std::map<Window, WinClient *>::iterator it = m_window_search.find(window);
    if (it != m_window_search.end())
        return it->second;

    std::map<Window, FluxboxWindow *>::iterator git = m_window_search_group.find(window);
    return git == m_window_search_group.end() ? 0 : &git->second->winClient();
}


/* Not implemented until we know how it'll be used
 * Recall that this refers to ICCCM groups, not fluxbox tabgroups
 * See ICCCM 4.1.11 for details
 */
/*
WinClient *Fluxbox::searchGroup(Window window) {
}
*/

void Fluxbox::saveWindowSearch(Window window, WinClient *data) {
    m_window_search[window] = data;
}

/* some windows relate to the whole group */
void Fluxbox::saveWindowSearchGroup(Window window, FluxboxWindow *data) {
    m_window_search_group[window] = data;
}

void Fluxbox::saveGroupSearch(Window window, WinClient *data) {
    m_group_search.insert(pair<Window, WinClient *>(window, data));
}


void Fluxbox::removeWindowSearch(Window window) {
    m_window_search.erase(window);
}

void Fluxbox::removeWindowSearchGroup(Window window) {
    m_window_search_group.erase(window);
}

void Fluxbox::removeGroupSearch(Window window) {
    m_group_search.erase(window);
}

/// restarts fluxbox
void Fluxbox::restart(const char *prog) {
    shutdown();

    m_restarting = true;

    if (prog) {
        m_restart_argument = prog;
    }
}

/// prepares fluxbox for a shutdown
void Fluxbox::shutdown() {
    if (m_shutdown)
        return;

    m_shutdown = true;

    XSetInputFocus(FbTk::App::instance()->display(), PointerRoot, None, CurrentTime);

    //send shutdown to all screens
    for_each(m_screen_list.begin(),
             m_screen_list.end(), mem_fun(&BScreen::shutdown));

    sync(false);
}

/// saves resources
void Fluxbox::save_rc() {
    _FB_USES_NLS;
    XrmDatabase new_blackboxrc = 0;

    char rc_string[1024];

    string dbfile(getRcFilename());

    if (!dbfile.empty()) {
        m_resourcemanager.save(dbfile.c_str(), dbfile.c_str());
        m_screen_rm.save(dbfile.c_str(), dbfile.c_str());
    } else
        cerr<<_FBTEXT(Fluxbox, BadRCFile, "rc filename is invalid!", "Bad settings file")<<endl;

    ScreenList::iterator it = m_screen_list.begin();
    ScreenList::iterator it_end = m_screen_list.end();

    //Save screen resources

    for (; it != it_end; ++it) {
        BScreen *screen = *it;
        int screen_number = screen->screenNumber();

        // these are static, but may not be saved in the users resource file,
        // writing these resources will allow the user to edit them at a later
        // time... but loading the defaults before saving allows us to rewrite the
        // users changes...

        // write out the users workspace names
        sprintf(rc_string, "session.screen%d.workspaceNames: ", screen_number);
        string workspaces_string(rc_string);

        for (unsigned int workspace=0; workspace < screen->getCount(); workspace++) {
            if (screen->getWorkspace(workspace)->name().size()!=0)
                workspaces_string.append(screen->getWorkspace(workspace)->name());
            else
                workspaces_string.append("Null");
            workspaces_string.append(",");
        }

        XrmPutLineResource(&new_blackboxrc, workspaces_string.c_str());

    }

    XrmDatabase old_blackboxrc = XrmGetFileDatabase(dbfile.c_str());

    XrmMergeDatabases(new_blackboxrc, &old_blackboxrc); //merge database together
    XrmPutFileDatabase(old_blackboxrc, dbfile.c_str());
    XrmDestroyDatabase(old_blackboxrc);
#ifdef DEBUG
    cerr<<__FILE__<<"("<<__LINE__<<"): ------------ SAVING DONE"<<endl;
#endif // DEBUG
}

/// @return filename of resource file
string Fluxbox::getRcFilename() {

    if (m_rc_file.empty()) { // set default filename
        string defaultfile(getenv("HOME") + string("/.") + m_RC_PATH + string("/") + m_RC_INIT_FILE);
        return defaultfile;
    }

    return m_rc_file;
}

/// Provides default filename of data file
void Fluxbox::getDefaultDataFilename(char *name, string &filename) {
    filename = string(getenv("HOME") + string("/.") + m_RC_PATH + string("/") + name);
}

/// loads resources
void Fluxbox::load_rc() {
    _FB_USES_NLS;
    //get resource filename
    string dbfile(getRcFilename());

    if (!dbfile.empty()) {
        if (!m_resourcemanager.load(dbfile.c_str())) {
            cerr<<_FBTEXT(Fluxbox, CantLoadRCFile, "Failed to load database", "Failed trying to read rc file")<<":"<<dbfile<<endl;
            cerr<<_FBTEXT(Fluxbox, CantLoadRCFileTrying, "Retrying with", "Retrying rc file loading with (the following file)")<<": "<<DEFAULT_INITFILE<<endl;
            if (!m_resourcemanager.load(DEFAULT_INITFILE))
                cerr<<_FBTEXT(Fluxbox, CantLoadRCFile, "Failed to load database", "")<<": "<<DEFAULT_INITFILE<<endl;
        }
    } else {
        if (!m_resourcemanager.load(DEFAULT_INITFILE))
            cerr<<_FBTEXT(Fluxbox, CantLoadRCFile, "Failed to load database", "")<<": "<<DEFAULT_INITFILE<<endl;
    }

    if (m_rc_menufile->empty())
        m_rc_menufile.setDefaultValue();

    if (FbTk::Transparent::haveComposite())
        FbTk::Transparent::usePseudoTransparent(*m_rc_pseudotrans);

    if (!m_rc_slitlistfile->empty()) {
        *m_rc_slitlistfile = StringUtil::expandFilename(*m_rc_slitlistfile);
    } else {
        string filename;
        getDefaultDataFilename("slitlist", filename);
        m_rc_slitlistfile.setFromString(filename.c_str());
    }

    if (*m_rc_colors_per_channel < 2)
        *m_rc_colors_per_channel = 2;
    else if (*m_rc_colors_per_channel > 6)
        *m_rc_colors_per_channel = 6;

    if (m_rc_stylefile->empty())
        *m_rc_stylefile = DEFAULTSTYLE;

    if (!Workspace::loadGroups(*m_rc_groupfile)) {
#ifdef DEBUG
        cerr<<_FBTEXT(Fluxbox, CantLoadGroupFile, "Failed to load groupfile", "Couldn't load the groupfile")<<": "<<*m_rc_groupfile<<endl;
#endif // DEBUG
    }
}

void Fluxbox::load_rc(BScreen &screen) {
    //get resource filename
    _FB_USES_NLS;
    string dbfile(getRcFilename());

    XrmDatabaseHelper database;

    database = XrmGetFileDatabase(dbfile.c_str());
    if (database==0)
        database = XrmGetFileDatabase(DEFAULT_INITFILE);

    XrmValue value;
    char *value_type, name_lookup[1024], class_lookup[1024];
    int screen_number = screen.screenNumber();


    screen.removeWorkspaceNames();

    sprintf(name_lookup, "session.screen%d.workspaceNames", screen_number);
    sprintf(class_lookup, "Session.Screen%d.WorkspaceNames", screen_number);
    if (XrmGetResource(*database, name_lookup, class_lookup, &value_type,
                       &value)) {
#ifdef DEBUG
        cerr<<__FILE__<<"("<<__FUNCTION__<<"): Workspaces="<<
            screen.getNumberOfWorkspaces()<<endl;
#endif // DEBUG
        char *search = StringUtil::strdup(value.addr);

        int i;
        for (i = 0; i < screen.getNumberOfWorkspaces(); i++) {
            char *nn;

            if (! i) nn = strtok(search, ",");
            else nn = strtok(0, ",");

            if (nn)
                screen.addWorkspaceName(nn);
            else break;

        }

        delete [] search;
    }

    FbTk::Image::removeAllSearchPaths();
    sprintf(name_lookup, "session.screen%d.imageSearchPath", screen_number);
    sprintf(class_lookup, "Session.Screen%d.imageSearchPath", screen_number);
    if (XrmGetResource(*database, name_lookup, class_lookup, &value_type,
                       &value) && value.addr) {
        std::vector<std::string> paths;
        StringUtil::stringtok(paths, value.addr, ", ");
        for (unsigned int i=0; i<paths.size(); ++i)
            FbTk::Image::addSearchPath(paths[i]);
    }

    if (!dbfile.empty()) {
        if (!m_screen_rm.load(dbfile.c_str())) {
            cerr<<_FBTEXT(Fluxbox, CantLoadRCFile, "Failed to load database", "Failed trying to read rc file")<<":"<<dbfile<<endl;
            cerr<<_FBTEXT(Fluxbox, CantLoadRCFileTrying, "Retrying with", "Retrying rc file loading with (the following file)")<<": "<<DEFAULT_INITFILE<<endl;
            if (!m_screen_rm.load(DEFAULT_INITFILE))
                cerr<<_FBTEXT(Fluxbox, CantLoadRCFile, "Failed to load database", "")<<": "<<DEFAULT_INITFILE<<endl;
        }
    } else {
        if (!m_screen_rm.load(DEFAULT_INITFILE))
            cerr<<_FBTEXT(Fluxbox, CantLoadRCFile, "Failed to load database", "")<<": "<<DEFAULT_INITFILE<<endl;
    }
}

void Fluxbox::loadRootCommand(BScreen &screen)	{

    string dbfile(getRcFilename());

    XrmDatabaseHelper database(dbfile.c_str());
    if (!*database)
        database = XrmGetFileDatabase(DEFAULT_INITFILE);

    XrmValue value;
    char *value_type, name_lookup[1024], class_lookup[1024];
    sprintf(name_lookup, "session.screen%d.rootCommand", screen.screenNumber());
    sprintf(class_lookup, "Session.Screen%d.RootCommand", screen.screenNumber());
    if (XrmGetResource(*database, name_lookup, class_lookup, &value_type,
                       &value)) {
        screen.saveRootCommand(value.addr==0 ? "": value.addr);
    } else
        screen.saveRootCommand("");

}

void Fluxbox::reload_rc() {
    load_rc();
    reconfigure();
}


void Fluxbox::reconfigure() {
    m_reconfigure_wait = true;
    m_reconfig_timer.start();
}


void Fluxbox::real_reconfigure() {

    XrmDatabase new_blackboxrc = (XrmDatabase) 0;

    string dbfile(getRcFilename());
    XrmDatabase old_blackboxrc = XrmGetFileDatabase(dbfile.c_str());

    XrmMergeDatabases(new_blackboxrc, &old_blackboxrc);
    XrmPutFileDatabase(old_blackboxrc, dbfile.c_str());

    if (old_blackboxrc)
        XrmDestroyDatabase(old_blackboxrc);

    ScreenList::iterator screen_it = m_screen_list.begin();
    ScreenList::iterator screen_it_end = m_screen_list.end();
    for (; screen_it != screen_it_end; ++screen_it)
        load_rc(*(*screen_it));

    // reconfigure all screens
    for_each(m_screen_list.begin(), m_screen_list.end(), mem_fun(&BScreen::reconfigure));

    //reconfigure keys
    m_key->reconfigure(StringUtil::expandFilename(*m_rc_keyfile).c_str());

}

BScreen *Fluxbox::findScreen(int id) {
    ScreenList::iterator it = m_screen_list.begin();
    ScreenList::iterator it_end = m_screen_list.end();
    for (; it != it_end; ++it) {
        if ((*it)->screenNumber() == id)
            break;
    }

    if (it == m_screen_list.end())
        return 0;

    return *it;
}

bool Fluxbox::menuTimestampsChanged() const {
    std::list<MenuTimestamp *>::const_iterator it = m_menu_timestamps.begin();
    std::list<MenuTimestamp *>::const_iterator it_end = m_menu_timestamps.end();
    for (; it != it_end; ++it) {
        
        time_t timestamp = FbTk::FileUtil::getLastStatusChangeTimestamp((*it)->filename.c_str());

        if (timestamp >= 0) {
            if (timestamp != (*it)->timestamp)
                return true;
        } else
            return true;
    }

    // no timestamp changed
    return false;
}

void Fluxbox::checkMenu() {
    if (menuTimestampsChanged())
        rereadMenu();
}

void Fluxbox::hideExtraMenus(BScreen &screen) {

#ifdef USE_TOOLBAR
        // hide toolbar that matches screen
        for (size_t toolbar = 0; toolbar < m_toolbars.size(); ++toolbar) {
            if (&(m_toolbars[toolbar]->screen()) == &screen)
                m_toolbars[toolbar]->menu().hide();
        }

#endif // USE_TOOLBAR

}

void Fluxbox::rereadMenu(bool show_after_reread) {
    m_reread_menu_wait = true;
    m_show_menu_after_reread = show_after_reread;
    m_reconfig_timer.start();
}


void Fluxbox::real_rereadMenu() {
    
    clearMenuFilenames();
    
    for_each(m_screen_list.begin(), 
             m_screen_list.end(), 
             mem_fun(&BScreen::rereadMenu));

    if(m_show_menu_after_reread) {

        FbCommands::ShowRootMenuCmd showcmd;
        showcmd.execute();

        m_show_menu_after_reread = false;
    }
}

void Fluxbox::saveMenuFilename(const char *filename) {
    if (filename == 0)
        return;

    bool found = false;

    std::list<MenuTimestamp *>::iterator it = m_menu_timestamps.begin();
    std::list<MenuTimestamp *>::iterator it_end = m_menu_timestamps.end();
    for (; it != it_end; ++it) {
        if ((*it)->filename == filename) {
            found = true;
            break;
        }
    }

    if (! found) {
        time_t timestamp = FbTk::FileUtil::getLastStatusChangeTimestamp(filename);

        if (timestamp >= 0) {
            MenuTimestamp *ts = new MenuTimestamp;

            ts->filename = filename;
            ts->timestamp = timestamp;

            m_menu_timestamps.push_back(ts);
        }
    }
}

void Fluxbox::clearMenuFilenames() {
    while(!m_menu_timestamps.empty()) {
        delete m_menu_timestamps.back();
        m_menu_timestamps.pop_back();
    }
}

void Fluxbox::timed_reconfigure() {
    if (m_reconfigure_wait)
        real_reconfigure();

    if (m_reread_menu_wait)
        real_rereadMenu();

    m_reconfigure_wait = m_reread_menu_wait = false;
}

// set focused window
void Fluxbox::setFocusedWindow(WinClient *client) {
    // already focused
    if (m_focused_window == client) {
#ifdef DEBUG
        cerr<<"Focused window already win"<<endl;
#endif // DEBUG
        return;
    }
#ifdef DEBUG
    cerr<<"Setting Focused window = "<<client<<endl;
    if (client != 0 && client->fbwindow() != 0)
        cerr<<"title: "<<client->fbwindow()->title()<<endl;
    cerr<<"Current Focused window = "<<m_focused_window<<endl;
    cerr<<"------------------"<<endl;
#endif // DEBUG
    BScreen *old_screen = 0, *screen = 0;
    WinClient *old_client = 0;

    if (m_focused_window != 0) {
        // check if m_focused_window is valid
        bool found = false;
        std::map<Window, WinClient *>::iterator it = m_window_search.begin();
        std::map<Window, WinClient *>::iterator it_end = m_window_search.end();
        for (; it != it_end; ++it) {
            if (it->second == m_focused_window) {
                // we found it, end loop
                found = true;
                break;
            }
        }

        if (!found) {
            m_focused_window = 0;
        } else {
            old_client = m_focused_window;
            old_screen = &old_client->screen();

            if (old_client->fbwindow()) {
                FluxboxWindow *old_win = old_client->fbwindow();

                if (!client || client->fbwindow() != old_win)
                    old_win->setFocusFlag(false);
            }
        }
    }

    if (client && client->fbwindow() && !client->fbwindow()->isIconic()) {
        FluxboxWindow *win = client->fbwindow();
        // make sure we have a valid win pointer with a valid screen
        ScreenList::iterator winscreen =
            std::find(m_screen_list.begin(), m_screen_list.end(),
                      &client->screen());
        if (winscreen == m_screen_list.end()) {
            m_focused_window = 0; // the window pointer wasn't valid, mark no window focused
        } else {
            screen = *winscreen;
            m_focused_window = client;     // update focused window
            win->setCurrentClient(*client, false); // don't setinputfocus
            win->setFocusFlag(true); // set focus flag
        }

    } else
        m_focused_window = 0;



    if (screen != 0) {
        screen->updateNetizenWindowFocus();
        for (AtomHandlerContainerIt it= m_atomhandler.begin();
             it != m_atomhandler.end(); it++) {
            (*it).first->updateFocusedWindow(*screen, (m_focused_window ?
                                                       m_focused_window->window() :
                                                       0));
        }
    }

    if (old_screen && old_screen != screen) {
        old_screen->updateNetizenWindowFocus();
        for (AtomHandlerContainerIt it= m_atomhandler.begin();
             it != m_atomhandler.end(); it++)
            (*it).first->updateFocusedWindow(*old_screen, 0);
    }

}

/**
 * This function is called whenever we aren't quite sure what
 * focus is meant to be, it'll make things right ;-)
 * last_focused is set to something if we want to make use of the
 * previously focused window (it must NOT be set focused now, it
 *   is probably dying).
 *
 * ignore_event means that it ignores the given event until
 * it gets a focusIn
 */
void Fluxbox::revertFocus(BScreen &screen) {
    // Relevant resources:
    // resource.focus_last = whether we focus last focused when changing workspace
    // BScreen::FocusModel = sloppy, click, whatever
    WinClient *next_focus = screen.getLastFocusedWindow(screen.currentWorkspaceID());

    // if setting focus fails, or isn't possible, fallback correctly
    if (!(next_focus && next_focus->fbwindow() &&
          next_focus->fbwindow()->setCurrentClient(*next_focus, true))) {
        setFocusedWindow(0); // so we don't get dangling m_focused_window pointer
        switch (screen.getFocusModel()) {
        case BScreen::SLOPPYFOCUS:
        case BScreen::SEMISLOPPYFOCUS:
            XSetInputFocus(FbTk::App::instance()->display(),
                           PointerRoot, None, CurrentTime);
            break;
        case BScreen::CLICKTOFOCUS:
            screen.rootWindow().setInputFocus(RevertToPointerRoot, CurrentTime);
            break;
        }
    }
}

/*
 * Like revertFocus, but specifically related to this window (transients etc)
 * if full_revert, we fallback to a full revertFocus if we can't find anything
 * local to the client.
 * If unfocus_frame is true, we won't focus anything in the same frame
 * as the client.
 *
 * So, we first prefer to choose a transient parent, then the last
 * client in this window, and if no luck (or unfocus_frame), then
 * we just use the normal revertFocus on the screen.
 *
 * assumption: client has focus
 */
void Fluxbox::unfocusWindow(WinClient &client, bool full_revert, bool unfocus_frame) {
    // go up the transient tree looking for a focusable window

    FluxboxWindow *fbwin = client.fbwindow();
    if (fbwin == 0)
        unfocus_frame = false;

    WinClient *trans_parent = client.transientFor();
    while (trans_parent) {
        if (trans_parent->fbwindow() && // can't focus if no fbwin
            (!unfocus_frame || trans_parent->fbwindow() != fbwin) && // can't be this window
            trans_parent->fbwindow()->isVisible() &&
            trans_parent->fbwindow()->setCurrentClient(*trans_parent, m_focused_window == &client)) {
            return;
        }
        trans_parent = trans_parent->transientFor();
    }

    if (fbwin == 0)
        return; // nothing more we can do

    BScreen &screen = fbwin->screen();

    if (!unfocus_frame) {
        WinClient *last_focus = screen.getLastFocusedWindow(*fbwin, &client);
        if (last_focus != 0 &&
            fbwin->setCurrentClient(*last_focus, m_focused_window == &client)) {
            return;
        }
    }

    if (full_revert && m_focused_window == &client)
        revertFocus(screen);

}


void Fluxbox::watchKeyRelease(BScreen &screen, unsigned int mods) {
    if (mods == 0) {
        cerr<<"WARNING: attempt to grab without modifiers!"<<endl;
        return;
    }
    m_watching_screen = &screen;
    m_watch_keyrelease = mods;
    XGrabKeyboard(FbTk::App::instance()->display(),
                  screen.rootWindow().window(), True,
                  GrabModeAsync, GrabModeAsync, CurrentTime);
}
