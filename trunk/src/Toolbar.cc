// Toolbar.cc for Fluxbox
// Copyright (c) 2002 - 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
//
// Toolbar.cc for Blackbox - an X11 Window manager
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// $Id: Toolbar.cc,v 1.106 2003/08/11 20:19:16 fluxgen Exp $

#include "Toolbar.hh"

#include "Container.hh"

#include "TextButton.hh"
#include "IconButton.hh"
#include "IconButtonTheme.hh"
#include "IconbarTheme.hh"
// tools
#include "IconbarTool.hh"
#include "WorkspaceNameTool.hh"
#include "ClockTool.hh"
 
#include "I18n.hh"
#include "fluxbox.hh"
#include "Screen.hh"
#include "Window.hh"
#include "WinClient.hh"
#include "Workspace.hh"
#include "ImageControl.hh"
#include "ToolbarTheme.hh"
#include "EventManager.hh"
#include "Text.hh"
#include "SimpleCommand.hh"
#include "IntResMenuItem.hh"
#include "MacroCommand.hh"
#include "RootTheme.hh"
#include "BoolMenuItem.hh"
#include "FbWinFrameTheme.hh"
#include "Xinerama.hh"
#include "Strut.hh"


// use GNU extensions
#ifndef	 _GNU_SOURCE
#define	 _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "Shape.hh"

#include <X11/Xutil.h>
#include <X11/keysym.h>

#include <cstring>
#include <cstdio>

#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else // !TIME_WITH_SYS_TIME
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else // !HAVE_SYS_TIME_H
#include <time.h>
#endif // HAVE_SYS_TIME_H
#endif // TIME_WITH_SYS_TIME


#include <iostream>

using namespace std;

template<>
void FbTk::Resource<Toolbar::Placement>::
setFromString(const char *strval) {
    if (strcasecmp(strval, "TopLeft")==0)
        m_value = Toolbar::TOPLEFT;
    else if (strcasecmp(strval, "BottomLeft")==0)
        m_value = Toolbar::BOTTOMLEFT;
    else if (strcasecmp(strval, "TopCenter")==0)
        m_value = Toolbar::TOPCENTER;
    else if (strcasecmp(strval, "BottomCenter")==0)
        m_value = Toolbar::BOTTOMCENTER;
    else if (strcasecmp(strval, "TopRight")==0)
        m_value = Toolbar::TOPRIGHT;
    else if (strcasecmp(strval, "BottomRight")==0)
        m_value = Toolbar::BOTTOMRIGHT;
    else if (strcasecmp(strval, "LeftTop") == 0)
        m_value = Toolbar::LEFTTOP;
    else if (strcasecmp(strval, "LeftCenter") == 0)
        m_value = Toolbar::LEFTCENTER;
    else if (strcasecmp(strval, "LeftBottom") == 0)
        m_value = Toolbar::LEFTBOTTOM;
    else if (strcasecmp(strval, "RightTop") == 0)
        m_value = Toolbar::RIGHTTOP;
    else if (strcasecmp(strval, "RightCenter") == 0)
        m_value = Toolbar::RIGHTCENTER;
    else if (strcasecmp(strval, "RightBottom") == 0)
        m_value = Toolbar::RIGHTBOTTOM;
    else
        setDefaultValue();
}

string FbTk::Resource<Toolbar::Placement>::
getString() {
    switch (m_value) {
    case Toolbar::TOPLEFT:
        return string("TopLeft");
        break;
    case Toolbar::BOTTOMLEFT:
        return string("BottomLeft");
        break;
    case Toolbar::TOPCENTER:
        return string("TopCenter");
        break;			
    case Toolbar::BOTTOMCENTER:
        return string("BottomCenter");
        break;
    case Toolbar::TOPRIGHT:
        return string("TopRight");
        break;
    case Toolbar::BOTTOMRIGHT:
        return string("BottomRight");
        break;
    case Toolbar::LEFTTOP:
        return string("LeftTop");
        break;
    case Toolbar::LEFTCENTER:
        return string("LeftCenter");
        break;
    case Toolbar::LEFTBOTTOM:
        return string("LeftBottom");
        break;
    case Toolbar::RIGHTTOP:
        return string("RightTop");
        break;
    case Toolbar::RIGHTCENTER:
        return string("RightCenter");
        break;
    case Toolbar::RIGHTBOTTOM:
        return string("RightBottom");
        break;
    }
    //default string
    return string("BottomCenter");
}


