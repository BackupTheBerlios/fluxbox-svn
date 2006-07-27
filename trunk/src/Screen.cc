// Screen.cc for Fluxbox Window Manager
// Copyright (c) 2001 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
//
// Screen.cc for Blackbox - an X11 Window manager
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


#include "Screen.hh"

#include "fluxbox.hh"
#include "Window.hh"
#include "Workspace.hh"
#include "Netizen.hh"

#include "Layer.hh"
#include "FocusControl.hh"
#include "ScreenPlacement.hh"

#include "STLUtil.hh"

// themes
#include "FbWinFrameTheme.hh"
#include "MenuTheme.hh"
#include "RootTheme.hh"
#include "WinButtonTheme.hh"
#include "SlitTheme.hh"

// menu items
#include "BoolMenuItem.hh"
#include "IntResMenuItem.hh"
#include "FocusModelMenuItem.hh"

// menus
#include "FbMenu.hh"
#include "LayerMenu.hh"

#include "MenuCreator.hh"

#include "WinClient.hh"
#include "FbWinFrame.hh"
#include "Strut.hh"
#include "CommandParser.hh"
#include "AtomHandler.hh"
#include "HeadArea.hh"


#include "FbTk/I18n.hh"
#include "FbTk/Subject.hh"
#include "FbTk/FbWindow.hh"
#include "FbTk/SimpleCommand.hh"
#include "FbTk/MultLayers.hh"
#include "FbTk/XLayerItem.hh"
#include "FbTk/MacroCommand.hh"
#include "FbTk/StringUtil.hh"
#include "FbTk/ImageControl.hh"
#include "FbTk/EventManager.hh"
#include "FbTk/Transparent.hh"
#include "FbTk/Select2nd.hh"
#include "FbTk/Compose.hh"
#include "FbTk/FbString.hh"

//use GNU extensions
#ifndef	 _GNU_SOURCE
#define	 _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef SLIT
#include "Slit.hh"
#include "SlitClient.hh"
#else
// fill it in
class Slit {};
#endif // SLIT

#ifdef STDC_HEADERS
#include <sys/types.h>
#endif // STDC_HEADERS

#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif // HAVE_CTYPE_H

#ifdef HAVE_UNISTD_H
#include <sys/types.h>
#include <unistd.h>
#endif // HAVE_UNISTD_H

#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif // HAVE_STDARG_H

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

#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>

#ifdef XINERAMA
extern  "C" {
#include <X11/extensions/Xinerama.h>
}
#endif // XINERAMA

#include <iostream>
#include <memory>
#include <algorithm>
#include <functional>
#include <stack>

using namespace std;

static bool running = true;
namespace {

int anotherWMRunning(Display *display, XErrorEvent *) {
    _FB_USES_NLS;
    cerr<<_FB_CONSOLETEXT(Screen, AnotherWMRunning, 
                  "BScreen::BScreen: an error occured while querying the X server.\n"
                  "	another window manager already running on display ",
                  "Message when another WM is found already active on all screens")
        <<DisplayString(display)<<endl;

    running = false;

    return -1;
}


class TabPlacementMenuItem: public FbTk::MenuItem {
public:
    TabPlacementMenuItem(FbTk::FbString & label, BScreen &screen, 
                         FbWinFrame::TabPlacement place, 
                         FbTk::RefCount<FbTk::Command> &cmd):
        FbTk::MenuItem(label, cmd),
        m_screen(screen),
        m_place(place) { }

    bool isEnabled() const { return m_screen.getTabPlacement() != m_place; }
    void click(int button, int time) {
        m_screen.saveTabPlacement(m_place);
        FbTk::MenuItem::click(button, time);
    }


private:
    BScreen &m_screen;
    FbWinFrame::TabPlacement m_place;
};

} // end anonymous namespace



namespace FbTk {

template<>
void FbTk::Resource<FbWinFrame::TabPlacement>::
setFromString(const char *strval) {
    if (strcasecmp(strval, "TopLeft")==0)
        m_value = FbWinFrame::TOPLEFT;
    else if (strcasecmp(strval, "BottomLeft")==0)
        m_value = FbWinFrame::BOTTOMLEFT;
    else if (strcasecmp(strval, "TopRight")==0)
        m_value = FbWinFrame::TOPRIGHT;
    else if (strcasecmp(strval, "BottomRight")==0)
        m_value = FbWinFrame::BOTTOMRIGHT;
    else if (strcasecmp(strval, "LeftTop") == 0)
        m_value = FbWinFrame::LEFTTOP;
    else if (strcasecmp(strval, "LeftBottom") == 0)
        m_value = FbWinFrame::LEFTBOTTOM;
    else if (strcasecmp(strval, "RightTop") == 0)
        m_value = FbWinFrame::RIGHTTOP;
    else if (strcasecmp(strval, "RightBottom") == 0)
        m_value = FbWinFrame::RIGHTBOTTOM;
    else
        setDefaultValue();
}

template<>
string FbTk::Resource<FbWinFrame::TabPlacement>::
getString() const {
    switch (m_value) {
    case FbWinFrame::TOPLEFT:
        return string("TopLeft");
        break;
    case FbWinFrame::BOTTOMLEFT:
        return string("BottomLeft");
        break;
    case FbWinFrame::TOPRIGHT:
        return string("TopRight");
        break;
    case FbWinFrame::BOTTOMRIGHT:
        return string("BottomRight");
        break;
    case FbWinFrame::LEFTTOP:
        return string("LeftTop");
        break;
    case FbWinFrame::LEFTBOTTOM:
        return string("LeftBottom");
        break;
    case FbWinFrame::RIGHTTOP:
        return string("RightTop");
        break;
    case FbWinFrame::RIGHTBOTTOM:
        return string("RightBottom");
        break;
    }
    //default string
    return string("TopLeft");
}
} // end namespace FbTk


BScreen::ScreenResource::ScreenResource(FbTk::ResourceManager &rm, 
                                        const std::string &scrname, 
                                        const std::string &altscrname):
    image_dither(rm, false, scrname+".imageDither", altscrname+".ImageDither"),
    opaque_move(rm, false, scrname + ".opaqueMove", altscrname+".OpaqueMove"),
    full_max(rm, true, scrname+".fullMaximization", altscrname+".FullMaximization"),
    workspace_warping(rm, true, scrname+".workspacewarping", altscrname+".WorkspaceWarping"),
    desktop_wheeling(rm, true, scrname+".desktopwheeling", altscrname+".DesktopWheeling"),
    reverse_wheeling(rm, false, scrname+".reversewheeling", altscrname+".ReverseWheeling"),
    show_window_pos(rm, true, scrname+".showwindowposition", altscrname+".ShowWindowPosition"),
    auto_raise(rm, false, scrname+".autoRaise", altscrname+".AutoRaise"),
    click_raises(rm, true, scrname+".clickRaises", altscrname+".ClickRaises"),
    decorate_transient(rm, false, scrname+".decorateTransient", altscrname+".DecorateTransient"),
    rootcommand(rm, "", scrname+".rootCommand", altscrname+".RootCommand"),
    resize_model(rm, BOTTOMRESIZE, scrname+".resizeMode", altscrname+".ResizeMode"),
    tab_placement(rm, FbWinFrame::TOPLEFT, scrname+".tab.placement", altscrname+".Tab.Placement"),
    windowmenufile(rm, "", scrname+".windowMenu", altscrname+".WindowMenu"),
    follow_model(rm, IGNORE_OTHER_WORKSPACES, scrname+".followModel", altscrname+".followModel"),
    workspaces(rm, 1, scrname+".workspaces", altscrname+".Workspaces"),
    edge_snap_threshold(rm, 0, scrname+".edgeSnapThreshold", altscrname+".EdgeSnapThreshold"),
    focused_alpha(rm, 255, scrname+".window.focus.alpha", altscrname+".Window.Focus.Alpha"),
    unfocused_alpha(rm, 255, scrname+".window.unfocus.alpha", altscrname+".Window.Unfocus.Alpha"),
    menu_alpha(rm, 255, scrname+".menu.alpha", altscrname+".Menu.Alpha"),
    menu_delay(rm, 0, scrname + ".menuDelay", altscrname+".MenuDelay"),
    menu_delay_close(rm, 0, scrname + ".menuDelayClose", altscrname+".MenuDelayClose"),
    tab_width(rm, 64, scrname + ".tab.width", altscrname+".Tab.Width"),
    menu_mode(rm, FbTk::MenuTheme::DELAY_OPEN, scrname+".menuMode", altscrname+".MenuMode"),

    gc_line_width(rm, 1, scrname+".overlay.lineWidth", altscrname+".Overlay.LineWidth"),
    gc_line_style(rm, 
                  FbTk::GContext::LINESOLID, 
                  scrname+".overlay.lineStyle", 
                  altscrname+".Overlay.LineStyle"),
    gc_join_style(rm,
                  FbTk::GContext::JOINMITER,
                  scrname+".overlay.joinStyle",
                  altscrname+".Overlay.JoinStyle"),
    gc_cap_style(rm,
                 FbTk::GContext::CAPNOTLAST,
                 scrname+".overlay.capStyle",
                 altscrname+".overlay.CapStyle"),
    scroll_action(rm, "", scrname+".windowScrollAction", altscrname+".WindowScrollAction"),
    scroll_reverse(rm, false, scrname+".windowScrollReverse", altscrname+".WindowScrollReverse"),
    max_over_tabs(rm, false, scrname+".tabs.maxOver", altscrname+".Tabs.MaxOver"),
    default_internal_tabs(rm, false /* TODO: autoconf option? */ , scrname+".tabs.intitlebar", altscrname+".Tabs.InTitlebar") {
    

}

