// fluxbox.hh for Fluxbox Window Manager
// Copyright (c) 2001 - 2002 Henrik Kinnunen (fluxgen@linuxmail.org)
//
// blackbox.hh for Blackbox - an X11 Window manager
// Copyright (c) 1997 - 2000 Brad Hughes (bhughes@tcac.net)
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.	IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// $Id: fluxbox.hh,v 1.27 2002/08/17 22:13:00 fluxgen Exp $

#ifndef	 FLUXBOX_HH
#define	 FLUXBOX_HH

#include "Resource.hh"
#include "Keys.hh"
#include "BaseDisplay.hh"
#include "Image.hh"
#include "Timer.hh"
#include "Window.hh"
#include "Tab.hh"
#include "Toolbar.hh"

#ifdef	SLIT
#include "Slit.hh"
#endif // SLIT

#include "SignalHandler.hh"
#include "FbAtoms.hh"

#include <X11/Xlib.h>
#include <X11/Xresource.h>

#include <cstdio>

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

#include <string>
#include <vector>
#include <map>
#include <list>

/**
	main class for the window manager.
	singleton type
*/
class Fluxbox : public BaseDisplay, public TimeoutHandler, 
	public FbTk::EventHandler<FbTk::SignalEvent>,
	public FbAtoms {
public:
	Fluxbox(int argc, char **argv, const char * dpy_name= 0, const char *rc = 0);	
	virtual ~Fluxbox();
	
	static Fluxbox *instance() { return singleton; }
	
	inline bool useTabs() { return *m_rc_tabs; }
	inline bool useIconBar() { return *m_rc_iconbar; }
	inline void saveTabs(bool value) { *m_rc_tabs = value; }
	inline void saveIconBar(bool value) { m_rc_iconbar = value; }
#ifdef HAVE_GETPID
	inline Atom getFluxboxPidAtom() const { return fluxbox_pid; }
	#ifdef KDE
	//For KDE dock applets
	inline Atom getKWM1DockwindowAtom() const { return kwm1_dockwindow; } //KDE v1.x
	inline Atom getKWM2DockwindowAtom() const { return kwm2_dockwindow; } //KDE v2.x
	#endif
#endif // HAVE_GETPID

	Basemenu *searchMenu(Window);

	FluxboxWindow *searchGroup(Window, FluxboxWindow *);
	FluxboxWindow *searchWindow(Window);
	inline FluxboxWindow *getFocusedWindow() { return focused_window; }

		
	BScreen *searchScreen(Window w);

	inline const Time &getDoubleClickInterval() const { return resource.double_click_interval; }
	inline const Time &getLastTime() const { return last_time; }

	Toolbar *searchToolbar(Window w);
	Tab *searchTab(Window);
	
	/// obsolete
	enum Titlebar{SHADE=0, MINIMIZE, MAXIMIZE, CLOSE, STICK, MENU, EMPTY};		
	
	inline const std::vector<Fluxbox::Titlebar>& getTitlebarRight() { return *m_rc_titlebar_right; }
	inline const std::vector<Fluxbox::Titlebar>& getTitlebarLeft() { return *m_rc_titlebar_left; }
	inline const char *getStyleFilename() const { return m_rc_stylefile->c_str(); }

	inline const char *getMenuFilename() const { return m_rc_menufile->c_str(); }
	inline const std::string &getSlitlistFilename() const { return *m_rc_slitlistfile; }
	inline int colorsPerChannel() const { return *m_rc_colors_per_channel; }

	inline const timeval &getAutoRaiseDelay() const { return resource.auto_raise_delay; }

	inline unsigned int getCacheLife() const { return *m_rc_cache_life * 60000; }
	inline unsigned int getCacheMax() const { return *m_rc_cache_max; }

	inline void maskWindowEvents(Window w, FluxboxWindow *bw)
		{ masked = w; masked_window = bw; }
	inline void setNoFocus(Bool f) { no_focus = f; }

	void setFocusedWindow(FluxboxWindow *w);
	void shutdown();
	void load_rc(BScreen *);
	void loadRootCommand(BScreen *);
	void loadTitlebar();
	void saveStyleFilename(const char *val) { m_rc_stylefile = (val == 0 ? "" : val); }
	void saveMenuFilename(const char *);
	void saveTitlebarFilename(const char *);
	void saveSlitlistFilename(const char *val) { m_rc_slitlistfile = (val == 0 ? "" : val); }
	void saveMenuSearch(Window, Basemenu *);
	void saveWindowSearch(Window, FluxboxWindow *);
	void saveToolbarSearch(Window, Toolbar *);
	void saveTabSearch(Window, Tab *);
	void saveGroupSearch(Window, FluxboxWindow *);	
	void save_rc();
	void removeMenuSearch(Window);
	void removeWindowSearch(Window);
	void removeToolbarSearch(Window);
	void removeTabSearch(Window);
	void removeGroupSearch(Window);
	void restart(const char * = 0);
	void reconfigure();
	void reconfigureTabs();
	void rereadMenu();
	void checkMenu();
	
	/// handle any system signal sent to the application
	void handleEvent(FbTk::SignalEvent * const signum);

	virtual void timeout();
	
	inline const Cursor &getSessionCursor() const { return cursor.session; }
	inline const Cursor &getMoveCursor() const { return cursor.move; }
	inline const Cursor &getLowerLeftAngleCursor() const { return cursor.ll_angle; }
	inline const Cursor &getLowerRightAngleCursor() const { return cursor.lr_angle; }


#ifdef	SLIT
	Slit *searchSlit(Window);

	void saveSlitSearch(Window, Slit *);
	void removeSlitSearch(Window);
#endif // SLIT

#ifndef	 HAVE_STRFTIME

	enum { B_AMERICANDATE = 1, B_EUROPEANDATE };
#endif // HAVE_STRFTIME
	
	typedef std::vector<Fluxbox::Titlebar> TitlebarList;
		
private:
	struct cursor {
		Cursor session, move, ll_angle, lr_angle;
	} cursor;

	void setupConfigFiles();
	void handleButtonEvent(XButtonEvent &be);
	void handleUnmapNotify(XUnmapEvent &ue);
	void handleClientMessage(XClientMessageEvent &ce);
	void handleKeyEvent(XKeyEvent &ke);	
	void doWindowAction(Keys::KeyAction action, const int param);
	#ifdef GNOME
	bool checkGnomeAtoms(XClientMessageEvent &ce);
	#endif
	#ifdef NEWWMSPEC
	bool checkNETWMAtoms(XClientMessageEvent &ce);
	#endif
	typedef struct MenuTimestamp {
		char *filename;
		time_t timestamp;
	} MenuTimestamp;

	struct resource {
		Time double_click_interval;		
		timeval auto_raise_delay;
	} resource;
		
	ResourceManager m_resourcemanager, m_screen_rm;
	
	//--- Resources
	Resource<bool> m_rc_tabs, m_rc_iconbar;
	Resource<int> m_rc_colors_per_channel;
	Resource<std::string> m_rc_stylefile, 
		m_rc_menufile, m_rc_keyfile, m_rc_slitlistfile,
		m_rc_groupfile;
	
	Resource<TitlebarList> m_rc_titlebar_left, m_rc_titlebar_right;
	Resource<unsigned int> m_rc_cache_life, m_rc_cache_max;

	void setTitlebar(std::vector<Fluxbox::Titlebar>& dir, const char *arg);
	
	std::map<Window, FluxboxWindow *> windowSearch;
	std::map<Window, FluxboxWindow *> groupSearch;
	std::map<Window, Basemenu *> menuSearch;
	std::map<Window, Toolbar *> toolbarSearch;
	typedef std::map<Window, Tab *> TabList;
	TabList tabSearch;
	
#ifdef SLIT
	std::map<Window, Slit *> slitSearch;
#ifdef KDE
	//For KDE dock applets
	Atom kwm1_dockwindow; //KDE v1.x
	Atom kwm2_dockwindow; //KDE v2.x
#endif//KDE
#endif // SLIT

	std::list<MenuTimestamp *> menuTimestamps;
	typedef std::list<BScreen *> ScreenList;
	ScreenList screenList;

	FluxboxWindow *focused_window, *masked_window;
	BTimer timer;

#ifdef		HAVE_GETPID
	Atom fluxbox_pid;
#endif // HAVE_GETPID

	bool no_focus, reconfigure_wait, reread_menu_wait;
	Time last_time;
	Window masked;
	std::string rc_file; ///< resource filename
	char **argv;
	int argc;
	Keys *key;
	std::string slitlist_path;
	//default arguments for titlebar left and right
	static Fluxbox::Titlebar m_titlebar_left[], m_titlebar_right[];

	std::string getRcFilename();
	void getDefaultDataFilename(char *, std::string &);
	void load_rc();
	
	void reload_rc();
	void real_rereadMenu();
	void real_reconfigure();

	void handleEvent(XEvent *xe);
	static Fluxbox *singleton;

};


#endif // _FLUXBOX_HH_