namespace {
class SetToolbarPlacementCmd: public FbTk::Command {
public:
    explicit SetToolbarPlacementCmd(Toolbar &tbar, Toolbar::Placement place):m_tbar(tbar), m_place(place) { }
    void execute() {
        m_tbar.setPlacement(m_place);
        m_tbar.reconfigure();        
        Fluxbox::instance()->save_rc();
    }
private:
    Toolbar &m_tbar;
    Toolbar::Placement m_place;
};

}; // end anonymous

// toolbar frame
Toolbar::Frame::Frame(FbTk::EventHandler &evh, int screen_num):
    window(screen_num, // screen (parent)
           0, 0, // pos
           10, 10, // size
           // event mask
           ButtonPressMask | ButtonReleaseMask | 
           EnterWindowMask | LeaveWindowMask,
           true) // override redirect 
{

    FbTk::EventManager &evm = *FbTk::EventManager::instance();
    // add windows to eventmanager
    evm.add(evh, window);

}

Toolbar::Frame::~Frame() {
    FbTk::EventManager &evm = *FbTk::EventManager::instance();
    // remove windows from eventmanager
    evm.remove(window);
}

Toolbar::Toolbar(BScreen &scrn, FbTk::XLayer &layer, FbTk::Menu &menu, size_t width):
    m_editing(false),
    m_hidden(false),
    frame(*this, scrn.screenNumber()),
    m_screen(scrn),
    m_toolbarmenu(menu),
    m_placementmenu(*scrn.menuTheme(),
                    scrn.screenNumber(), scrn.imageControl()),
    m_layermenu(*scrn.menuTheme(), 
                scrn.screenNumber(), 
                scrn.imageControl(),
                *scrn.layerManager().getLayer(Fluxbox::instance()->getMenuLayer()), 
                this,
                true),
    m_theme(scrn.screenNumber()),
    m_layeritem(frame.window, layer),
    m_strut(0),
    // lock rcmanager here
    m_rc_auto_hide(scrn.resourceManager().lock(), false, 
                   scrn.name() + ".toolbar.autoHide", scrn.altName() + ".Toolbar.AutoHide"),
    m_rc_maximize_over(scrn.resourceManager(), false,
                       scrn.name() + ".toolbar.maxOver", scrn.altName() + ".Toolbar.MaxOver"),
    m_rc_width_percent(scrn.resourceManager(), 65, 
                       scrn.name() + ".toolbar.widthPercent", scrn.altName() + ".Toolbar.WidthPercent"),  
    m_rc_layernum(scrn.resourceManager(), Fluxbox::Layer(Fluxbox::instance()->getDesktopLayer()), 
                  scrn.name() + ".toolbar.layer", scrn.altName() + ".Toolbar.Layer"),
    m_rc_on_head(scrn.resourceManager(), 0,
                 scrn.name() + ".toolbar.onhead", scrn.altName() + ".Toolbar.onHead"),
    m_rc_placement(scrn.resourceManager(), Toolbar::BOTTOMCENTER, 
                   scrn.name() + ".toolbar.placement", scrn.altName() + ".Toolbar.Placement"),
    m_shape(new Shape(frame.window, 0)),
    m_clock_theme(scrn.screenNumber(), "toolbar.clock", "Toolbar.Clock"),
    m_workspace_theme(scrn.screenNumber(), "toolbar.workspace", "Toolbar.Workspace"),
    m_iconbar_theme(scrn.screenNumber(), "toolbar.iconbar", "Toolbar.Iconbar") {

    // we need to get notified when the theme is reloaded
    m_theme.reconfigSig().attach(this);
    // listen to screen reconfigure
    screen().reconfigureSig().attach(this);

    moveToLayer((*m_rc_layernum).getNum());

    // TODO: nls
    m_layermenu.setLabel("Toolbar Layer");
    m_placementmenu.setLabel("Toolbar Placement");

    m_layermenu.setInternalMenu();
    m_placementmenu.setInternalMenu();
    setupMenus();

    // geometry settings
    frame.width = width;
    frame.height = 10;
    frame.bevel_w = 1;
    frame.grab_x = frame.grab_y = 0;
    
    // add toolbar items
    m_item_list.push_back(new WorkspaceNameTool(frame.window, m_workspace_theme, screen()));
    m_item_list.push_back(new IconbarTool(frame.window, m_iconbar_theme, screen()));
    m_item_list.push_back(new ClockTool(frame.window, m_clock_theme, screen()));

    m_theme.font().setAntialias(screen().antialias());
    
    // setup hide timer
    m_hide_timer.setTimeout(Fluxbox::instance()->getAutoRaiseDelay());
    FbTk::RefCount<FbTk::Command> toggle_hidden(new FbTk::SimpleCommand<Toolbar>(*this, &Toolbar::toggleHidden));
    m_hide_timer.setCommand(toggle_hidden);
    m_hide_timer.fireOnce(true);

    // get everything together
    reconfigure(); 

    // show all windows
    frame.window.showSubwindows();
    frame.window.show();

    scrn.resourceManager().unlock();
}