BScreen::BScreen(FbTk::ResourceManager &rm,
                 const std::string &screenname,
                 const std::string &altscreenname,
                 int scrn, int num_layers) : 
    m_clientlist_sig(*this),  // client signal
    m_iconlist_sig(*this), // icon list signal
    m_workspacecount_sig(*this), // workspace count signal
    m_workspacenames_sig(*this), // workspace names signal 
    m_workspace_area_sig(*this), // workspace area signal
    m_currentworkspace_sig(*this), // current workspace signal
    m_reconfigure_sig(*this), // reconfigure signal
    m_resize_sig(*this),
    m_layermanager(num_layers),
    m_windowtheme(new FbWinFrameTheme(scrn)), 
    // the order of windowtheme and winbutton theme is important
    // because winbutton need to rescale the pixmaps in winbutton theme
    // after fbwinframe have resized them
    m_winbutton_theme(new WinButtonTheme(scrn, *m_windowtheme)),
    m_menutheme(new MenuTheme(scrn)),
    m_root_window(scrn),
    m_geom_window(m_root_window,
                  0, 0, 10, 10, 
                  false,  // override redirect
                  true), // save under
    m_pos_window(m_root_window,
                 0, 0, 10, 10,
                 false,  // override redirect
                 true), // save under
    resource(rm, screenname, altscreenname),
    m_resource_manager(rm),
    m_name(screenname),
    m_altname(altscreenname),
    m_focus_control(new FocusControl(*this)),
    m_placement_strategy(new ScreenPlacement(*this)),
    m_xinerama_headinfo(0),
    m_shutdown(false) {


    Display *disp = m_root_window.display();

    initXinerama();

    // setup error handler to catch "screen already managed by other wm"
    XErrorHandler old = XSetErrorHandler((XErrorHandler) anotherWMRunning);

    rootWindow().setEventMask(ColormapChangeMask | EnterWindowMask | PropertyChangeMask |
                              SubstructureRedirectMask | KeyPressMask | KeyReleaseMask |
                              ButtonPressMask | ButtonReleaseMask| SubstructureNotifyMask);

    FbTk::App::instance()->sync(false);

    XSetErrorHandler((XErrorHandler) old);

    managed = running;
    if (! managed) {
        delete m_placement_strategy; m_placement_strategy = 0;
        delete m_focus_control; m_focus_control = 0;
        return;
    }

    // TODO fluxgen: check if this is the right place
    m_head_areas = new HeadArea[numHeads() ? numHeads() : 1];
	
    _FB_USES_NLS;
	
    fprintf(stderr, _FB_CONSOLETEXT(Screen, ManagingScreen,
                            "BScreen::BScreen: managing screen %d "
                            "using visual 0x%lx, depth %d\n", 
                            "informational message saying screen number (%d), visual (%lx), and colour depth (%d)").c_str(),
            screenNumber(), XVisualIDFromVisual(rootWindow().visual()),
            rootWindow().depth());

    
    rootWindow().setCursor(XCreateFontCursor(disp, XC_left_ptr));

    // load this screens resources
    Fluxbox *fluxbox = Fluxbox::instance();
    fluxbox->load_rc(*this);

    // setup image cache engine
    m_image_control.reset(new FbTk::ImageControl(scrn, true, 
                                                 fluxbox->colorsPerChannel(),
                                                 fluxbox->getCacheLife(), fluxbox->getCacheMax()));
    imageControl().installRootColormap();
    root_colormap_installed = true;

    m_root_theme.reset(new RootTheme(*resource.rootcommand, imageControl()));

    m_windowtheme->setFocusedAlpha(*resource.focused_alpha);
    m_windowtheme->setUnfocusedAlpha(*resource.unfocused_alpha);
    m_menutheme->setAlpha(*resource.menu_alpha);
    m_menutheme->setMenuMode(*resource.menu_mode);
    // clamp values
    if (*resource.menu_delay > 5000)
        *resource.menu_delay = 5000;
    if (*resource.menu_delay < 0)
        *resource.menu_delay = 0;

    if (*resource.menu_delay_close > 5000)
        *resource.menu_delay_close = 5000;
    if (*resource.menu_delay_close < 0)
        *resource.menu_delay_close = 0;

    m_menutheme->setDelayOpen(*resource.menu_delay);
    m_menutheme->setDelayClose(*resource.menu_delay_close);

    imageControl().setDither(*resource.image_dither);

    winFrameTheme().reconfigSig().attach(this);// for geom window


    geom_visible = false;
    geom_pixmap = 0;
    pos_visible = false;
    pos_pixmap = 0;

    renderGeomWindow();
    renderPosWindow();

    // setup workspaces and workspace menu
    int nr_ws = *resource.workspaces;
    addWorkspace(); // at least one
    for (int i = 1; i < nr_ws; ++i) {
        addWorkspace();
    }

    m_current_workspace = m_workspaces_list.front();
    

    //!! TODO: we shouldn't do this more than once, but since slit handles their
    // own resources we must do this.
    fluxbox->load_rc(*this);

    m_configmenu.reset(createMenu(_FB_XTEXT(Menu, Configuration, 
                                  "Configuration", "Title of configuration menu")));
    setupConfigmenu(*m_configmenu.get());
    m_configmenu->setInternalMenu();

    // start with workspace 0
    changeWorkspaceID(0);
    updateNetizenWorkspaceCount();

    // we need to load win frame theme before we create any fluxbox window
    // and after we've load the resources
    // else we get some bad handle/grip height/width
    //    FbTk::ThemeManager::instance().loadTheme(*m_windowtheme.get());
    //!! TODO: For some strange reason we must load everything,
    // else the focus label doesn't get updated
    // So we lock root theme temporary so it doesn't uses RootTheme::reconfigTheme
    // This must be fixed in the future.
    FbTk::ThemeManager::instance().load(fluxbox->getStyleFilename(),
                                        fluxbox->getStyleOverlayFilename(),
                                        m_root_theme->screenNum());
    m_root_theme->setLineAttributes(*resource.gc_line_width,
                                    *resource.gc_line_style,
                                    *resource.gc_cap_style,
                                    *resource.gc_join_style);

#ifdef SLIT
    m_slit.reset(new Slit(*this, *layerManager().getLayer(Layer::DESKTOP),
                 fluxbox->getSlitlistFilename().c_str()));
#endif // SLIT

    rm.unlock();
  
    XFlush(disp);
}



BScreen::~BScreen() {
    

    if (! managed)
        return;
    
    if (m_rootmenu.get() != 0)
        m_rootmenu->removeAll();
    
    // Since workspacemenu holds client list menus (from workspace)
    // we need to destroy it before we destroy workspaces
    m_workspacemenu.reset(0);

    ExtraMenus::iterator mit = m_extramenus.begin();
    ExtraMenus::iterator mit_end = m_extramenus.end();
    for (; mit != mit_end; ++mit) {
        // we set them to NOT internal so that they will be deleted when the
        // menu is cleaned up. We can't delete them here because they are
        // still in the menu
        // (They need to be internal for most of the time so that if we
        // rebuild the menu, then they won't be removed.
        if (mit->second->parent() == 0) {
            // not attached to our windowmenu
            // so we clean it up
            delete mit->second;
        } else {
            // let the parent clean it up
            mit->second->setInternalMenu(false);
        }
    }
    
    if (geom_pixmap != None)
        imageControl().removeImage(geom_pixmap);

    if (pos_pixmap != None)
        imageControl().removeImage(pos_pixmap);

    removeWorkspaceNames();
    using namespace STLUtil;
    destroyAndClear(m_workspaces_list);
    destroyAndClear(m_netizen_list);
    destroyAndClear(m_managed_resources);

    //why not destroyAndClear(m_icon_list); ? 
    //problem with that: a delete FluxboxWindow* calls m_diesig.notify()
    //which leads to screen.removeWindow() which leads to removeIcon(win)
    //which would modify the m_icon_list anyways...
    Icons tmp;
    tmp = m_icon_list;
    while(!tmp.empty()) {
        removeWindow(tmp.back());
        tmp.back()->restore(true);
        delete (tmp.back());
        tmp.pop_back();
    }
    
    if (hasXinerama() && m_xinerama_headinfo) {
        delete [] m_xinerama_headinfo;
    }

    // slit must be destroyed before headAreas (Struts)
    m_slit.reset(0);

    // TODO fluxgen: check if this is the right place
    delete [] m_head_areas;

    delete m_focus_control;
    delete m_placement_strategy;

}

