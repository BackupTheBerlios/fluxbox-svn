// Screen.hh for Fluxbox Window Manager
// Copyright (c) 2001 - 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
// 
// Screen.hh for Blackbox - an X11 Window manager
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

// $Id: Screen.hh,v 1.67 2003/02/16 00:36:17 fluxgen Exp $

#ifndef	 SCREEN_HH
#define	 SCREEN_HH

#include "Theme.hh"
#include "BaseDisplay.hh"
#include "Workspace.hh"
#include "Tab.hh"
#include "Resource.hh"
#include "Subject.hh"
#include "FbWinFrameTheme.hh"
#include "MultLayers.hh"
#include "XLayerItem.hh"

#include <X11/Xlib.h>
#include <X11/Xresource.h>

#include <cstdio>
#include <string>
#include <list>
#include <vector>
#include <fstream>
#include <memory>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

class Netizen;
class Slit;
class Toolbar;
class FbWinFrameTheme;
class RootTheme;

namespace FbTk {
class MenuTheme;
class Menu;
class ImageControl;
};

/// Handles screen connection and screen clients
/**
 Create a toolbar and workspaces, handles switching between workspaces and windows
 */
class BScreen : public ScreenInfo {
public:
    typedef std::vector<Workspace *> Workspaces;
    typedef std::vector<std::string> WorkspaceNames;
	
    BScreen(ResourceManager &rm,
            const std::string &screenname, const std::string &altscreenname,
            int scrn, int number_of_layers);
    ~BScreen();

    inline bool isToolbarOnTop() const { return *resource.toolbar_on_top; }
    inline bool doToolbarAutoHide() const { return *resource.toolbar_auto_hide; }
    inline bool isSloppyFocus() const { return resource.sloppy_focus; }
    inline bool isSemiSloppyFocus() const { return resource.semi_sloppy_focus; }
    inline bool isRootColormapInstalled() const { return root_colormap_installed; }
    inline bool isScreenManaged() const { return managed; }
    inline bool isTabRotateVertical() const { return *resource.tab_rotate_vertical; }
    inline bool isSloppyWindowGrouping() const { return *resource.sloppy_window_grouping; }
    inline bool isWorkspaceWarping() const { return *resource.workspace_warping; }
    inline bool isDesktopWheeling() const { return *resource.desktop_wheeling; }
    inline bool doAutoRaise() const { return resource.auto_raise; }
    inline bool doImageDither() const { return *resource.image_dither; }
    inline bool doMaxOverSlit() const { return *resource.max_over_slit; }
    inline bool doOpaqueMove() const { return *resource.opaque_move; }
    inline bool doFullMax() const { return *resource.full_max; }
    inline bool doFocusNew() const { return *resource.focus_new; }
    inline bool doFocusLast() const { return *resource.focus_last; }
    inline bool doShowWindowPos() const { return *resource.show_window_pos; }
    bool antialias() const { return *resource.antialias; }
    inline GC getOpGC() const { return theme->getOpGC(); }
	
    inline const FbTk::Color *getBorderColor() const { return &theme->getBorderColor(); }
    inline FbTk::ImageControl *getImageControl() { return image_control; }
    const FbTk::Menu * const getRootmenu() const { return m_rootmenu.get(); }
    FbTk::Menu * const getRootmenu() { return m_rootmenu.get(); }
	
    inline const std::string &getRootCommand() const { return *resource.rootcommand; }

    inline bool isSlitOnTop() const { return resource.slit_on_top; }
    inline bool doSlitAutoHide() const { return resource.slit_auto_hide; }
#ifdef SLIT
    inline Slit *getSlit() { return m_slit.get(); }
    inline const Slit *getSlit() const { return m_slit.get(); }
#endif // SLIT
    inline int getSlitPlacement() const { return resource.slit_placement; }
    inline int getSlitDirection() const { return resource.slit_direction; }
    inline void saveSlitPlacement(int p) { resource.slit_placement = p;  }
    inline void saveSlitDirection(int d) { resource.slit_direction = d;  }
    inline void saveSlitOnTop(bool t) { resource.slit_on_top = t;  }
    inline void saveSlitAutoHide(bool t) { resource.slit_auto_hide = t;  }

    inline unsigned int getSlitOnHead() const { return resource.slit_on_head; }
    inline void saveSlitOnHead(unsigned int h) { resource.slit_on_head = h;  }

    inline const Toolbar *getToolbar() const { return m_toolbar.get(); }
    inline Toolbar *getToolbar() { return m_toolbar.get(); }