Toolbar::~Toolbar() {
    while (!m_item_list.empty()) {
        delete m_item_list.back();
        m_item_list.pop_back();
    }

    clearStrut();
}

void Toolbar::clearStrut() {
    if (m_strut) {
        screen().clearStrut(m_strut);
        m_strut = 0;
    }
}

void Toolbar::updateStrut() {
    bool had_strut = m_strut ? true : false;
    clearStrut();
    // we should request space if we're in autohide mode or
    // if the user dont want to request space for toolbar.
    if (doAutoHide() || *m_rc_maximize_over) {
        if (had_strut)
            screen().updateAvailableWorkspaceArea();            
        return;
    }

    // request area on screen
    int top = 0, bottom = 0, left = 0, right = 0;
    switch (placement()) {
    case TOPLEFT:
    case TOPCENTER:
    case TOPRIGHT:
        top = height();
        break;
    case BOTTOMLEFT:
    case BOTTOMCENTER:
    case BOTTOMRIGHT:
        bottom = height();
        break;
    case RIGHTTOP:
    case RIGHTCENTER:
    case RIGHTBOTTOM:
        right = width();
        break;
    case LEFTTOP:
    case LEFTCENTER:
    case LEFTBOTTOM:
        left = width();
        break;
    };
    m_strut = screen().requestStrut(left, right, top, bottom);
    screen().updateAvailableWorkspaceArea();
}

bool Toolbar::isVertical() const {
    return (placement() == RIGHTCENTER ||
            placement() == RIGHTTOP ||
            placement() == RIGHTBOTTOM ||
            placement() == LEFTCENTER ||
            placement() == LEFTTOP ||
            placement() == LEFTBOTTOM);
}


void Toolbar::raise() {
    m_layeritem.raise();
}

void Toolbar::lower() {
    m_layeritem.lower();
}