void BScreen::initWindows() {
    unsigned int nchild;
    Window r, p, *children;
    Display *disp = FbTk::App::instance()->display();
    XQueryTree(disp, rootWindow().window(), &r, &p, &children, &nchild);

    // preen the window list of all icon windows... for better dockapp support
    for (unsigned int i = 0; i < nchild; i++) {

        if (children[i] == None)
            continue;

        XWMHints *wmhints = XGetWMHints(disp, children[i]);

        if (wmhints) {
            if ((wmhints->flags & IconWindowHint) &&
                (wmhints->icon_window != children[i]))
                for (unsigned int j = 0; j < nchild; j++) {
                    if (children[j] == wmhints->icon_window) {
#ifdef DEBUG
                        cerr<<"BScreen::initWindows(): children[j] = 0x"<<hex<<children[j]<<dec<<endl;
                        cerr<<"BScreen::initWindows(): = icon_window"<<endl;
#endif // DEBUG
                        children[j] = None;
                        break;
                    }
                }
            XFree(wmhints);
        }

    }

    Fluxbox *fluxbox = Fluxbox::instance();

    // manage shown windows
    Window transient_for = 0;
    bool safety_flag = false;
    unsigned int num_transients = 0;
    for (unsigned int i = 0; i <= nchild; ++i) {
        if (i == nchild) {
            if (num_transients) {
                nchild = num_transients;
                i = num_transients = 0;
                safety_flag = true;
            } else
                break;
        }

        if (children[i] == None)
            continue;
        else if (!fluxbox->validateWindow(children[i])) {
#ifdef DEBUG
            cerr<<"BScreen::initWindows(): not valid window = "<<hex<<children[i]<<dec<<endl;
#endif // DEBUG
            children[i] = None;
            continue;
        }

        // if we have a transient_for window and it isn't created yet...
        // postpone creation of this window until after all others
        if (XGetTransientForHint(disp, children[i], &transient_for) &&
            fluxbox->searchWindow(transient_for) == 0 && !safety_flag) {
            // add this window back to the beginning of the list of children
            children[num_transients] = children[i];
            num_transients++;

#ifdef DEBUG
            cerr<<"BScreen::initWindows(): postpone creation of 0x"<<hex<<children[i]<<dec<<endl;
            cerr<<"BScreen::initWindows(): transient_for = 0x"<<hex<<transient_for<<dec<<endl;
#endif // DEBUG
            continue;
        }


        XWindowAttributes attrib;
        if (XGetWindowAttributes(disp, children[i],
                                 &attrib)) {
            if (attrib.override_redirect) {
                children[i] = None; // we dont need this anymore, since we already created a window for it
                continue;
            }

            if (attrib.map_state != IsUnmapped) {
                FluxboxWindow *win = createWindow(children[i]);

                if (win) {
                    XMapRequestEvent mre;
                    mre.window = children[i];
                    win->mapRequestEvent(mre);
                }
            }
        }
        children[i] = None; // we dont need this anymore, since we already created a window for it
    }


    XFree(children);

}

unsigned int BScreen::currentWorkspaceID() const { 
    return m_current_workspace->workspaceID(); 
}

const Strut* BScreen::availableWorkspaceArea(int head) const {
    return m_head_areas[head ? head-1 : 0].availableWorkspaceArea();
}

unsigned int BScreen::maxLeft(int head) const {
	
    // we ignore strut if we're doing full maximization
    if (hasXinerama())
        return doFullMax() ? getHeadX(head) : 
            getHeadX(head) + availableWorkspaceArea(head)->left();
    else
        return doFullMax() ? 0 : availableWorkspaceArea(head)->left();
}

unsigned int BScreen::maxRight(int head) const {
    // we ignore strut if we're doing full maximization
    if (hasXinerama())
        return doFullMax() ? getHeadX(head) + getHeadWidth(head) : 
            getHeadX(head) + getHeadWidth(head) - availableWorkspaceArea(head)->right();
    else
        return doFullMax() ? width() : width() - availableWorkspaceArea(head)->right();
}

unsigned int BScreen::maxTop(int head) const {
    // we ignore strut if we're doing full maximization

    if (hasXinerama())
        return doFullMax() ? getHeadY(head) : getHeadY(head) + availableWorkspaceArea(head)->top();
    else
        return doFullMax() ? 0 : availableWorkspaceArea(head)->top();
}

unsigned int BScreen::maxBottom(int head) const {
    // we ignore strut if we're doing full maximization

    if (hasXinerama())
        return doFullMax() ? getHeadY(head) + getHeadHeight(head) :
            getHeadY(head) + getHeadHeight(head) - availableWorkspaceArea(head)->bottom();
    else
        return doFullMax() ? height() : height() - availableWorkspaceArea(head)->bottom();
}

void BScreen::update(FbTk::Subject *subj) {
    // for now we're only listening to the theme sig, so no object check
    // if another signal is added later, will need to differentiate here

    renderGeomWindow();
    renderPosWindow();

    Fluxbox *fluxbox = Fluxbox::instance();

    // and update frame extents on theme changes
    Workspaces::iterator w_it = getWorkspacesList().begin();
    const Workspaces::iterator w_it_end = getWorkspacesList().end();
    for (; w_it != w_it_end; ++w_it) {
        Workspace::Windows::iterator win_it = (*w_it)->windowList().begin();
        const Workspace::Windows::iterator win_it_end = (*w_it)->windowList().end();
        for (; win_it != win_it_end; ++win_it)
            fluxbox->updateFrameExtents(**win_it);
    }

    Icons::iterator it = iconList().begin();
    const Icons::iterator it_end = iconList().end();
    for (; it != it_end; ++it)
        fluxbox->updateFrameExtents(**it);
    
}

FbTk::Menu *BScreen::createMenu(const std::string &label) {
    FbTk::Menu *menu = new FbMenu(menuTheme(), 
                                  imageControl(), 
                                  *layerManager().getLayer(Layer::MENU));
    if (!label.empty())
        menu->setLabel(label);

    return menu;
}
FbTk::Menu *BScreen::createToggleMenu(const std::string &label) {
    FbTk::Menu *menu = new ToggleMenu(menuTheme(), 
                                      imageControl(), 
                                      *layerManager().getLayer(Layer::MENU));
    if (!label.empty())
        menu->setLabel(label);

    return menu;
}

void BScreen::addExtraWindowMenu(const FbTk::FbString &label, FbTk::Menu *menu) {
    menu->setInternalMenu();
    menu->disableTitle();
    m_extramenus.push_back(std::make_pair(label, menu));
    // recreate window menu
    m_windowmenu.reset(MenuCreator::createMenuType("windowmenu", screenNumber()));
    m_windowmenu->setInternalMenu();
}

void BScreen::hideMenus() {
    // hide extra menus
    Fluxbox::instance()->hideExtraMenus(*this);

#ifdef SLIT
    // hide slit menu
    if (slit())
        slit()->menu().hide();
#endif // SLIT

    // hide icon menus
    if (!iconList().empty()) {
        Icons::iterator it = iconList().begin();
        const Icons::iterator it_end = iconList().end();
        for (; it != it_end; ++it)
            (*it)->menu().hide();
    }
    // hide all client menus
    hideWindowMenus(); 

}

void BScreen::hideWindowMenus(const FluxboxWindow* except) {
    Workspaces::iterator w_it = getWorkspacesList().begin();
    const Workspaces::iterator w_it_end = getWorkspacesList().end();
    for (; w_it != w_it_end; ++w_it) {
        if (!(*w_it)->windowList().empty()) {
            Workspace::Windows::iterator win_it = (*w_it)->windowList().begin();
            const Workspace::Windows::iterator win_it_end = (*w_it)->windowList().end();
            for (; win_it != win_it_end; ++win_it) {
                if (*win_it != except)
                    (*win_it)->menu().hide();
            }
        }
    }
}



void BScreen::reconfigure() {
    Fluxbox *fluxbox = Fluxbox::instance();

    m_windowtheme->setFocusedAlpha(*resource.focused_alpha);
    m_windowtheme->setUnfocusedAlpha(*resource.unfocused_alpha);
    m_menutheme->setAlpha(*resource.menu_alpha);
    m_menutheme->setMenuMode(*resource.menu_mode);

    // clamp values
    if (*resource.menu_delay > 5000)
        *resource.menu_delay = 5000;
    if (*resource.menu_delay < 0)
        *resource.menu_delay = 0;

    if (*resource.menu_delay_close > 5000)
        *resource.menu_delay_close = 5000;
    if (*resource.menu_delay_close < 0)
        *resource.menu_delay_close = 0;

    m_root_theme->setLineAttributes(*resource.gc_line_width,
                                    *resource.gc_line_style,
                                    *resource.gc_cap_style,
                                    *resource.gc_join_style);

    m_menutheme->setDelayOpen(*resource.menu_delay);
    m_menutheme->setDelayClose(*resource.menu_delay_close);

    renderGeomWindow();
    renderPosWindow();

    // realize the number of workspaces from the init-file
    const unsigned int nr_ws = *resource.workspaces;
    if (nr_ws > m_workspaces_list.size()) {
        while(nr_ws != m_workspaces_list.size()) {
            addWorkspace();
        }
    } else if (nr_ws < m_workspaces_list.size()) {
        while(nr_ws != m_workspaces_list.size()) {
            removeLastWorkspace();
        }
    }

    //reconfigure menus
    m_workspacemenu->reconfigure();
    m_configmenu->reconfigure();
    // recreate window menu
    m_windowmenu.reset(MenuCreator::createMenuType("windowmenu", screenNumber()));
    m_windowmenu->setInternalMenu();

    // We need to check to see if the timestamps
    // changed before we actually can restore the menus 
    // in the same way, since we can't really say if
    // any submenu is in the same place as before if the
    // menu changed.

    // if timestamp changed then no restoring
    bool restore_menus = ! fluxbox->menuTimestampsChanged();

    // destroy old timestamps
    fluxbox->clearMenuFilenames();

    // save submenu index so we can restore them afterwards
    vector<int> remember_sub;
    if (restore_menus) {
        FbTk::Menu *menu = m_rootmenu.get();
        while (menu) {
            int r = menu->currentSubmenu();
            if (r < 0) break;
            remember_sub.push_back(r);
            menu = menu->find(r)->submenu();
        }
    }

    rereadMenu();

    if (restore_menus) {
        // restore submenus, no timestamp changed
        FbTk::Menu *menu = m_rootmenu.get();
        for (size_t i = 0; i < remember_sub.size(); i++ ) {
            int sub = remember_sub[i];
            if (!menu || sub < 0) 
                break;
            FbTk::MenuItem *item = menu->find(sub);
            if (item != 0) {
                menu->drawSubmenu(sub);
                menu = item->submenu();
            } else
                menu = 0;

        }
    }

    // reconfigure workspaces
    for_each(m_workspaces_list.begin(),
             m_workspaces_list.end(),
             mem_fun(&Workspace::reconfigure));
    
    // reconfigure Icons
    for_each(m_icon_list.begin(),
             m_icon_list.end(),
             mem_fun(&FluxboxWindow::reconfigure));
    
    imageControl().cleanCache();
    // notify objects that the screen is reconfigured
    m_reconfigure_sig.notify();

    m_root_theme->setLoaded();

    // Reload style
    FbTk::ThemeManager::instance().load(fluxbox->getStyleFilename(),
                                        fluxbox->getStyleOverlayFilename(),
                                        m_root_theme->screenNum());

    reconfigureTabs();
}