    inline Workspace *getWorkspace(unsigned int w) { return ( w < workspacesList.size() ? workspacesList[w] : 0); }
    inline Workspace *getCurrentWorkspace() { return current_workspace; }

    const FbTk::Menu *getWorkspacemenu() const { return workspacemenu.get(); }
    FbTk::Menu *getWorkspacemenu() { return workspacemenu.get(); }

    inline unsigned int getHandleWidth() const { return theme->getHandleWidth(); }
    inline unsigned int getBevelWidth() const { return theme->getBevelWidth(); }
    inline unsigned int getFrameWidth() const { return theme->getFrameWidth(); }
    inline unsigned int getBorderWidth() const { return theme->getBorderWidth(); }
    inline unsigned int getBorderWidth2x() const { return theme->getBorderWidth()*2; }
    inline unsigned int getCurrentWorkspaceID() const { return current_workspace->workspaceID(); }

    /*
      maximum screen surface
    */
    unsigned int getMaxLeft() const;
    unsigned int getMaxRight() const;
    unsigned int getMaxTop() const;
    unsigned int getMaxBottom() const;

    typedef std::vector<FluxboxWindow *> Icons;

    /// @return number of workspaces
    inline unsigned int getCount() const { return workspacesList.size(); }
    /// @return number of icons
    inline unsigned int getIconCount() const { return iconList.size(); }
    inline const Icons &getIconList() const { return iconList; }
    inline Icons &getIconList() { return iconList; }
    const Workspaces &getWorkspacesList() const { return workspacesList; }
    const WorkspaceNames &getWorkspaceNames() const { return workspaceNames; }
    /**
       @name Screen signals
    */
    //@{
    /// client list signal
    FbTk::Subject &clientListSig() { return m_clientlist_sig; } 
    /// workspace count signal
    FbTk::Subject &workspaceCountSig() { return m_workspacecount_sig; }
    /// workspace names signal 
    FbTk::Subject &workspaceNamesSig() { return m_workspacenames_sig; }
    /// current workspace signal
    FbTk::Subject &currentWorkspaceSig() { return m_currentworkspace_sig; }
    //@}
		
    /// @return the resource value of number of workspace
    inline int getNumberOfWorkspaces() const { return *resource.workspaces; }	

    inline int getToolbarOnHead() { return *resource.toolbar_on_head; }

    inline int getToolbarWidthPercent() const { return *resource.toolbar_width_percent; }
    inline int getPlacementPolicy() const { return resource.placement_policy; }
    inline int getEdgeSnapThreshold() const { return *resource.edge_snap_threshold; }
    inline int getRowPlacementDirection() const { return resource.row_direction; }
    inline int getColPlacementDirection() const { return resource.col_direction; }
    inline unsigned int getTabWidth() const { return *resource.tab_width; }
    inline unsigned int getTabHeight() const { return *resource.tab_height; }
    inline Tab::Placement getTabPlacement() const { return *resource.tab_placement; }
    inline Tab::Alignment getTabAlignment() const { return *resource.tab_alignment; }

    inline void setRootColormapInstalled(Bool r) { root_colormap_installed = r;  }
    inline void saveRootCommand(std::string rootcmd) { *resource.rootcommand = rootcmd;  }
    inline void saveSloppyFocus(bool s) { resource.sloppy_focus = s;  }
    inline void saveSemiSloppyFocus(bool s) { resource.semi_sloppy_focus = s;  }
    inline void saveAutoRaise(bool a) { resource.auto_raise = a;  }
    inline void saveWorkspaces(int w) { *resource.workspaces = w;  }
    inline void saveToolbarOnTop(bool r) { *resource.toolbar_on_top = r;  }
    inline void saveToolbarAutoHide(bool r) { *resource.toolbar_auto_hide = r;  }
    inline void saveToolbarWidthPercent(int w) { *resource.toolbar_width_percent = w;  }

    inline void saveToolbarOnHead(int head) { *resource.toolbar_on_head = head;  }