void Toolbar::reconfigure() {

    theme().font().setAntialias(screen().antialias());

    if (doAutoHide())
        m_hide_timer.start();

    frame.bevel_w = theme().bevelWidth();
    // destroy shape if the theme wasn't specified with one,
    // or create one 
    if (theme().shape() == false && m_shape.get())
        m_shape.reset(0);
    else if (theme().shape() && m_shape.get() == 0) {
        m_shape.reset(new Shape(frame.window, 0));
    }

    // recallibrate size
    setPlacement(placement());

    if (isHidden())
        frame.window.moveResize(frame.x_hidden, frame.y_hidden,
                                frame.width, frame.height);
    else {
        frame.window.moveResize(frame.x, frame.y,
                                frame.width, frame.height);
    }



    frame.window.setBorderColor(theme().borderColor());
    frame.window.setBorderWidth(theme().borderWidth());
    frame.window.clear();
    
    if (theme().shape() && m_shape.get())
        m_shape->update();


    // calculate size for fixed items
    ItemList::iterator item_it = m_item_list.begin();
    ItemList::iterator item_it_end = m_item_list.end();
    int fixed_width = 0; // combined size of all fixed items
    int fixed_items = 0; // number of fixed items
    for (; item_it != item_it_end; ++item_it) {
        if ((*item_it)->type() == ToolbarItem::FIXED) {
            fixed_width += (*item_it)->width();
            fixed_items++;
        }
    }
    // calculate what's going to be left over to the relative sized items
    int relative_width = 0;
    if (fixed_items == 0) // no fixed items, then the rest is the entire width
        relative_width = width();
    else {
        const int relative_items = m_item_list.size() - fixed_items;
        if (relative_items == 0)
            relative_width = 0;
        else // size left after fixed items / number of relative items
            relative_width = (width() - fixed_width)/relative_items;
    }

    // now move and resize the items
    int next_x = 0;
    for (item_it = m_item_list.begin(); item_it != item_it_end; ++item_it) {
        if ((*item_it)->type() == ToolbarItem::RELATIVE) {
            (*item_it)->moveResize(next_x, 0, relative_width, height());
        } else { // fixed size
            (*item_it)->moveResize(next_x, 0,
                                   (*item_it)->width(), height()); 
        }
        next_x += (*item_it)->width();
    }

    m_toolbarmenu.reconfigure();
    // we're done with all resizing and stuff now we can request a new 
    // area to be reserved on screen
    updateStrut();
}



void Toolbar::buttonPressEvent(XButtonEvent &be) {
    if (be.button != 3)
        return;

    if (! m_toolbarmenu.isVisible()) {
        int x, y;

        x = be.x_root - (m_toolbarmenu.width() / 2);
        y = be.y_root - (m_toolbarmenu.height() / 2);

        if (x < 0)
            x = 0;
        else if (x + m_toolbarmenu.width() > screen().width())
            x = screen().width() - m_toolbarmenu.width();

        if (y < 0)
            y = 0;
        else if (y + m_toolbarmenu.height() > screen().height())
            y = screen().height() - m_toolbarmenu.height();

        m_toolbarmenu.move(x, y);
        m_toolbarmenu.show();
    } else
        m_toolbarmenu.hide();
	
}


void Toolbar::buttonReleaseEvent(XButtonEvent &re) {
    if (re.button == 1)
        raise();
    else if (re.button == 4) //mousewheel scroll up
        screen().nextWorkspace(1);
    else if (re.button == 5)	//mousewheel scroll down
        screen().prevWorkspace(1);
}


void Toolbar::enterNotifyEvent(XCrossingEvent &not_used) {
    if (! doAutoHide())
        return;

    if (isHidden()) {
        if (! m_hide_timer.isTiming())
            m_hide_timer.start();
    } else {
        if (m_hide_timer.isTiming())
            m_hide_timer.stop();
    }
}

void Toolbar::leaveNotifyEvent(XCrossingEvent &not_used) {
    if (! doAutoHide())
        return;

    if (isHidden()) {
        if (m_hide_timer.isTiming()) 
            m_hide_timer.stop();
    } else if (! m_toolbarmenu.isVisible() && ! m_hide_timer.isTiming()) 
        m_hide_timer.start();

}


void Toolbar::exposeEvent(XExposeEvent &ee) {

}