void BScreen::reconfigureTabs() {
    Workspaces::iterator w_it = getWorkspacesList().begin();
    const Workspaces::iterator w_it_end = getWorkspacesList().end();
    for (; w_it != w_it_end; ++w_it) {
        if (!(*w_it)->windowList().empty()) {
            Workspace::Windows::iterator win_it = (*w_it)->windowList().begin();
            const Workspace::Windows::iterator win_it_end = (*w_it)->windowList().end();
            for (; win_it != win_it_end; ++win_it) {
                (*win_it)->frame().updateTabProperties();
                if (*resource.default_internal_tabs)
                    (*win_it)->frame().setTabMode(FbWinFrame::INTERNAL);
                else
                    (*win_it)->frame().setTabMode(FbWinFrame::EXTERNAL);
            }
        }
    }
    Icons::iterator icon_it = m_icon_list.begin();
    Icons::iterator icon_it_end = m_icon_list.end();
    for (; icon_it != icon_it_end; ++icon_it) {
        (*icon_it)->frame().updateTabProperties();
        if (*resource.default_internal_tabs)
            (*icon_it)->frame().setTabMode(FbWinFrame::INTERNAL);
        else
            (*icon_it)->frame().setTabMode(FbWinFrame::EXTERNAL);
    }
}


void BScreen::rereadMenu() {
    initMenu();
    m_rootmenu->reconfigure();
}


void BScreen::removeWorkspaceNames() {
    m_workspace_names.clear();
}

void BScreen::updateWorkspaceNamesAtom() {
    m_workspacenames_sig.notify();
}

void BScreen::addIcon(FluxboxWindow *w) {
    if (w == 0) 
        return;

    // make sure we have a unique list
    if (find(iconList().begin(), iconList().end(), w) != iconList().end())
        return;

    m_icon_list.push_back(w);

    // notify listeners
    m_iconlist_sig.notify();
}


void BScreen::removeIcon(FluxboxWindow *w) {
    if (w == 0)
        return;

    Icons::iterator erase_it = remove_if(iconList().begin(),
                                         iconList().end(),
                                         bind2nd(equal_to<FluxboxWindow *>(), w));
    // no need to send iconlist signal if we didn't 
    // change the iconlist
    if (erase_it != m_icon_list.end()) {
        iconList().erase(erase_it);
        m_iconlist_sig.notify();
    }
}

void BScreen::removeWindow(FluxboxWindow *win) {
#ifdef DEBUG
    cerr<<"BScreen::removeWindow("<<win<<")"<<endl;
#endif // DEBUG
    // extra precaution, if for some reason, the 
    // icon list should be out of sync
    removeIcon(win);
    // remove from workspace
    Workspace *space = getWorkspace(win->workspaceNumber());
    if (space != 0)
        space->removeWindow(win, false);
}


void BScreen::removeClient(WinClient &client) {

    focusControl().removeClient(client);

    for_each(getWorkspacesList().begin(), getWorkspacesList().end(),
             mem_fun(&Workspace::updateClientmenu));

    using namespace FbTk;

    // remove any grouping this is expecting
    Groupables::iterator erase_it = find_if(m_expecting_groups.begin(),
                                            m_expecting_groups.end(),
                                            Compose(bind2nd(equal_to<WinClient *>(), &client),
                                                    Select2nd<Groupables::value_type>()));

    if (erase_it != m_expecting_groups.end())
        m_expecting_groups.erase(erase_it);

    // the client could be on icon menu so we update it
    //!! TODO: check this with the new icon menu
    //    updateIconMenu();

}

int BScreen::addWorkspace() {

    bool save_name = getNameOfWorkspace(m_workspaces_list.size()) != "" ? false : true;
    Workspace *wkspc = new Workspace(*this, m_layermanager, 
                                     getNameOfWorkspace(m_workspaces_list.size()),
                                     m_workspaces_list.size());
    m_workspaces_list.push_back(wkspc);
    
    if (save_name)
        addWorkspaceName(wkspc->name().c_str()); //update names

    saveWorkspaces(m_workspaces_list.size());
    
    updateNetizenWorkspaceCount();	
	
    return m_workspaces_list.size();
	
}

/// removes last workspace
/// @return number of desktops left
int BScreen::removeLastWorkspace() {
    if (m_workspaces_list.size() <= 1)
        return 0;
    Workspace *wkspc = m_workspaces_list.back();

    if (m_current_workspace->workspaceID() == wkspc->workspaceID())
        changeWorkspaceID(m_current_workspace->workspaceID() - 1);

    wkspc->removeAll(wkspc->workspaceID()-1);

    Icons::iterator it = iconList().begin();
    const Icons::iterator it_end = iconList().end();
    for (; it != it_end; ++it) {
        if ((*it)->workspaceNumber() == wkspc->workspaceID())
            (*it)->setWorkspace(wkspc->workspaceID()-1);
    }
    m_clientlist_sig.notify();

    //remove last workspace
    m_workspaces_list.pop_back();		

    updateNetizenWorkspaceCount();
    saveWorkspaces(m_workspaces_list.size());
    // must be deleted after we send notify!!
    // so we dont get bad pointers somewhere
    // while processing the notify signal
    delete wkspc;

    return m_workspaces_list.size();
}


void BScreen::changeWorkspaceID(unsigned int id) {

    if (! m_current_workspace || id >= m_workspaces_list.size() ||
        id == m_current_workspace->workspaceID())
        return;

    FbTk::App::instance()->sync(false);

    FluxboxWindow *focused = FocusControl::focusedFbWindow();

    if (focused && focused->isMoving()) {
        if (doOpaqueMove())
            reassociateWindow(focused, id, true);
        // don't reassociate if not opaque moving
        focused->pauseMoving();
    }

    // reassociate all windows that are stuck to the new workspace
    Workspace *wksp = currentWorkspace();
    Workspace::Windows wins = wksp->windowList();
    Workspace::Windows::iterator it = wins.begin();
    for (; it != wins.end(); ++it) {
        if ((*it)->isStuck()) {
            reassociateWindow(*it, id, true);
        }
    }

    currentWorkspace()->hideAll(false);

    // set new workspace
    m_current_workspace = getWorkspace(id);

    // This is a little tricks to reduce flicker 
    // this way we can set focus pixmap on frame before we show it
    // and using ExposeEvent to redraw without flicker
    /*    
    WinClient *win = getLastFocusedWindow(currentWorkspaceID());
    if (win && win->fbwindow())
        win->fbwindow()->setFocusFlag(true);
    */

    currentWorkspace()->showAll();

    if (focused && (focused->isStuck() || focused->isMoving()))
        focused->setInputFocus();
    else
        FocusControl::revertFocus(*this);

    if (focused && focused->isMoving())
        focused->resumeMoving();

    updateNetizenCurrentWorkspace();
    FbTk::App::instance()->sync(false);

}


void BScreen::sendToWorkspace(unsigned int id, FluxboxWindow *win, bool changeWS) {
    if (! m_current_workspace || id >= m_workspaces_list.size())
        return;

    if (!win)
        win = FocusControl::focusedFbWindow();

    FbTk::App::instance()->sync(false);

    if (win && &win->screen() == this &&
        (! win->isStuck())) {

        // if iconified, deiconify it before we send it somewhere
        if (win->isIconic())
            win->deiconify();

        // if the window isn't on current workspace, hide it
        if (id != currentWorkspace()->workspaceID()) 
            win->withdraw(true);

        windowMenu().hide();

        reassociateWindow(win, id, true);

        // if the window is on current workspace, show it.
        if (id == currentWorkspace()->workspaceID())
            win->deiconify(false, false);

        // change workspace ?
        if (changeWS && id != currentWorkspace()->workspaceID()) {
            changeWorkspaceID(id);
            win->setInputFocus();
        }

    }

}


void BScreen::addNetizen(Window win) {
    Netizen *net = new Netizen(*this, win);
    m_netizen_list.push_back(net);

    net->sendWorkspaceCount();
    net->sendCurrentWorkspace();

    // send all windows to netizen
    Workspaces::iterator it = m_workspaces_list.begin();
    Workspaces::iterator it_end = m_workspaces_list.end();
    for (; it != it_end; ++it) {
        Workspace::Windows::iterator win_it = (*it)->windowList().begin();
        Workspace::Windows::iterator win_it_end = (*it)->windowList().end();
        for (; win_it != win_it_end; ++win_it) {
            net->sendWindowAdd((*win_it)->clientWindow(), 
                               (*it)->workspaceID());
        }
    }

    Window f = ((FocusControl::focusedWindow()) ?
		FocusControl::focusedWindow()->window() : None);
    net->sendWindowFocus(f);
}

