// fluxbox.hh for Fluxbox Window Manager
// Copyright (c) 2001 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
//
// blackbox.hh for Blackbox - an X11 Window manager
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

#ifndef	 FLUXBOX_HH
#define	 FLUXBOX_HH

#include "FbTk/App.hh"
#include "FbTk/Resource.hh"
#include "FbTk/Timer.hh"
#include "FbTk/Observer.hh"
#include "FbTk/SignalHandler.hh"
#include "AttentionNoticeHandler.hh"

#include <X11/Xlib.h>
#include <X11/Xresource.h>

#ifdef HAVE_CSTDIO
  #include <cstdio>
#else
  #include <stdio.h>
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else // !TIME_WITH_SYS_TIME
#ifdef	HAVE_SYS_TIME_H
#include <sys/time.h>
#else // !HAVE_SYS_TIME_H
#include <time.h>
#endif // HAVE_SYS_TIME_H
#endif // TIME_WITH_SYS_TIME

#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>

class AtomHandler;
class FluxboxWindow;
class WinClient;
class Keys;
class BScreen;
class FbAtoms;
class Toolbar;


///	main class for the window manager.
/**
	singleton type
*/
class Fluxbox : public FbTk::App,
                public FbTk::SignalEventHandler,
                public FbTk::Observer {
public:
    Fluxbox(int argc, char **argv, const char * dpy_name= 0,
            const char *rcfilename = 0);
    virtual ~Fluxbox();

    static Fluxbox *instance() { return s_singleton; }

    /// main event loop
    void eventLoop();
    bool validateWindow(Window win) const;
    bool validateClient(const WinClient *client) const;

    void grab();
    void ungrab();
    Keys *keys() { return m_key.get(); }
    Atom getFluxboxPidAtom() const { return m_fluxbox_pid; }

    // Not currently implemented until we decide how it'll be used
    //WinClient *searchGroup(Window);
    WinClient *searchWindow(Window);

    int initScreen(int screen_nr);
    BScreen *searchScreen(Window w);

    unsigned int getDoubleClickInterval() const { return *m_rc_double_click_interval; }
    Time getLastTime() const { return m_last_time; }

    AtomHandler *getAtomHandler(const std::string &name);
    void addAtomHandler(AtomHandler *atomh, const std::string &name);
    void removeAtomHandler(AtomHandler *atomh);

    /// obsolete
    enum TabsAttachArea{ATTACH_AREA_WINDOW= 0, ATTACH_AREA_TITLEBAR};


    bool getIgnoreBorder() const { return *m_rc_ignoreborder; }
    bool &getPseudoTrans() { return *m_rc_pseudotrans; }

    Fluxbox::TabsAttachArea getTabsAttachArea() const { return *m_rc_tabs_attach_area; }
    const std::string &getStyleFilename() const { return *m_rc_stylefile; }
    const std::string &getStyleOverlayFilename() const { return *m_rc_styleoverlayfile; }

    const std::string &getMenuFilename() const { return *m_rc_menufile; }
    const std::string &getSlitlistFilename() const { return *m_rc_slitlistfile; }
    const std::string &getAppsFilename() const { return *m_rc_appsfile; }
    int colorsPerChannel() const { return *m_rc_colors_per_channel; }
    int getNumberOfLayers() const { return *m_rc_numlayers; }
    int getTabsPadding() const { return *m_rc_tabs_padding; }
    int getFocusedTabMinWidth() const { return *m_rc_focused_tab_min_width; }

    // class to store layer numbers (special Resource type)
    // we have a special resource type because we need to be able to name certain layers
    // a Resource<int> wouldn't allow this


    time_t getAutoRaiseDelay() const { return *m_rc_auto_raise_delay; }

    unsigned int getCacheLife() const { return *m_rc_cache_life * 60000; }
    unsigned int getCacheMax() const { return *m_rc_cache_max; }


    unsigned int getModKey() const;
    void setModKey(const char*);

    void maskWindowEvents(Window w, FluxboxWindow *bw)
        { m_masked = w; m_masked_window = bw; }

    void watchKeyRelease(BScreen &screen, unsigned int mods);

    void shutdown();
    void load_rc(BScreen &scr);
    void saveStyleFilename(const char *val) { m_rc_stylefile = (val == 0 ? "" : val); }
    void saveMenuFilename(const char *);
    void clearMenuFilenames();
    void saveWindowSearch(Window win, WinClient *winclient);
    // some windows relate to the group, not the client, so we record separately
    // searchWindow on these windows will give the active client in the group
    void saveWindowSearchGroup(Window win, FluxboxWindow *fbwin);
    void saveGroupSearch(Window win, WinClient *winclient);
    void save_rc();
    void removeWindowSearch(Window win);
    void removeWindowSearchGroup(Window win);
    void removeGroupSearch(Window win);
    void restart(const char *command = 0);
    void reconfigure();
    void rereadMenu(bool show_after_reread = false);
    /// reloads the menus if the timestamps changed

    void hideExtraMenus(BScreen &screen);

    /// handle any system signal sent to the application
    void handleSignal(int signum);
    void update(FbTk::Subject *changed);
    /**
     * Sends update signal to atomhandlers,
     * @param screen the new screen
     * @param old_screen the old screen if any, can be the same as new screen
     */
    void updateFocusedWindow(BScreen *screen, BScreen *old_screen);
    /// todo, remove this. just temporary
    void updateFrameExtents(FluxboxWindow &win);

    void attachSignals(FluxboxWindow &win);
    void attachSignals(WinClient &winclient);

    void timed_reconfigure();

    bool isStartup() const { return m_starting; }
    bool isRestarting() const { return m_restarting; }

    const std::string &getRestartArgument() const { return m_restart_argument; }

    /// get screen from number
    BScreen *findScreen(int num);

    /// @return whether the timestamps on the menu changed
    bool menuTimestampsChanged() const;
    bool haveShape() const { return m_have_shape; }
    int shapeEventbase() const { return m_shape_eventbase; }
    void getDefaultDataFilename(char *name, std::string &);
    // screen mouse was in at last key event
    BScreen *mouseScreen() { return m_mousescreen; }
    // screen of window that last key event (i.e. focused window) went to
    BScreen *keyScreen() { return m_keyscreen; }
    // screen we are watching for modifier changes
    BScreen *watchingScreen() { return m_watching_screen; }
    const XEvent &lastEvent() const { return m_last_event; }

    AttentionNoticeHandler &attentionHandler() { return m_attention_handler; }

private:

    typedef struct MenuTimestamp {
        std::string filename;
        time_t timestamp;
    } MenuTimestamp;



    std::string getRcFilename();
    void load_rc();
    void reload_rc();

    void real_rereadMenu();
    void real_reconfigure();

    void handleEvent(XEvent *xe);

    void setupConfigFiles();
    void handleButtonEvent(XButtonEvent &be);
    void handleUnmapNotify(XUnmapEvent &ue);
    void handleClientMessage(XClientMessageEvent &ce);
    void handleKeyEvent(XKeyEvent &ke);

    std::auto_ptr<FbAtoms> m_fbatoms;

    FbTk::ResourceManager m_resourcemanager, &m_screen_rm;

    //--- Resources

    FbTk::Resource<bool> m_rc_tabs, m_rc_ignoreborder;
    FbTk::Resource<bool> m_rc_pseudotrans;
    FbTk::Resource<int> m_rc_colors_per_channel, m_rc_numlayers,
        m_rc_double_click_interval,
        m_rc_tabs_padding,
        m_rc_focused_tab_min_width;
    FbTk::Resource<std::string> m_rc_stylefile,
        m_rc_styleoverlayfile,
        m_rc_menufile, m_rc_keyfile, m_rc_slitlistfile,
        m_rc_groupfile, m_rc_appsfile;


    FbTk::Resource<TabsAttachArea> m_rc_tabs_attach_area;
    FbTk::Resource<unsigned int> m_rc_cache_life, m_rc_cache_max;
    FbTk::Resource<time_t> m_rc_auto_raise_delay;
    FbTk::Resource<std::string> m_rc_mod_key;

    typedef std::map<Window, WinClient *> WinClientMap;
    WinClientMap m_window_search;
    typedef std::map<Window, FluxboxWindow *> WindowMap;
    WindowMap m_window_search_group;
    // A window is the group leader, which can map to several
    // WinClients in the group, it is *not* fluxbox's concept of groups
    // See ICCCM section 4.1.11
    // The group leader (which may not be mapped, so may not have a WinClient)
    // will have it's window being the group index
    std::multimap<Window, WinClient *> m_group_search;

    std::list<MenuTimestamp *> m_menu_timestamps;
    typedef std::list<BScreen *> ScreenList;
    ScreenList m_screen_list;

    FluxboxWindow *m_masked_window;

    BScreen *m_mousescreen, *m_keyscreen;
    BScreen *m_watching_screen;
    unsigned int m_watch_keyrelease;

    Atom m_fluxbox_pid;

    bool m_reconfigure_wait, m_reread_menu_wait;
    Time m_last_time;
    Window m_masked;
    std::string m_rc_file; ///< resource filename
    char **m_argv;
    int m_argc;

    std::string m_restart_argument; ///< what to restart

    XEvent m_last_event;

    FbTk::Timer m_reconfig_timer; ///< when we execute reconfig command we must wait at least to next event round

    std::auto_ptr<Keys> m_key;

    //default arguments for titlebar left and right
    static Fluxbox *s_singleton;

    typedef std::map<AtomHandler *, std::string> AtomHandlerContainer;
    typedef AtomHandlerContainer::iterator AtomHandlerContainerIt;

    AtomHandlerContainer m_atomhandler;
    typedef std::vector<Toolbar *> Toolbars;
    Toolbars m_toolbars;

    bool m_starting;
    bool m_restarting;
    bool m_shutdown;
    bool m_show_menu_after_reread;
    int m_server_grabs;
    int m_randr_event_type; ///< the type number of randr event
    int m_shape_eventbase; ///< event base for shape events
    bool m_have_shape; ///< if shape is supported by server
    const char *m_RC_PATH;
    const char *m_RC_INIT_FILE;
    Atom m_kwm1_dockwindow, m_kwm2_dockwindow;

    AttentionNoticeHandler m_attention_handler;
};


#endif // FLUXBOX_HH