void Toolbar::keyPressEvent(XKeyEvent &ke) {

}


void Toolbar::update(FbTk::Subject *subj) {

    // either screen reconfigured or theme was reloaded
    
    reconfigure();
}

void Toolbar::setPlacement(Toolbar::Placement where) {
    *m_rc_placement = where;
    int head_x = 0,
        head_y = 0,
        head_w,
        head_h;

#ifdef XINERAMA
    if (screen().hasXinerama()) {
        int head = *m_rc_on_head;
        head_x = screen().getHeadX(head);
        head_y = screen().getHeadY(head);
        head_w = screen().getHeadWidth(head);
        head_h = screen().getHeadHeight(head);
    } else 
#endif // XINERAMA
    {
        head_w = screen().width();
        head_h = screen().height();
    }


    frame.width = head_w * (*m_rc_width_percent) / 100;
    frame.height = m_theme.font().height();

    frame.height += 2;
    frame.height += (frame.bevel_w * 2);

    int bevel_width = theme().bevelWidth();
    int border_width = theme().borderWidth();

    // should we flipp sizes?
    if (isVertical()) {
        frame.width = frame.height;
        frame.height = head_h * (*m_rc_width_percent) / 100;
        if (!m_theme.font().isRotated())
            m_theme.font().rotate(90); // rotate to vertical text

    } else { // horizontal toolbar
        if (m_theme.font().isRotated()) 
            m_theme.font().rotate(0); // rotate to horizontal text
    }

    // So we get at least one pixel visible in hidden mode
    if (bevel_width <= border_width)
        bevel_width = border_width + 1;

    switch (where) {
    case TOPLEFT:
        frame.x = head_x;
        frame.y = head_y;
        frame.x_hidden = head_x;
        frame.y_hidden = head_y + bevel_width - border_width - frame.height;
        if (m_shape.get())
            m_shape->setPlaces(Shape::BOTTOMRIGHT | Shape::BOTTOMLEFT);
        break;

    case BOTTOMLEFT:
        frame.x = head_x;
        frame.y = head_y + head_h - frame.height - border_width*2;
        frame.x_hidden = head_x;
        frame.y_hidden = head_y + head_h - bevel_width - border_width;
        if (m_shape.get())
            m_shape->setPlaces(Shape::TOPRIGHT | Shape::TOPLEFT);
        break;

    case TOPCENTER:
        frame.x = head_x + (head_w - frame.width) / 2;
        frame.y = head_y;
        frame.x_hidden = frame.x;
        frame.y_hidden = head_y + bevel_width - border_width - frame.height;
        if (m_shape.get())
            m_shape->setPlaces(Shape::BOTTOMRIGHT | Shape::BOTTOMLEFT);
        break;
    case TOPRIGHT:
        frame.x = head_x + head_w - frame.width - border_width*2;
        frame.y = head_y;
        frame.x_hidden = frame.x;
        if (m_shape.get())
            m_shape->setPlaces(Shape::BOTTOMRIGHT | Shape::BOTTOMLEFT);
        break;

    case BOTTOMRIGHT:
        frame.x = head_x + head_w - frame.width - border_width*2;
        frame.y = head_y + head_h - frame.height - border_width*2;
        frame.x_hidden = frame.x;
        frame.y_hidden = head_y + head_h - bevel_width - border_width;
        if (m_shape.get())
            m_shape->setPlaces(Shape::TOPRIGHT | Shape::TOPLEFT);
        break;

    case BOTTOMCENTER: // default is BOTTOMCENTER
    default:
        frame.x = head_x + (head_w - frame.width) / 2;
        frame.y = head_y + head_h - frame.height - border_width*2;
        frame.x_hidden = frame.x;
        frame.y_hidden = head_y + head_h - bevel_width - border_width;
        if (m_shape.get())
            m_shape->setPlaces(Shape::TOPRIGHT | Shape::TOPLEFT);
        break;
    case LEFTCENTER:
        frame.x = head_x;
        frame.y = head_y + (head_h - frame.height)/2;
        frame.x_hidden = frame.x - frame.width + bevel_width + border_width;
        frame.y_hidden = frame.y;
        if (m_shape.get())
            m_shape->setPlaces(Shape::TOPRIGHT | Shape::BOTTOMRIGHT);
        break;
    case LEFTTOP:
        frame.x = head_x;
        frame.y = head_y;
        frame.x_hidden = frame.x - frame.width + bevel_width + border_width;
        frame.y_hidden = frame.y;
        if (m_shape.get())
            m_shape->setPlaces(Shape::TOPRIGHT | Shape::BOTTOMRIGHT);
        break;
    case LEFTBOTTOM:
        frame.x = head_x;
        frame.y = head_y + head_h - frame.height - border_width*2;
        frame.x_hidden = frame.x - frame.width + bevel_width + border_width;
        frame.y_hidden = frame.y;
        if (m_shape.get())
            m_shape->setPlaces(Shape::TOPRIGHT | Shape::BOTTOMRIGHT);
        break;
    case RIGHTCENTER:
        frame.x = head_x + head_w - frame.width - border_width*2;
        frame.y = head_y + (head_h - frame.height)/2;
        frame.x_hidden = frame.x + frame.width - bevel_width - border_width;
        frame.y_hidden = frame.y;
        if (m_shape.get())
            m_shape->setPlaces(Shape::TOPLEFT | Shape::BOTTOMLEFT);
        break;
    case RIGHTTOP:
        frame.x = head_x + head_w - frame.width - border_width*2;
        frame.y = head_y;
        frame.x_hidden = frame.x + frame.width - bevel_width - border_width;
        frame.y_hidden = frame.y;
        if (m_shape.get())
            m_shape->setPlaces(Shape::TOPLEFT | Shape::BOTTOMLEFT);
        break;
    case RIGHTBOTTOM:
        frame.x = head_x + head_w - frame.width - border_width*2;
        frame.y = head_y + head_h - frame.height - border_width*2;
        frame.x_hidden = frame.x + frame.width - bevel_width - border_width;
        frame.y_hidden = frame.y;
        if (m_shape.get())
            m_shape->setPlaces(Shape::TOPLEFT | Shape::BOTTOMLEFT);
        break;
    }
}