void BScreen::removeNetizen(Window w) {
    Netizens::iterator it = m_netizen_list.begin();
    Netizens::iterator it_end = m_netizen_list.end();
    for (; it != it_end; ++it) {
        if ((*it)->window() == w) {
            Netizen *n = *it;
            delete n;
            m_netizen_list.erase(it);			
            break;
        }
    }
}


void BScreen::updateNetizenCurrentWorkspace() {
    m_currentworkspace_sig.notify();
    for_each(m_netizen_list.begin(),
             m_netizen_list.end(),
             mem_fun(&Netizen::sendCurrentWorkspace));
}


void BScreen::updateNetizenWorkspaceCount() {
    for_each(m_netizen_list.begin(),
             m_netizen_list.end(),
             mem_fun(&Netizen::sendWorkspaceCount));
    m_workspacecount_sig.notify();	
}


void BScreen::updateNetizenWindowFocus() {
    Window f = ((FocusControl::focusedWindow()) ?
                FocusControl::focusedWindow()->window() : None);
    for_each(m_netizen_list.begin(),
             m_netizen_list.end(),
             bind2nd(mem_fun(&Netizen::sendWindowFocus), f));
}


void BScreen::updateNetizenWindowAdd(Window w, unsigned long p) {

    // update the list of clients
    m_clientlist_sig.notify();
    
    // and then send the signal to listeners
    Netizens::iterator it = m_netizen_list.begin();
    Netizens::iterator it_end = m_netizen_list.end();
    for (; it != it_end; ++it) {
        (*it)->sendWindowAdd(w, p);
    }
	
}


void BScreen::updateNetizenWindowDel(Window w) {
    for_each(m_netizen_list.begin(),
             m_netizen_list.end(),
             bind2nd(mem_fun(&Netizen::sendWindowDel), w));
	
    m_clientlist_sig.notify();
}


void BScreen::updateNetizenWindowRaise(Window w) {
    for_each(m_netizen_list.begin(),
             m_netizen_list.end(),
             bind2nd(mem_fun(&Netizen::sendWindowRaise), w));
}


void BScreen::updateNetizenWindowLower(Window w) {
    for_each(m_netizen_list.begin(),
             m_netizen_list.end(),
             bind2nd(mem_fun(&Netizen::sendWindowLower), w));
}

void BScreen::updateNetizenConfigNotify(XEvent &e) {
    Netizens::iterator it = m_netizen_list.begin();
    Netizens::iterator it_end = m_netizen_list.end();
    for (; it != it_end; ++it)
        (*it)->sendConfigNotify(e);
}

bool BScreen::isKdeDockapp(Window client) const {
    //Check and see if client is KDE dock applet.
    //If so add to Slit
    bool iskdedockapp = false;
    Atom ajunk;
    int ijunk;
    unsigned long *data = 0, uljunk;
    Display *disp = FbTk::App::instance()->display();
    // Check if KDE v2.x dock applet
    if (XGetWindowProperty(disp, client,
                           XInternAtom(FbTk::App::instance()->display(), 
                                       "_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR", False),
                           0l, 1l, False,
                           XA_WINDOW, &ajunk, &ijunk, &uljunk,
                           &uljunk, (unsigned char **) &data) == Success) {
					
        if (data)
            iskdedockapp = true;
        XFree((void *) data);
        data = 0;
    }

    // Check if KDE v1.x dock applet
    if (!iskdedockapp) {
        Atom kwm1 = XInternAtom(FbTk::App::instance()->display(), 
                                "KWM_DOCKWINDOW", False);
        if (XGetWindowProperty(disp, client,
                               kwm1, 0l, 1l, False,
                               kwm1, &ajunk, &ijunk, &uljunk,
                               &uljunk, (unsigned char **) &data) == Success && data) {
            iskdedockapp = (data && data[0] != 0);
            XFree((void *) data);
            data = 0;
        }
    } 

    return iskdedockapp;
}

bool BScreen::addKdeDockapp(Window client) {

    XSelectInput(FbTk::App::instance()->display(), client, StructureNotifyMask);
    char intbuff[16];    
    sprintf(intbuff, "%d", screenNumber());
    std::string atom_name("_NET_SYSTEM_TRAY_S");
    atom_name += intbuff; // append number
    // find the right atomhandler that has the name: _NET_SYSTEM_TRAY_S<num>
    AtomHandler *handler = Fluxbox::instance()->getAtomHandler(atom_name);
    FbTk::EventHandler *evh  = 0;
    FbTk::EventManager *evm = FbTk::EventManager::instance();
    if (handler == 0) {
#ifdef SLIT
        if (slit() != 0)
            slit()->addClient(client);
        else
#endif // SLIT
            return false;
    } else {
        // this handler is a special case 
        // so we call setupClient in it
        WinClient winclient(client, *this);
        handler->setupClient(winclient);
        // we need to save old handler and re-add it later
        evh = evm->find(client);
    }

    if (evh != 0) // re-add handler
        evm->add(*evh, client);

    return true;
}

FluxboxWindow *BScreen::createWindow(Window client) {
    FbTk::App::instance()->sync(false);


    if (isKdeDockapp(client) && addKdeDockapp(client)) {
        return 0; // dont create a FluxboxWindow for this one
    }

    WinClient *winclient = new WinClient(client, *this);

    if (winclient->initial_state == WithdrawnState) {
        delete winclient;
#ifdef SLIT
        if (slit())
            slit()->addClient(client);
#endif // SLIT
        return 0;
    }

    // check if it should be grouped with something else
    FluxboxWindow *win;
    WinClient *other;
    if ((other = findGroupLeft(*winclient)) && (win = other->fbwindow())) {
        win->attachClient(*winclient);
        Fluxbox::instance()->attachSignals(*winclient);
    } else {

        Fluxbox::instance()->attachSignals(*winclient);
        if (winclient->fbwindow()) // may have been set in an atomhandler
            win = winclient->fbwindow();
        else {
            win = new FluxboxWindow(*winclient,
                                    winFrameTheme(),
                                    *layerManager().getLayer(Layer::NORMAL));
            
            if (!win->isManaged()) {
                delete win;
                return 0;
            } 
        }
    }
                
    // always put on end of focused list, if it gets focused it'll get pushed up
    // there is only the one win client at this stage
    focusControl().addFocusBack(*winclient);

    // we also need to check if another window expects this window to the left
    // and if so, then join it.
    if ((other = findGroupRight(*winclient)) && other->fbwindow() != win)
        win->attachClient(*other);
    else if (other)
        win->moveClientRightOf(*other, *winclient);

    m_clientlist_sig.notify();

    FbTk::App::instance()->sync(false);
    return win;
}


FluxboxWindow *BScreen::createWindow(WinClient &client) {

    if (isKdeDockapp(client.window()) && addKdeDockapp(client.window())) {
        return 0;
    }

    FluxboxWindow *win = new FluxboxWindow(client,
                                           winFrameTheme(),
                                           *layerManager().getLayer(Layer::NORMAL));

#ifdef SLIT
    if (win->initialState() == WithdrawnState && slit() != 0) {
        slit()->addClient(win->clientWindow());
    }
#endif // SLIT
                 

    if (!win->isManaged()) {
        delete win;
        return 0;
    }

    // can't setInputFocus yet and mapNotifyEvent doesn't happen for the client
    if (focusControl().focusNew() || FocusControl::focusedWindow() == &client)
        FocusControl::setFocusedWindow(&client);

    m_clientlist_sig.notify();

    return win;
}

Strut *BScreen::requestStrut(int head, int left, int right, int top, int bottom) {
    if (head > numHeads() && head != 1) {
        // head does not exist (if head == 1, then numHeads() == 0, 
        // which means no xinerama, but there's a head after all
        head = numHeads();
    }

    int begin = head-1;
    int end   = head;

    if (head == 0) { // all heads (or no xinerama)
        begin = 0;
        end = (numHeads() ? numHeads() : 1);
    }

    Strut* next = 0;
    for (int i = begin; i != end; i++) {
        next = m_head_areas[i].requestStrut(i+1, left, right, top, bottom, next);
    }

    return next;
}

void BScreen::clearStrut(Strut *str) {
    if (str->next()) 
        clearStrut(str->next());
    int head = str->head() ? str->head() - 1 : 0;
    m_head_areas[head].clearStrut(str);
    // str is invalid now
}

void BScreen::updateAvailableWorkspaceArea() {
    size_t n = (numHeads() ? numHeads() : 1);
    bool updated = false;

    for (size_t i = 0; i < n; i++) {
        updated = m_head_areas[i].updateAvailableWorkspaceArea() || updated;
    }

    if (updated)
        m_workspace_area_sig.notify();
}

void BScreen::addWorkspaceName(const char *name) {
    m_workspace_names.push_back(FbTk::FbStringUtil::LocaleStrToFb(name));
}


string BScreen::getNameOfWorkspace(unsigned int workspace) const {
    if (workspace < m_workspace_names.size())
        return m_workspace_names[workspace];
    else
        return "";
}

