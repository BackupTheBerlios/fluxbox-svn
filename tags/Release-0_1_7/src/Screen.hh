// Screen.hh for Fluxbox Window Manager
// Copyright (c) 2001 - 2002 Henrik Kinnunen (fluxgen@linuxmail.org)
// 
// Screen.hh for Blackbox - an X11 Window manager
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

// $Id: Screen.hh,v 1.21 2002/02/26 22:25:53 fluxgen Exp $

#ifndef	 SCREEN_HH
#define	 SCREEN_HH

#include "Theme.hh"
#include "BaseDisplay.hh"
#include "Configmenu.hh"
#include "Icon.hh"
#include "Netizen.hh"
#include "Rootmenu.hh"
#include "Timer.hh"
#include "Workspace.hh"
#include "Workspacemenu.hh"
#include "fluxbox.hh"

#ifdef		SLIT
#	include "Slit.hh"
#endif // SLIT


#include <X11/Xlib.h>
#include <X11/Xresource.h>

#ifdef		TIME_WITH_SYS_TIME
#	include <sys/time.h>
#	include <time.h>
#else // !TIME_WITH_SYS_TIME
#	ifdef		HAVE_SYS_TIME_H
#		include <sys/time.h>
#	else // !HAVE_SYS_TIME_H
#		include <time.h>
#	endif // HAVE_SYS_TIME_H
#endif // TIME_WITH_SYS_TIME

#include <stdio.h>
#include <string>
#include <list>
#include <vector>
#include <fstream>

class BScreen : public ScreenInfo {
public:
	BScreen(ResourceManager &rm, Fluxbox *b, 
		const std::string &screenname, const std::string &altscreenname,
		int scrn);
	~BScreen();

	inline const bool isToolbarOnTop(void) { return *resource.toolbar_on_top; }
	inline const bool doToolbarAutoHide(void) { return *resource.toolbar_auto_hide; }
	inline const bool isSloppyFocus(void) { return resource.sloppy_focus; }
	inline const bool isSemiSloppyFocus(void) { return resource.semi_sloppy_focus; }
	inline const bool isRootColormapInstalled(void) { return root_colormap_installed; }
	inline const bool isScreenManaged(void) { return managed; }
	inline const bool isTabRotateVertical(void) { return *resource.tab_rotate_vertical; }
	inline const bool isSloppyWindowGrouping(void) { return *resource.sloppy_window_grouping; }
	inline const bool doAutoRaise(void) { return resource.auto_raise; }
	inline const bool doImageDither(void) { return *resource.image_dither; }
	inline const bool doMaxOverSlit(void) { return *resource.max_over_slit; }
	inline const bool doOpaqueMove(void) { return *resource.opaque_move; }
	inline const bool doFullMax(void) { return *resource.full_max; }
	inline const bool doFocusNew(void) { return *resource.focus_new; }
	inline const bool doFocusLast(void) { return *resource.focus_last; }

	inline const GC &getOpGC() const { return theme->getOpGC(); }
	
	inline const BColor *getBorderColor(void) { return &theme->getBorderColor(); }
	inline BImageControl *getImageControl(void) { return image_control; }
	inline Rootmenu *getRootmenu(void) { return rootmenu; }
	inline std::string &getRootCommand(void) { return *resource.rootcommand; }
#ifdef	 SLIT
	inline const Bool &isSlitOnTop(void) const { return resource.slit_on_top; }
	inline const Bool &doSlitAutoHide(void) const { return resource.slit_auto_hide; }
	inline Slit *getSlit(void) { return slit; }
	inline const int &getSlitPlacement(void) const { return resource.slit_placement; }
	inline const int &getSlitDirection(void) const { return resource.slit_direction; }
	inline void saveSlitPlacement(int p) { resource.slit_placement = p; }
	inline void saveSlitDirection(int d) { resource.slit_direction = d; }
	inline void saveSlitOnTop(Bool t)		{ resource.slit_on_top = t; }
	inline void saveSlitAutoHide(Bool t) { resource.slit_auto_hide = t; }
#endif // SLIT

	inline Toolbar *getToolbar(void) { return toolbar; }

	inline Workspace *getWorkspace(int w) { return workspacesList[w]; }
	inline Workspace *getCurrentWorkspace(void) { return current_workspace; }

	inline Workspacemenu *getWorkspacemenu(void) { return workspacemenu; }

	inline const unsigned int getHandleWidth(void) const { return theme->getHandleWidth(); }
	inline const unsigned int getBevelWidth(void) const	{ return theme->getBevelWidth(); }
	inline const unsigned int getFrameWidth(void) const { return theme->getFrameWidth(); }
	inline const unsigned int getBorderWidth(void) const { return theme->getBorderWidth(); }
	inline const unsigned int getBorderWidth2x(void) const { return theme->getBorderWidth()*2; }
	inline const int getCurrentWorkspaceID() { return current_workspace->getWorkspaceID(); }