void Toolbar::toggleHidden() {
    if (isEditing()) { // don't hide if we're editing workspace label
        m_hide_timer.fireOnce(false);
        m_hide_timer.start(); // restart timer and try next timeout
        return;
    }

    m_hide_timer.fireOnce(true);

    // toggle hidden
    m_hidden = ! m_hidden;
    if (isHidden())
        frame.window.move(frame.x_hidden, frame.y_hidden);
    else 
        frame.window.move(frame.x, frame.y);

}

void Toolbar::moveToLayer(int layernum) {
    m_layeritem.moveToLayer(layernum); 
    *m_rc_layernum = layernum;
}

void Toolbar::setupMenus() {
    Toolbar &tbar = *this;
    I18n *i18n = I18n::instance();
    using namespace FBNLS;
    using namespace FbTk;

    FbTk::Menu &menu = tbar.menu();
    //!! TODO: this should be inserted by the workspace tool
    /*    

    RefCount<Command> start_edit(new SimpleCommand<Toolbar>(tbar, &Toolbar::edit));
    menu.insert(i18n->getMessage(FBNLS::ToolbarSet, FBNLS::ToolbarEditWkspcName,
    "Edit current workspace name"),
    start_edit);
    */
    menu.setLabel(i18n->getMessage(FBNLS::ToolbarSet, FBNLS::ToolbarToolbarTitle,
                                   "Toolbar")); 

    FbTk::MenuItem *toolbar_menuitem = new IntResMenuItem("Toolbar width percent",
                                                          m_rc_width_percent,
                                                          0, 100); // min/max value


    FbTk::RefCount<FbTk::Command> reconfig_toolbar(new FbTk::
                                                   SimpleCommand<Toolbar>
                                                   (tbar, &Toolbar::reconfigure));
    FbTk::RefCount<FbTk::Command> save_resources(new FbCommands::SaveResources());
    FbTk::MacroCommand *toolbar_menuitem_macro = new FbTk::MacroCommand();
    toolbar_menuitem_macro->add(reconfig_toolbar);
    toolbar_menuitem_macro->add(save_resources);

    FbTk::RefCount<FbTk::Command> reconfig_toolbar_and_save_resource(toolbar_menuitem_macro);
    toolbar_menuitem->setCommand(reconfig_toolbar_and_save_resource);  

    menu.insert(toolbar_menuitem);

    menu.insert(new BoolMenuItem(i18n->getMessage(FBNLS::CommonSet, FBNLS::CommonAutoHide,
                                                  "Auto hide"),
                                 *m_rc_auto_hide,
                                 reconfig_toolbar_and_save_resource));
    menu.insert(new BoolMenuItem("Maximize Over", *m_rc_maximize_over,
                                 reconfig_toolbar_and_save_resource));
    menu.insert("Layer...", &tbar.layermenu());

    if (tbar.screen().hasXinerama()) {
        // TODO: nls (main label plus menu heading
        menu.insert("On Head...", new XineramaHeadMenu<Toolbar>(
                                                                *tbar.screen().menuTheme(),
                                                                tbar.screen(),
                                                                tbar.screen().imageControl(),
                                                                *tbar.screen().layerManager().getLayer(Fluxbox::instance()->getMenuLayer()),
                                                                tbar,
                                                                "Toolbar on Head"
                                                                ));
    }

    // setup items in placement menu
    struct {
        int set;
        int base;
        const char *default_str;
        Toolbar::Placement placement;
    } place_menu[]  = {
        {0, 0, "Top Left", Toolbar::TOPLEFT},
        {0, 0, "Left Top", Toolbar::LEFTTOP},
        {0, 0, "Left Center", Toolbar::LEFTCENTER},
        {0, 0, "Left Bottom", Toolbar::LEFTBOTTOM}, 
        {0, 0, "Bottom Left", Toolbar::BOTTOMLEFT},
        {0, 0, "Top Center", Toolbar::TOPCENTER},
        {0, 0, 0, Toolbar::TOPCENTER},
        {0, 0, 0, Toolbar::BOTTOMCENTER},
        {0, 0, 0, Toolbar::BOTTOMCENTER},
        {0, 0, "Bottom Center", Toolbar::BOTTOMCENTER},
        {0, 0, "Top Right", Toolbar::TOPRIGHT},
        {0, 0, "Right Top", Toolbar::RIGHTTOP},
        {0, 0, "Right Center", Toolbar::RIGHTCENTER},
        {0, 0, "Right Bottom", Toolbar::RIGHTBOTTOM},
        {0, 0, "Bottom Right", Toolbar::BOTTOMRIGHT}
    };
    tbar.placementMenu().setMinimumSublevels(3);
    // create items in sub menu
    for (size_t i=0; i<15; ++i) {
        if (place_menu[i].default_str == 0) {
            tbar.placementMenu().insert("");
        } else {
            const char *i18n_str = i18n->getMessage(place_menu[i].set, 
                                                    place_menu[i].base,
                                                    place_menu[i].default_str);
            RefCount<FbTk::Command> setplace(new SetToolbarPlacementCmd(tbar, place_menu[i].placement));
            tbar.placementMenu().insert(i18n_str, setplace);
                                                              
        }
    }
    menu.insert("Placement", &tbar.placementMenu());
    tbar.placementMenu().update();
    menu.update();
}

void Toolbar::saveOnHead(int head) {
    m_rc_on_head = head;
    reconfigure();
}