void BScreen::reassociateWindow(FluxboxWindow *w, unsigned int wkspc_id, 
                                bool ignore_sticky) {
    if (w == 0)
        return;

    if (wkspc_id >= numberOfWorkspaces())
        wkspc_id = currentWorkspace()->workspaceID();

    if (!w->isIconic() && w->workspaceNumber() == wkspc_id)
        return;


    if (w->isIconic()) {
        removeIcon(w);
        getWorkspace(wkspc_id)->addWindow(*w);
        // client list need to notify now even though
        // we didn't remove/add any window,
        // so listeners that uses the client list to 
        // show whats on current/other workspace
        // gets updated
        m_clientlist_sig.notify(); 
    } else if (ignore_sticky || ! w->isStuck()) {
        // fresh windows have workspaceNumber == -1, which leads to
        // an invalid workspace (unsigned int)
        if (getWorkspace(w->workspaceNumber()))
            getWorkspace(w->workspaceNumber())->removeWindow(w, true);
        getWorkspace(wkspc_id)->addWindow(*w);
        // see comment above
        m_clientlist_sig.notify();
    }
}

void BScreen::initMenus() {
    m_workspacemenu.reset(MenuCreator::createMenuType("workspacemenu", screenNumber()));
    m_windowmenu.reset(MenuCreator::createMenuType("windowmenu", screenNumber()));
    m_windowmenu->setInternalMenu();
    initMenu();
}


void BScreen::initMenu() {
	
    if (m_rootmenu.get()) {
        // since all menus in root is submenus in m_rootmenu
        // just remove every item in m_rootmenu and then clear m_rootmenu_list
        while (m_rootmenu->numberOfItems())
            m_rootmenu->remove(0); 
        m_rootmenu_list.clear();

    } else
        m_rootmenu.reset(createMenu(""));

    Fluxbox * const fb = Fluxbox::instance();
    if (!fb->getMenuFilename().empty()) {
        m_rootmenu.reset(MenuCreator::createFromFile(fb->getMenuFilename(),
                                                     screenNumber(), true));

    }

    if (m_rootmenu.get() == 0) {
        _FB_USES_NLS;
        m_rootmenu.reset(createMenu(_FB_XTEXT(Menu, DefaultRootMenu, "Fluxbox default menu", "Title of fallback root menu")));
        FbTk::RefCount<FbTk::Command> restart_fb(CommandParser::instance().parseLine("restart"));
        FbTk::RefCount<FbTk::Command> exit_fb(CommandParser::instance().parseLine("exit"));
        FbTk::RefCount<FbTk::Command> execute_xterm(CommandParser::instance().parseLine("exec xterm"));
        m_rootmenu->setInternalMenu();
        m_rootmenu->insert("xterm", execute_xterm);
        m_rootmenu->insert(_FB_XTEXT(Menu, Restart, "Restart", "Restart command"),
                           restart_fb);
        m_rootmenu->insert(_FB_XTEXT(Menu, Exit, "Exit", "Exit command"),
                           exit_fb);
    }

    m_rootmenu->updateMenu();
}


void BScreen::addConfigMenu(const FbTk::FbString &label, FbTk::Menu &menu) {
    m_configmenu_list.push_back(std::make_pair(label, &menu));
    setupConfigmenu(*m_configmenu.get());
}

void BScreen::removeConfigMenu(FbTk::Menu &menu) {
    Configmenus::iterator erase_it = find_if(m_configmenu_list.begin(),
                                             m_configmenu_list.end(),
                                             FbTk::Compose(bind2nd(equal_to<FbTk::Menu *>(), &menu),
                                                           FbTk::Select2nd<Configmenus::value_type>()));
    if (erase_it != m_configmenu_list.end())
        m_configmenu_list.erase(erase_it);

    setupConfigmenu(*m_configmenu.get());

}    


void BScreen::addManagedResource(FbTk::Resource_base *resource) {
    m_managed_resources.push_back(resource);
}

void BScreen::setupConfigmenu(FbTk::Menu &menu) {
    _FB_USES_NLS;

    menu.removeAll();

    FbTk::MacroCommand *s_a_reconf_macro = new FbTk::MacroCommand();
    FbTk::MacroCommand *s_a_reconftabs_macro = new FbTk::MacroCommand();
    FbTk::RefCount<FbTk::Command> saverc_cmd(new FbTk::SimpleCommand<Fluxbox>(
                                                 *Fluxbox::instance(), 
                                                 &Fluxbox::save_rc));
    FbTk::RefCount<FbTk::Command> reconf_cmd(CommandParser::instance().parseLine("reconfigure"));

    FbTk::RefCount<FbTk::Command> reconftabs_cmd(new FbTk::SimpleCommand<BScreen>(
                                                 *this, 
                                                 &BScreen::reconfigureTabs));
    s_a_reconf_macro->add(saverc_cmd);
    s_a_reconf_macro->add(reconf_cmd);
    s_a_reconftabs_macro->add(saverc_cmd);
    s_a_reconftabs_macro->add(reconftabs_cmd);
    FbTk::RefCount<FbTk::Command> save_and_reconfigure(s_a_reconf_macro);
    FbTk::RefCount<FbTk::Command> save_and_reconftabs(s_a_reconftabs_macro);
    // create focus menu
    // we don't set this to internal menu so will 
    // be deleted toghether with the parent
    FbTk::FbString focusmenu_label = _FB_XTEXT(Configmenu, FocusModel,
                                          "Focus Model", 
                                          "Method used to give focus to windows");
    FbTk::Menu *focus_menu = createMenu(focusmenu_label);

#define _BOOLITEM(m,a, b, c, d, e, f) (m).insert(new BoolMenuItem(_FB_XTEXT(a, b, c, d), e, f))
                             

#define _FOCUSITEM(a, b, c, d, e) \
    focus_menu->insert(new FocusModelMenuItem(_FB_XTEXT(a, b, c, d), focusControl(), \
                                              e, save_and_reconfigure))

    _FOCUSITEM(Configmenu, ClickFocus,
               "Click To Focus", "Click to focus",
               FocusControl::CLICKFOCUS);
    _FOCUSITEM(Configmenu, MouseFocus,
               "Mouse Focus", "Mouse Focus",
               FocusControl::MOUSEFOCUS);
#undef _FOCUSITEM

    focus_menu->insert(new TabFocusModelMenuItem("ClickTabFocus", focusControl(), 
                                                 FocusControl::CLICKTABFOCUS, 
                                                 save_and_reconfigure));
    focus_menu->insert(new TabFocusModelMenuItem("MouseTabFocus", focusControl(), 
                                                 FocusControl::MOUSETABFOCUS, 
                                                 save_and_reconfigure));
    

    focus_menu->insert(new BoolMenuItem(_FB_XTEXT(Configmenu, 
                                                AutoRaise,
                                                "Auto Raise",
                                                "Auto Raise windows on sloppy"),
                                        *resource.auto_raise,
                                        save_and_reconfigure));

    focus_menu->updateMenu();

    menu.insert(focusmenu_label, focus_menu);

    // END focus menu

    // BEGIN tab menu

    FbTk::FbString tabmenu_label = _FB_XTEXT(Configmenu, TabMenu,
                                        "Tab Options", 
                                        "heading for tab-related options");
    FbTk::Menu *tab_menu = createMenu(tabmenu_label);
    FbTk::FbString tabplacement_label = _FB_XTEXT(Menu, Placement, "Placement", "Title of Placement menu");
    FbTk::Menu *tabplacement_menu = createToggleMenu(tabplacement_label);

    tab_menu->insert(tabplacement_label, tabplacement_menu);

    _BOOLITEM(*tab_menu,Configmenu, TabsInTitlebar,
              "Tabs in Titlebar", "Tabs in Titlebar",
              *resource.default_internal_tabs, save_and_reconftabs);
    tab_menu->insert(new BoolMenuItem(_FB_XTEXT(Common, MaximizeOver,
              "Maximize Over", "Maximize over this thing when maximizing"),
              *resource.max_over_tabs, save_and_reconfigure));

    FbTk::MenuItem *tab_width_item =
            new IntResMenuItem(_FB_XTEXT(Configmenu, ExternalTabWidth, 
                                       "External Tab Width",
                                       "Width of external-style tabs"),
                               resource.tab_width, 10, 3000, /* silly number */
                               *tab_menu);
    tab_width_item->setCommand(save_and_reconftabs);
    tab_menu->insert(tab_width_item);


    typedef pair<FbTk::FbString, FbWinFrame::TabPlacement> PlacementP;
    typedef list<PlacementP> Placements;
    Placements place_menu;

    // menu is 2 wide, 2 down
    place_menu.push_back(PlacementP(_FB_XTEXT(Align, TopLeft, "Top Left", "Top Left"), FbWinFrame::TOPLEFT));
    place_menu.push_back(PlacementP(_FB_XTEXT(Align, LeftTop, "Left Top", "Left Top"), FbWinFrame::LEFTTOP));
    place_menu.push_back(PlacementP(_FB_XTEXT(Align, LeftBottom, "Left Bottom", "Left Bottom"), FbWinFrame::LEFTBOTTOM));
    place_menu.push_back(PlacementP(_FB_XTEXT(Align, BottomLeft, "Bottom Left", "Bottom Left"), FbWinFrame::BOTTOMLEFT));
    place_menu.push_back(PlacementP(_FB_XTEXT(Align, TopRight, "Top Right", "Top Right"), FbWinFrame::TOPRIGHT));
    place_menu.push_back(PlacementP(_FB_XTEXT(Align, RightTop, "Right Top", "Right Top"), FbWinFrame::RIGHTTOP));
    place_menu.push_back(PlacementP(_FB_XTEXT(Align, RightBottom, "Right Bottom", "Right Bottom"), FbWinFrame::RIGHTBOTTOM));
    place_menu.push_back(PlacementP(_FB_XTEXT(Align, BottomRight, "Bottom Right", "Bottom Right"), FbWinFrame::BOTTOMRIGHT));

    tabplacement_menu->setMinimumSublevels(2);
    // create items in sub menu
    size_t i=0;
    while (!place_menu.empty()) {
        i++;
        FbTk::FbString &str = place_menu.front().first;
        FbWinFrame::TabPlacement placement = place_menu.front().second;

        tabplacement_menu->insert(new TabPlacementMenuItem(str, *this, placement, save_and_reconftabs));
        place_menu.pop_front();
    }
    tabplacement_menu->updateMenu();

    menu.insert(tabmenu_label, tab_menu);

    Configmenus::iterator it = m_configmenu_list.begin();
    Configmenus::iterator it_end = m_configmenu_list.end();
    for (; it != it_end; ++it)
        menu.insert(it->first, it->second);

    _BOOLITEM(menu, Configmenu, ImageDithering,
              "Image Dithering", "Image Dithering",
              *resource.image_dither, save_and_reconfigure);
    _BOOLITEM(menu, Configmenu, OpaqueMove,
              "Opaque Window Moving", 
              "Window Moving with whole window visible (as opposed to outline moving)",
              *resource.opaque_move, saverc_cmd);
    _BOOLITEM(menu, Configmenu, FullMax,
              "Full Maximization", "Maximise over slit, toolbar, etc",
              *resource.full_max, saverc_cmd);
    try {
        _BOOLITEM(menu, Configmenu, FocusNew,
                  "Focus New Windows", "Focus newly created windows",
                  *m_resource_manager.getResource<bool>(name() + ".focusNewWindows"),
                  saverc_cmd);
    } catch (FbTk::ResourceException e) {
        cerr<<e.what()<<endl;
    }

    try {
        _BOOLITEM(menu, Configmenu, FocusLast,
                  "Focus Last Window on Workspace", "Focus Last Window on Workspace",
                  *resourceManager().getResource<bool>(name() + ".focusLastWindow"),
                  saverc_cmd);
    } catch (FbTk::ResourceException e) {
        cerr<<e.what()<<endl;
    }

    _BOOLITEM(menu, Configmenu, WorkspaceWarping,
              "Workspace Warping", 
              "Workspace Warping - dragging windows to the edge and onto the next workspace",
              *resource.workspace_warping, saverc_cmd);
    _BOOLITEM(menu, Configmenu, DesktopWheeling,
              "Desktop MouseWheel Switching", "Workspace switching using mouse wheel",
              *resource.desktop_wheeling, saverc_cmd);
    _BOOLITEM(menu, Configmenu, DecorateTransient,
              "Decorate Transient Windows", "Decorate Transient Windows",
              *resource.decorate_transient, saverc_cmd);
    _BOOLITEM(menu, Configmenu, ClickRaises,
              "Click Raises", "Click Raises", 
              *resource.click_raises, saverc_cmd);

#ifdef HAVE_XRENDER
    if (FbTk::Transparent::haveRender() ||
        FbTk::Transparent::haveComposite()) {

        FbTk::FbString alphamenu_label = _FB_XTEXT(Configmenu, Transparency,
                                          "Transparency", 
                                           "Menu containing various transparency options");
        FbTk::Menu *alpha_menu = createMenu(alphamenu_label);

        if (FbTk::Transparent::haveComposite(true)) {
            alpha_menu->insert(new BoolMenuItem(_FB_XTEXT(Configmenu, ForcePseudoTrans,
                               "Force Pseudo-Transparency", 
                               "When composite is available, still use old pseudo-transparency"),
                    Fluxbox::instance()->getPseudoTrans(), save_and_reconfigure));
        }

        FbTk::MenuItem *focused_alpha_item =
            new IntResMenuItem(_FB_XTEXT(Configmenu, FocusedAlpha, 
                                       "Focused Window Alpha",
                                       "Transparency level of the focused window"),
                    resource.focused_alpha, 0, 255, *alpha_menu);
        focused_alpha_item->setCommand(saverc_cmd);
        alpha_menu->insert(focused_alpha_item);

        FbTk::MenuItem *unfocused_alpha_item = 
            new IntResMenuItem(_FB_XTEXT(Configmenu, 
                                       UnfocusedAlpha, 
                                       "Unfocused Window Alpha", 
                                       "Transparency level of unfocused windows"),

                    resource.unfocused_alpha, 0, 255, *alpha_menu);
        unfocused_alpha_item->setCommand(saverc_cmd);
        alpha_menu->insert(unfocused_alpha_item);

        FbTk::MenuItem *menu_alpha_item = 
            new IntResMenuItem(_FB_XTEXT(Configmenu, MenuAlpha, 
                                       "Menu Alpha", "Transparency level of menu"),
                    resource.menu_alpha, 0, 255, *alpha_menu);
        menu_alpha_item->setCommand(saverc_cmd);
        alpha_menu->insert(menu_alpha_item);

        alpha_menu->updateMenu();
        menu.insert(alphamenu_label, alpha_menu);
    }
#endif // HAVE_XRENDER
#undef _BOOLITEM

    // finaly update menu 
    menu.updateMenu();
}