    inline void savePlacementPolicy(int p) { resource.placement_policy = p;  }
    inline void saveRowPlacementDirection(int d) { resource.row_direction = d;  }
    inline void saveColPlacementDirection(int d) { resource.col_direction = d;  }
    inline void saveEdgeSnapThreshold(int t) { resource.edge_snap_threshold = t;  }
    inline void saveImageDither(bool d) { resource.image_dither = d;  }
    inline void saveMaxOverSlit(bool m) { resource.max_over_slit = m;  }
    inline void saveOpaqueMove(bool o) { resource.opaque_move = o;  }
    inline void saveFullMax(bool f) { resource.full_max = f;  }
    inline void saveFocusNew(bool f) { resource.focus_new = f;  }
    inline void saveFocusLast(bool f) { resource.focus_last = f;  }
    inline void saveTabWidth(unsigned int w) { resource.tab_width = w;  }
    inline void saveTabHeight(unsigned int h) { resource.tab_height = h;  }
    inline void saveTabPlacement(Tab::Placement p) { *resource.tab_placement = p;  }
    inline void saveTabAlignment(Tab::Alignment a) { *resource.tab_alignment = a;  }
    inline void saveTabRotateVertical(bool r) { resource.tab_rotate_vertical = r;   }
    inline void saveSloppyWindowGrouping(bool s) { resource.sloppy_window_grouping = s;  }
    inline void saveWorkspaceWarping(bool s) { resource.workspace_warping = s; }
    inline void saveDesktopWheeling(bool s) { resource.desktop_wheeling = s; }
    void iconUpdate();

    void setAntialias(bool value);
	
    inline const char *getStrftimeFormat() { return resource.strftime_format.c_str(); }
    void saveStrftimeFormat(const char *format);

    inline int getDateFormat() { return resource.date_format; }
    inline void saveDateFormat(int f) { resource.date_format = f; }
    inline bool isClock24Hour() { return resource.clock24hour; }
    inline void saveClock24Hour(bool c) { resource.clock24hour = c; }

    inline Theme::WindowStyle *getWindowStyle() { return &theme->getWindowStyle(); } 
    inline FbWinFrameTheme &winFrameTheme() { return m_windowtheme; }
    inline const FbWinFrameTheme &winFrameTheme() const { return m_windowtheme; }
    inline FbTk::MenuTheme *menuTheme() { return m_menutheme.get(); }
    inline const FbTk::MenuTheme *menuTheme() const { return m_menutheme.get(); }

    const Theme *getTheme() const { return theme; }
    FluxboxWindow *getIcon(unsigned int index);
    FbTk::MultLayers &layerManager() { return m_layermanager; }
    const FbTk::MultLayers &layerManager() const { return m_layermanager; }

    int addWorkspace();
    int removeLastWorkspace();
    //scroll workspaces
    void nextWorkspace() { nextWorkspace(1); }
    void prevWorkspace() { prevWorkspace(1); }
    void nextWorkspace(int delta);
    void prevWorkspace(int delta);
    void rightWorkspace(int delta);
    void leftWorkspace(int delta);

    void removeWorkspaceNames();
    void updateWorkspaceNamesAtom();
	
    void addWorkspaceName(const char *name);
    void addNetizen(Netizen *net);
    void removeNetizen(Window win);
    void addIcon(FluxboxWindow *win);
    void removeIcon(FluxboxWindow *win);
    // remove window
    void removeWindow(FluxboxWindow *win);

    std::string getNameOfWorkspace(unsigned int workspace) const;
    void changeWorkspaceID(unsigned int);
    void sendToWorkspace(unsigned int workspace, FluxboxWindow *win=0, bool changeworkspace=true);
    void raiseWindows(const Workspace::Stack &workspace_stack);
    void reassociateGroup(FluxboxWindow *window, unsigned int workspace_id, bool ignore_sticky);
    void reassociateWindow(FluxboxWindow *window, unsigned int workspace_id, bool ignore_sticky);
    void prevFocus() { prevFocus(0); }
    void nextFocus() { nextFocus(0); }
    void prevFocus(int options);
    void nextFocus(int options);
    void raiseFocus();
    void reconfigure();	
    void rereadMenu();
    void shutdown();
    void showPosition(int x, int y);
    void showGeometry(unsigned int, unsigned int);
    void hideGeometry();

    void setLayer(FbTk::XLayerItem &item, int layernum);
    // remove? no, items are never removed from their layer until they die

    FluxboxWindow* useAutoGroupWindow();

    void updateNetizenCurrentWorkspace();
    void updateNetizenWorkspaceCount();
    void updateNetizenWindowFocus();
    void updateNetizenWindowAdd(Window, unsigned long);
    void updateNetizenWindowDel(Window);
    void updateNetizenConfigNotify(XEvent *);
    void updateNetizenWindowRaise(Window);
    void updateNetizenWindowLower(Window);
    /// create window frame for client window and attach it
    FluxboxWindow *createWindow(Window clientwin);
    void setupWindowActions(FluxboxWindow &win);