    typedef std::vector<FluxboxWindow *> Icons;
	inline const int getCount(void) { return workspacesList.size(); }
	inline const int getIconCount(void) { return iconList.size(); }
	inline Icons &getIconList(void) { return iconList; }

	inline const int getNumberOfWorkspaces(void) { return *resource.workspaces; }
	inline const Toolbar::Placement getToolbarPlacement(void) { return *resource.toolbar_placement; }
	inline const int getToolbarWidthPercent(void) { return *resource.toolbar_width_percent; }
	inline const int getPlacementPolicy(void) const { return resource.placement_policy; }
	inline const int getEdgeSnapThreshold(void) { return *resource.edge_snap_threshold; }
	inline const int getRowPlacementDirection(void) const { return resource.row_direction; }
	inline const int getColPlacementDirection(void) const { return resource.col_direction; }
	inline const unsigned int getTabWidth(void) { return *resource.tab_width; }
	inline const unsigned int getTabHeight(void) { return *resource.tab_height; }
	inline const Tab::Placement getTabPlacement(void) { return *resource.tab_placement; }
	inline const Tab::Alignment getTabAlignment(void) { return *resource.tab_alignment; }

	inline void setRootColormapInstalled(Bool r) { root_colormap_installed = r; }
	inline void saveRootCommand(std::string rootcmd) { *resource.rootcommand = rootcmd; }
	inline void saveSloppyFocus(bool s) { resource.sloppy_focus = s; }
	inline void saveSemiSloppyFocus(bool s) { resource.semi_sloppy_focus = s; }
	inline void saveAutoRaise(bool a) { resource.auto_raise = a; }
	inline void saveWorkspaces(int w) { *resource.workspaces = w; }
	inline void saveToolbarOnTop(bool r) { *resource.toolbar_on_top = r; }
	inline void saveToolbarAutoHide(bool r) { *resource.toolbar_auto_hide = r; }
	inline void saveToolbarWidthPercent(int w) { *resource.toolbar_width_percent = w; }
	inline void saveToolbarPlacement(Toolbar::Placement p) { *resource.toolbar_placement = p; }
	inline void savePlacementPolicy(int p) { resource.placement_policy = p; }
	inline void saveRowPlacementDirection(int d) { resource.row_direction = d; }
	inline void saveColPlacementDirection(int d) { resource.col_direction = d; }
	inline void saveEdgeSnapThreshold(int t) { resource.edge_snap_threshold = t; }
	inline void saveImageDither(bool d) { resource.image_dither = d; }
	inline void saveMaxOverSlit(bool m) { resource.max_over_slit = m; }
	inline void saveOpaqueMove(bool o) { resource.opaque_move = o; }
	inline void saveFullMax(bool f) { resource.full_max = f; }
	inline void saveFocusNew(bool f) { resource.focus_new = f; }
	inline void saveFocusLast(bool f) { resource.focus_last = f; }
	inline void saveTabWidth(unsigned int w) { resource.tab_width = w; }
	inline void saveTabHeight(unsigned int h) { resource.tab_height = h; }
	inline void saveTabPlacement(Tab::Placement p) { *resource.tab_placement = p; }
	inline void saveTabAlignment(Tab::Alignment a) { *resource.tab_alignment = a; }
	inline void saveTabRotateVertical(Bool r) { resource.tab_rotate_vertical = r; }
	inline void saveSloppyWindowGrouping(Bool s) { resource.sloppy_window_grouping = s; }
	inline void iconUpdate(void) { iconmenu->update(); }
	inline Iconmenu *getIconmenu(void) { return iconmenu; }

	
	#ifdef HAVE_STRFTIME
	inline char *getStrftimeFormat(void) { return resource.strftime_format; }
	void saveStrftimeFormat(char *);
	#else // !HAVE_STRFTIME
	inline int getDateFormat(void) { return resource.date_format; }
	inline void saveDateFormat(int f) { resource.date_format = f; }
	inline Bool isClock24Hour(void) { return resource.clock24hour; }
	inline void saveClock24Hour(Bool c) { resource.clock24hour = c; }
	#endif // HAVE_STRFTIME

	inline Theme::WindowStyle *getWindowStyle(void) { return &theme->getWindowStyle(); } 
	inline Theme::MenuStyle *getMenuStyle(void) { return &theme->getMenuStyle(); } 
	inline Theme::ToolbarStyle *getToolbarStyle(void) { return &theme->getToolbarStyle(); } 

	FluxboxWindow *getIcon(int);

	int addWorkspace(void);
	int removeLastWorkspace(void);
	//scroll workspaces
	void nextWorkspace(const int delta);
	void prevWorkspace(const int delta);
	void rightWorkspace(const int delta);
	void leftWorkspace(const int delta);

	void removeWorkspaceNames(void);
	void updateWorkspaceNamesAtom(void);
	