void BScreen::shutdown() {
    rootWindow().setEventMask(NoEventMask);
    FbTk::App::instance()->sync(false);
    m_shutdown = true;
    m_focus_control->shutdown();
    for_each(m_workspaces_list.begin(),
             m_workspaces_list.end(),
             mem_fun(&Workspace::shutdown));
}


void BScreen::showPosition(int x, int y) {
        if (!doShowWindowPos())
        return;

    if (! pos_visible) {
        if (hasXinerama()) {
            unsigned int head = getCurrHead();

            m_pos_window.move(getHeadX(head) + (getHeadWidth(head) - m_pos_window.width()) / 2,
                              getHeadY(head) + (getHeadHeight(head) - m_pos_window.height()) / 2);
                            
        } else {
            m_pos_window.move((width() - m_pos_window.width()) / 2, 
                              (height() - m_pos_window.height()) / 2);
        }

        m_pos_window.show();
        m_pos_window.raise();

        pos_visible = true;
    }

    char label[256];
    sprintf(label, "X:%5d x Y:%5d", x, y);

    m_pos_window.clear();

    winFrameTheme().font().drawText(m_pos_window,
                                    screenNumber(),
                                    winFrameTheme().labelTextFocusGC(),
                                    label, strlen(label),
                                    winFrameTheme().bevelWidth(), 
                                    winFrameTheme().bevelWidth() + 
                                    winFrameTheme().font().ascent());
		
}


void BScreen::hidePosition() {
    if (pos_visible) {
        m_pos_window.hide();
        pos_visible = false;
    }
}

// can be negative when base_width/height > min_width/height
void BScreen::showGeometry(int gx, int gy) {
    if (!doShowWindowPos())
        return;

    if (! geom_visible) {
        if (hasXinerama()) {
            unsigned int head = getCurrHead();

            m_geom_window.move(getHeadX(head) + (getHeadWidth(head) - m_geom_window.width()) / 2,
                               getHeadY(head) + (getHeadHeight(head) - m_geom_window.height()) / 2);
        } else {
            m_geom_window.move((width() - m_geom_window.width()) / 2, 
                               (height() - m_geom_window.height()) / 2);

        }
        m_geom_window.show();
        m_geom_window.raise();

        geom_visible = true;
    }
	
    char label[256];
    _FB_USES_NLS;

    sprintf(label,
            _FB_XTEXT(Screen, GeometryFormat,
                    "W: %4d x H: %4d",
                    "Format for width and height window, %4d for width, and %4d for height").c_str(),
            gx, gy);

    m_geom_window.clear();

    //!! TODO: geom window again?! repeated
    winFrameTheme().font().drawText(m_geom_window,
                                    screenNumber(),
                                    winFrameTheme().labelTextFocusGC(),
                                    label, strlen(label),
                                    winFrameTheme().bevelWidth(), 
                                    winFrameTheme().bevelWidth() + 
                                    winFrameTheme().font().ascent());	
}


void BScreen::hideGeometry() {
    if (geom_visible) {
        m_geom_window.hide();
        geom_visible = false;
    }
}

void BScreen::setLayer(FbTk::XLayerItem &item, int layernum) {
    m_layermanager.moveToLayer(item, layernum);
}


/**
 Goes to the workspace "right" of the current
*/
void BScreen::nextWorkspace(const int delta) {
    changeWorkspaceID( (currentWorkspaceID() + delta) % numberOfWorkspaces());
}

/**
 Goes to the workspace "left" of the current
*/
void BScreen::prevWorkspace(const int delta) {
    changeWorkspaceID( (static_cast<signed>(numberOfWorkspaces()) + currentWorkspaceID() - (delta % numberOfWorkspaces())) % numberOfWorkspaces());
}

/**
 Goes to the workspace "right" of the current
*/
void BScreen::rightWorkspace(const int delta) {
    if (currentWorkspaceID()+delta < numberOfWorkspaces())
        changeWorkspaceID(currentWorkspaceID()+delta);
}

/**
 Goes to the workspace "left" of the current
*/
void BScreen::leftWorkspace(const int delta) {
    if (currentWorkspaceID() >= static_cast<unsigned int>(delta))
        changeWorkspaceID(currentWorkspaceID()-delta);
}