    enum { ROWSMARTPLACEMENT = 1, COLSMARTPLACEMENT, CASCADEPLACEMENT, LEFTRIGHT,
           RIGHTLEFT, TOPBOTTOM, BOTTOMTOP };
    enum { LEFTJUSTIFY = 1, RIGHTJUSTIFY, CENTERJUSTIFY };

    /// obsolete
    enum { ROUNDBULLET = 1, TRIANGELBULLET, SQUAERBULLET, NOBULLET };
    /// obsolete
    enum { RESTART = 1, RESTARTOTHER, EXIT, SHUTDOWN, EXECUTE, RECONFIGURE,
           WINDOWSHADE, WINDOWICONIFY, WINDOWMAXIMIZE, WINDOWCLOSE, WINDOWRAISE,
           WINDOWLOWER, WINDOWSTICK, WINDOWKILL, SETSTYLE, WINDOWTAB};
    // prevFocus/nextFocus option bits
    enum { CYCLESKIPLOWERTABS = 0x01, CYCLESKIPSTUCK = 0x02, CYCLESKIPSHADED = 0x04,
           CYCLEDEFAULT = 0x00 };

    class ScreenSubject:public FbTk::Subject {
    public:
        ScreenSubject(BScreen &scr):m_scr(scr) { }
        const BScreen &screen() const { return m_scr; }
        BScreen &screen() { return m_scr; }
    private:
        BScreen &m_scr;
    };
	
private:
    void setupConfigmenu(FbTk::Menu &menu);
    void createStyleMenu(FbTk::Menu &menu, bool newmenu, const char *label, const char *directory);

    bool parseMenuFile(std::ifstream &filestream, FbTk::Menu &menu, int &row);

    void initMenu();

    bool doSkipWindow(const FluxboxWindow *w, int options);

    ScreenSubject 
    m_clientlist_sig,  ///< client signal
        m_workspacecount_sig, ///< workspace count signal
        m_workspacenames_sig, ///< workspace names signal 
        m_currentworkspace_sig; ///< current workspace signal
		
    FbTk::MultLayers m_layermanager;
    //!!	
    Theme *theme; ///< obsolete
	
    Bool root_colormap_installed, managed, geom_visible;
    GC opGC;
    Pixmap geom_pixmap;
    FbTk::FbWindow geom_window;

    FbTk::ImageControl *image_control;
    std::auto_ptr<FbTk::Menu> m_configmenu;

    std::auto_ptr<FbTk::Menu> m_rootmenu;

    typedef std::list<FbTk::Menu *> Rootmenus;
    typedef std::list<Netizen *> Netizens;

    Rootmenus rootmenuList;
    Netizens netizenList;
    Icons iconList;
#ifdef SLIT
    std::auto_ptr<Slit> m_slit;
#endif // SLIT
    std::auto_ptr<Toolbar> m_toolbar;
    Workspace *current_workspace;
    std::auto_ptr<FbTk::Menu> workspacemenu;

    unsigned int geom_w, geom_h;
    unsigned long event_mask;

    WorkspaceNames workspaceNames;
    Workspaces workspacesList;

    Window auto_group_window;
	
    FbWinFrameTheme m_windowtheme;
    std::auto_ptr<FbTk::MenuTheme> m_menutheme;

    struct ScreenResource {
        ScreenResource(ResourceManager &rm, const std::string &scrname,
                       const std::string &altscrname);

        Resource<bool> toolbar_on_top, toolbar_auto_hide,
            image_dither, opaque_move, full_max,
            max_over_slit, tab_rotate_vertical,
            sloppy_window_grouping, workspace_warping,
            desktop_wheeling, show_window_pos,
            focus_last, focus_new,
            antialias;
        Resource<std::string> rootcommand;		
        bool auto_raise, sloppy_focus, semi_sloppy_focus,
            ordered_dither;
        Resource<int> workspaces, toolbar_width_percent, edge_snap_threshold,
            tab_width, tab_height;
        int placement_policy, row_direction, col_direction;

        Resource<Tab::Placement> tab_placement;
        Resource<Tab::Alignment> tab_alignment;
        Resource<int> toolbar_on_head;

        bool slit_on_top, slit_auto_hide;
        int slit_placement, slit_direction;

        unsigned int slit_on_head;

        std::string strftime_format;

        bool clock24hour;
        int date_format;


    } resource;

    std::auto_ptr<RootTheme> m_root_theme;
};


#endif // SCREEN_HH