	void addWorkspaceName(char *);
	void addNetizen(Netizen *);
	void removeNetizen(Window);
	void addIcon(FluxboxWindow *);
	void removeIcon(FluxboxWindow *);
	void getNameOfWorkspace(int, char **);
	void changeWorkspaceID(int);
	void sendToWorkspace(int);
	void sendToWorkspace(int, bool);
	void raiseWindows(Window *, int);
	void reassociateWindow(FluxboxWindow *, int, Bool);
	void prevFocus(void);
	void nextFocus(void);
	void raiseFocus(void);
	void reconfigure(void);	
	void rereadMenu(void);
	void shutdown(void);
	void showPosition(int, int);
	void showGeometry(unsigned int, unsigned int);
	void hideGeometry(void);

	void updateNetizenCurrentWorkspace(void);
	void updateNetizenWorkspaceCount(void);
	void updateNetizenWindowFocus(void);
	void updateNetizenWindowAdd(Window, unsigned long);
	void updateNetizenWindowDel(Window);
	void updateNetizenConfigNotify(XEvent *);
	void updateNetizenWindowRaise(Window);
	void updateNetizenWindowLower(Window);

	enum { ROWSMARTPLACEMENT = 1, COLSMARTPLACEMENT, CASCADEPLACEMENT, LEFTRIGHT,
				 RIGHTLEFT, TOPBOTTOM, BOTTOMTOP };
	enum { LEFTJUSTIFY = 1, RIGHTJUSTIFY, CENTERJUSTIFY };
	enum { ROUNDBULLET = 1, TRIANGELBULLET, SQUAERBULLET, NOBULLET };
	enum { RESTART = 1, RESTARTOTHER, EXIT, SHUTDOWN, EXECUTE, RECONFIGURE,
				 WINDOWSHADE, WINDOWICONIFY, WINDOWMAXIMIZE, WINDOWCLOSE, WINDOWRAISE,
				 WINDOWLOWER, WINDOWSTICK, WINDOWKILL, SETSTYLE, WINDOWTAB};

private:
	#ifdef GNOME
	void initGnomeAtoms();
	void updateGnomeClientList();
	Window gnome_win;
	#endif
	Theme *theme;
	
	Bool root_colormap_installed, managed, geom_visible;
	GC opGC;
	Pixmap geom_pixmap;
	Window geom_window;

	Fluxbox *fluxbox;
	BImageControl *image_control;
	Configmenu *configmenu;
	Iconmenu *iconmenu;

	Rootmenu *rootmenu;

    typedef std::list<Rootmenu *> Rootmenus;
    typedef std::list<Netizen *> Netizens;

    Rootmenus rootmenuList;
    Netizens netizenList;
    Icons iconList;

	#ifdef		SLIT
	Slit *slit;
	#endif // SLIT

	Toolbar *toolbar;
	Workspace *current_workspace;
	Workspacemenu *workspacemenu;

	unsigned int geom_w, geom_h;
	unsigned long event_mask;

    typedef std::vector<std::string> WorkspaceNames;
    typedef std::vector<Workspace *> Workspaces;

    WorkspaceNames workspaceNames;
    Workspaces workspacesList;
	
	struct ScreenResource {
		ScreenResource(ResourceManager &rm, const std::string &scrname,
			const std::string &altscrname);

		Resource<bool> toolbar_on_top, toolbar_auto_hide,
			image_dither, opaque_move, full_max,
			max_over_slit, tab_rotate_vertical,
			sloppy_window_grouping, focus_last, focus_new;
		Resource<std::string> rootcommand;		
		bool auto_raise, sloppy_focus, semi_sloppy_focus,
			ordered_dither;
		Resource<int> workspaces, toolbar_width_percent, edge_snap_threshold,
			tab_width, tab_height;
		int placement_policy, row_direction, col_direction;

		Resource<Tab::Placement> tab_placement;
		Resource<Tab::Alignment> tab_alignment;
		Resource<Toolbar::Placement> toolbar_placement;


		#ifdef SLIT
		Bool slit_on_top, slit_auto_hide;
		int slit_placement, slit_direction;
		#endif // SLIT


		#ifdef	HAVE_STRFTIME
		char *strftime_format;
		#else // !HAVE_STRFTIME
		Bool clock24hour;
		int date_format;
		#endif // HAVE_STRFTIME

	} resource;

	void createStyleMenu(Rootmenu *menu, bool newmenu, const char *label, const char *directory);
protected:
	Bool parseMenuFile(std::ifstream &, Rootmenu *, int&);

	bool readDatabaseTexture(char *, char *, BTexture *, unsigned long);
	bool readDatabaseColor(char *, char *, BColor *, unsigned long);

	void readDatabaseFontSet(char *, char *, XFontSet *);
	XFontSet createFontSet(char *);
	void readDatabaseFont(char *, char *, XFontStruct **);

	void InitMenu(void);


};


#endif // _SCREEN_HH_