void BScreen::renderGeomWindow() {

    char label[256];
    _FB_USES_NLS;

    sprintf(label,
            _FB_XTEXT(Screen, GeometryFormat,
            "W: %04d x H: %04d", "Representative maximum sized text for width and height dialog").c_str(),
            0, 0);

    int geom_h = winFrameTheme().font().height() + winFrameTheme().bevelWidth()*2;
    int geom_w = winFrameTheme().font().textWidth(label, strlen(label)) + winFrameTheme().bevelWidth()*2;
    m_geom_window.resize(geom_w, geom_h);

    m_geom_window.setBorderWidth(winFrameTheme().border().width());
    m_geom_window.setBorderColor(winFrameTheme().border().color());


    Pixmap tmp = geom_pixmap;

    if (winFrameTheme().labelFocusTexture().type() & FbTk::Texture::PARENTRELATIVE) {
        if (!winFrameTheme().titleFocusTexture().usePixmap()) {
            geom_pixmap = None;
            m_geom_window.setBackgroundColor(winFrameTheme().titleFocusTexture().color());
        } else {
            geom_pixmap = imageControl().renderImage(m_geom_window.width(), m_geom_window.height(),
                                                     winFrameTheme().titleFocusTexture());
            m_geom_window.setBackgroundPixmap(geom_pixmap);
        }
    } else {
        if (!winFrameTheme().labelFocusTexture().usePixmap()) {
            geom_pixmap = None;
            m_geom_window.setBackgroundColor(winFrameTheme().labelFocusTexture().color());
        } else {
            geom_pixmap = imageControl().renderImage(m_geom_window.width(), m_geom_window.height(),
                                                     winFrameTheme().labelFocusTexture());
            m_geom_window.setBackgroundPixmap(geom_pixmap);
        }
    }

    if (tmp)
        imageControl().removeImage(tmp);

}


void BScreen::renderPosWindow() {

    int pos_h = winFrameTheme().font().height() + winFrameTheme().bevelWidth()*2;
    int pos_w = winFrameTheme().font().textWidth("0:00000 x 0:00000", 17) + winFrameTheme().bevelWidth()*2;
    m_pos_window.resize(pos_w, pos_h);

    m_pos_window.setBorderWidth(winFrameTheme().border().width());
    m_pos_window.setBorderColor(winFrameTheme().border().color());


    Pixmap tmp = pos_pixmap;

    if (winFrameTheme().labelFocusTexture().type() & FbTk::Texture::PARENTRELATIVE) {
        if (!winFrameTheme().titleFocusTexture().usePixmap()) {
            pos_pixmap = None;
            m_pos_window.setBackgroundColor(winFrameTheme().titleFocusTexture().color());
        } else {
            pos_pixmap = imageControl().renderImage(m_pos_window.width(), m_pos_window.height(),
                                                    winFrameTheme().titleFocusTexture());
            m_pos_window.setBackgroundPixmap(pos_pixmap);
        }
    } else {
        if (!winFrameTheme().labelFocusTexture().usePixmap()) {
            pos_pixmap = None;
            m_pos_window.setBackgroundColor(winFrameTheme().labelFocusTexture().color());
        } else {
            pos_pixmap = imageControl().renderImage(m_pos_window.width(), m_pos_window.height(),
                                                     winFrameTheme().labelFocusTexture());
            m_pos_window.setBackgroundPixmap(pos_pixmap);
        }
    }

    if (tmp)
        imageControl().removeImage(tmp);

}




/**
   Called when a set of watched modifiers has been released
*/
void BScreen::notifyReleasedKeys(XKeyEvent &ke) {
    focusControl().stopCyclingFocus();
}

void BScreen::updateSize() {
    // force update geometry
    rootWindow().updateGeometry();

    // reset background
    m_root_theme->reconfigTheme();

    // send resize notify
    m_resize_sig.notify();
}


/**
 * Find the winclient to this window's left
 * So, we check the leftgroup hint, and see if we know any windows
 */
WinClient *BScreen::findGroupLeft(WinClient &winclient) {
    Window w = winclient.getGroupLeftWindow();
    if (w == None)
        return 0;

    WinClient *have_client = Fluxbox::instance()->searchWindow(w);

    if (!have_client) {
        // not found, add it to expecting
        m_expecting_groups[w] = &winclient;
    } else if (&have_client->screen() != &winclient.screen())
        // something is not consistent
        return 0;

    return have_client;
}

WinClient *BScreen::findGroupRight(WinClient &winclient) {
    Groupables::iterator it = m_expecting_groups.find(winclient.window());
    if (it == m_expecting_groups.end())
        return 0;

    // yay, this'll do.
    WinClient *other = it->second;
    m_expecting_groups.erase(it); // don't expect it anymore

    // forget about it if it isn't the left-most client in the group, plus
    // it must have the atom set on it (i.e. previously encountered by fluxbox)
    // for us to check our expecting
    if (!winclient.hasGroupLeftWindow() ||
        other->getGroupLeftWindow() != None)
        return 0;

    return other;
}

void BScreen::initXinerama() {
#ifdef XINERAMA
    Display *display = FbTk::App::instance()->display();

    if (!XineramaIsActive(display)) {
#ifdef DEBUG
        cerr<<"BScreen::initXinerama(): dont have Xinerama"<<endl;
#endif // DEBUG    
        m_xinerama_avail = false;
        m_xinerama_headinfo = 0;
        m_xinerama_num_heads = 0;
        return;
    }
#ifdef DEBUG
    cerr<<"BScreen::initXinerama(): have Xinerama"<<endl;
#endif // DEBUG    
    m_xinerama_avail = true;

    XineramaScreenInfo *screen_info;
    int number;
    screen_info = XineramaQueryScreens(display, &number);
    m_xinerama_headinfo = new XineramaHeadInfo[number];
    m_xinerama_num_heads = number;
    for (int i=0; i < number; i++) {
        m_xinerama_headinfo[i].x = screen_info[i].x_org;
        m_xinerama_headinfo[i].y = screen_info[i].y_org;
        m_xinerama_headinfo[i].width = screen_info[i].width;
        m_xinerama_headinfo[i].height = screen_info[i].height;
    }
    XFree(screen_info);
#ifdef DEBUG
    cerr<<"BScreen::initXinerama(): number of heads ="<<number<<endl;
#endif // DEBUG

#else // XINERAMA
    // no xinerama
    m_xinerama_avail = false;
    m_xinerama_num_heads = 0;
#endif // XINERAMA

}

int BScreen::getHead(int x, int y) const {
    if (!hasXinerama()) return 0;
#ifdef XINERAMA

    for (int i=0; i < m_xinerama_num_heads; i++) {
        if (x >= m_xinerama_headinfo[i].x &&
            x < (m_xinerama_headinfo[i].x + m_xinerama_headinfo[i].width) &&
            y >= m_xinerama_headinfo[i].y &&
            y < (m_xinerama_headinfo[i].y + m_xinerama_headinfo[i].height)) {
            return i+1;
        }
    }

#endif // XINERAMA
    return 0;
}

int BScreen::getHead(FbTk::FbWindow &win) const {
    if (hasXinerama())
        return getHead(win.x() + win.width()/2, win.y() + win.height()/2);
    else 
        return 0;
}


int BScreen::getCurrHead() const {
    if (!hasXinerama()) return 0;
    int root_x = 0, root_y = 0;
#ifdef XINERAMA
    int ignore_i;
    unsigned int ignore_ui;

    Window ignore_w;

    XQueryPointer(FbTk::App::instance()->display(),
                  rootWindow().window(), &ignore_w, 
                  &ignore_w, &root_x, &root_y,
                  &ignore_i, &ignore_i, &ignore_ui);
#endif // XINERAMA
    return getHead(root_x, root_y);
}

int BScreen::getHeadX(int head) const {
#ifdef XINERAMA
    if (head == 0 || head > m_xinerama_num_heads) return 0;
    return m_xinerama_headinfo[head-1].x;
#else
    return 0;
#endif // XINERAMA
}

int BScreen::getHeadY(int head) const {
#ifdef XINERAMA
    if (head == 0 || head > m_xinerama_num_heads) return 0;
    return m_xinerama_headinfo[head-1].y;
#else
    return 0;
#endif // XINERAMA
}

int BScreen::getHeadWidth(int head) const {
#ifdef XINERAMA
    if (head == 0 || head > m_xinerama_num_heads) return width();
    return m_xinerama_headinfo[head-1].width;
#else
    return width();
#endif // XINERAMA
}

int BScreen::getHeadHeight(int head) const {
#ifdef XINERAMA
    if (head == 0 || head > m_xinerama_num_heads) return height();
    return m_xinerama_headinfo[head-1].height;
#else
    return height();
#endif // XINERAMA
}

pair<int,int> BScreen::clampToHead(int head, int x, int y, int w, int h) const {

    // if there are multiple heads, head=0 is not valid
    // a better way would be to search the closest head
    if (head == 0 && numHeads() != 0)
	head = 1;
    
    int hx = getHeadX(head);
    int hy = getHeadY(head);
    int hw = getHeadWidth(head);
    int hh = getHeadHeight(head);
 
    if (x + w > hx + hw)
        x = hx + hw - w;
    if (y + h > hy + hh)
        y = hy + hh - h;
 
    if (x < hx)
        x = hx;
    if (y < hy)
       y = hy;

    return make_pair(x,y);
}

// TODO: when toolbar gets its resources moved into Toolbar.hh/cc, then
// this can be gone and a consistent interface for the two used
// on the actual objects

template<>
void BScreen::setOnHead<FluxboxWindow>(FluxboxWindow& win, int head) {
    if (head > 0 && head <= numHeads()) {
        int current_head = getHead(win.fbWindow());
        win.move(getHeadX(head) + win.frame().x() - getHeadX(current_head),
                 getHeadY(head) + win.frame().y() - getHeadY(current_head));
    }
}

