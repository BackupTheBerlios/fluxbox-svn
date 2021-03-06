// IconbarTool.cc
// Copyright (c) 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
//                and Simon Bowden    (rathnor at users.sourceforge.net)
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

// $Id: IconbarTool.cc,v 1.9 2003/08/18 11:13:32 fluxgen Exp $

#include "IconbarTool.hh"

#include "Screen.hh"
#include "ImageControl.hh"
#include "IconbarTheme.hh"
#include "Window.hh"
#include "IconButton.hh"
#include "Workspace.hh"

#include <typeinfo>

IconbarTool::IconbarTool(const FbTk::FbWindow &parent, IconbarTheme &theme, BScreen &screen):
    ToolbarItem(ToolbarItem::RELATIVE),
    m_screen(screen),
    m_icon_container(parent),
    m_theme(theme),
    m_focused_pm(0),
    m_unfocused_pm(0),
    m_empty_pm(0) {

    // setup signals
    theme.reconfigSig().attach(this);
    screen.clientListSig().attach(this);
    screen.currentWorkspaceSig().attach(this);

    update(0);

}

IconbarTool::~IconbarTool() {
    deleteIcons();

    // remove cached images
    if (m_focused_pm)
        m_screen.imageControl().removeImage(m_focused_pm);
    if (m_unfocused_pm)
        m_screen.imageControl().removeImage(m_focused_pm);
    if (m_empty_pm)
        m_screen.imageControl().removeImage(m_empty_pm);

}

void IconbarTool::move(int x, int y) {
    m_icon_container.move(x, y);
}

void IconbarTool::resize(unsigned int width, unsigned int height) {
    m_icon_container.resize(width, height);
    renderTheme();
}

void IconbarTool::moveResize(int x, int y,
                             unsigned int width, unsigned int height) {

    m_icon_container.moveResize(x, y, width, height);
    renderTheme();
}

void IconbarTool::show() {
    m_icon_container.show();
}

void IconbarTool::hide() {
    m_icon_container.hide();
}

unsigned int IconbarTool::width() const {
    return m_icon_container.width();
}

unsigned int IconbarTool::height() const {
    return m_icon_container.height();
}

unsigned int IconbarTool::borderWidth() const {
    return m_icon_container.borderWidth();
}

void IconbarTool::update(FbTk::Subject *subj) {
    // ignore updates if we're shutting down
    if (m_screen.isShuttingdown())
        return;

    // just focus signal?
    if (subj != 0 && typeid(*subj) == typeid(FluxboxWindow::WinSubject)) {
        // we handle everything except die signal here
        FluxboxWindow::WinSubject *winsubj = static_cast<FluxboxWindow::WinSubject *>(subj);
        if (subj == &(winsubj->win().focusSig())) {
            renderWindow(winsubj->win());
            return;
        } else if (subj == &(winsubj->win().workspaceSig())) {
            // workspace changed for this window, and if it's not on current workspace we remove it
            if (m_screen.currentWorkspaceID() != winsubj->win().workspaceNumber())
                removeWindow(winsubj->win());
            return;
        } else { // die sig
            removeWindow(winsubj->win());
            return; // we don't need to update the entire list
        }
    }

    bool remove_all = false; // if we should readd all windows    

    if (subj != 0 && typeid(*subj) == typeid(BScreen::ScreenSubject)) {
        BScreen::ScreenSubject *screen_subj = static_cast<BScreen::ScreenSubject *>(subj);
        if (&screen_subj->screen().currentWorkspaceSig() == screen_subj) {
            remove_all = true; // remove and readd all windows
        }
    }

    // ok, we got some signal that we need to update our iconbar container

    // get current workspace and all it's clients
    Workspace &space = *m_screen.currentWorkspace();
    // build a ItemList and add it (faster than adding single items)
    Container::ItemList items;
    Workspace::Windows itemlist(space.windowList());
    // add icons to the itemlist
    {
        BScreen::Icons::iterator icon_it = m_screen.getIconList().begin();
        BScreen::Icons::iterator icon_it_end = m_screen.getIconList().end();
        for (; icon_it != icon_it_end; ++icon_it) {
            // just add the icons that are on the this workspace
            if ((*icon_it)->workspaceNumber() == m_screen.currentWorkspaceID())
                itemlist.push_back(*icon_it);
        }
    }

    // go through the current list and see if there're windows to be added
    // (note: we dont need to check if there's one deleted since we're listening
    //  to dieSig )
    if (!remove_all) {
        IconList::iterator win_it = m_icon_list.begin();
        IconList::iterator win_it_end = m_icon_list.end();
        Workspace::Windows::iterator remove_it = itemlist.end();
        for (; win_it != win_it_end; ++win_it)
            remove_it = remove(itemlist.begin(), remove_it, &(*win_it)->win());

       itemlist.erase(remove_it, itemlist.end());
       // we dont need to do anything
       // since we dont have anything to add ...
       if (itemlist.size() == 0) 
           return;

    } else {
        deleteIcons();
    }

    // ok, now we should have a list of icons that we need to add
    Workspace::Windows::iterator it = itemlist.begin();
    Workspace::Windows::iterator it_end = itemlist.end();
    for (; it != it_end; ++it) {
        
        // we just want windows that has clients
         if ((*it)->clientList().size() == 0)
            continue;

        IconButton *button = new IconButton(m_icon_container, m_theme.focusedText().font(), **it);
        items.push_back(button);
        m_icon_list.push_back(button);

        (*it)->focusSig().attach(this);
        (*it)->dieSig().attach(this);
        (*it)->workspaceSig().attach(this);
    }

    m_icon_container.showSubwindows();
    m_icon_container.insertItems(items);

    renderTheme();
}

void IconbarTool::renderWindow(FluxboxWindow &win) {
    
    IconList::iterator icon_it = m_icon_list.begin();
    IconList::iterator icon_it_end = m_icon_list.end();
    for (; icon_it != icon_it_end; ++icon_it) {
        if (&(*icon_it)->win() == &win)
            break;
    }

    if (icon_it == m_icon_list.end())
        return;

    renderButton(*(*icon_it));
}

void IconbarTool::renderTheme() {
    Pixmap tmp = m_focused_pm;
    if (m_theme.focusedTexture().type() == (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
        m_focused_pm = 0;        
    } else {
        m_focused_pm = m_screen.imageControl().renderImage(m_icon_container.maxWidthPerClient(),
                                                           m_icon_container.height(),
                                                           m_theme.focusedTexture());
    }
        
    if (tmp)
        m_screen.imageControl().removeImage(tmp);

    tmp = m_unfocused_pm;
    if (m_theme.unfocusedTexture().type() == (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
        m_unfocused_pm = 0;        
    } else {
        m_unfocused_pm = m_screen.imageControl().renderImage(m_icon_container.maxWidthPerClient(),
                                                             m_icon_container.height(),
                                                             m_theme.unfocusedTexture());
    }
    if (tmp)
        m_screen.imageControl().removeImage(tmp);

    // if we dont have any icons then we should render empty texture
    tmp = m_empty_pm;
    if (m_theme.emptyTexture().type() == (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
        m_empty_pm = 0;
        m_icon_container.setBackgroundColor(m_theme.emptyTexture().color());
    } else {
        m_empty_pm = m_screen.imageControl().renderImage(m_icon_container.width(), m_icon_container.height(),
                                                         m_theme.emptyTexture());
        m_icon_container.setBackgroundPixmap(m_empty_pm);
    }

    if (tmp)
        m_screen.imageControl().removeImage(m_empty_pm);

    m_icon_container.setBorderWidth(m_theme.border().width());
    m_icon_container.setBorderColor(m_theme.border().color());

    // update buttons
    IconList::iterator icon_it = m_icon_list.begin();
    IconList::iterator icon_it_end = m_icon_list.end();
    for (; icon_it != icon_it_end; ++icon_it)
        renderButton(*(*icon_it));


}

void IconbarTool::renderButton(IconButton &button) {

    if (button.win().isFocused()) { // focused texture
        button.setGC(m_theme.focusedText().textGC());     
        button.setFont(m_theme.focusedText().font());
        button.setJustify(m_theme.focusedText().justify());

        if (m_focused_pm != 0)
            button.setBackgroundPixmap(m_focused_pm);
        else
            button.setBackgroundColor(m_theme.focusedTexture().color());            

        button.setBorderWidth(m_theme.focusedBorder().width());
        button.setBorderColor(m_theme.focusedBorder().color());

    } else { // unfocused
        button.setGC(m_theme.unfocusedText().textGC());
        button.setFont(m_theme.unfocusedText().font());
        button.setJustify(m_theme.unfocusedText().justify());

        if (m_unfocused_pm != 0)
            button.setBackgroundPixmap(m_unfocused_pm);
        else
            button.setBackgroundColor(m_theme.unfocusedTexture().color());

        button.setBorderWidth(m_theme.unfocusedBorder().width());
        button.setBorderColor(m_theme.unfocusedBorder().color());
    }

    button.clear();
}

void IconbarTool::deleteIcons() {
    m_icon_container.removeAll();
    while (!m_icon_list.empty()) {
        delete m_icon_list.back();
        m_icon_list.pop_back();
    }
}

void IconbarTool::removeWindow(FluxboxWindow &win) {

    // got window die signal, lets find and remove the window
    IconList::iterator it = m_icon_list.begin();
    IconList::iterator it_end = m_icon_list.end();
    for (; it != it_end; ++it) {
        if (&(*it)->win() == &win)
            break;
    }
    // did we find it?
    if (it == m_icon_list.end())
        return;

    // remove from list and render theme again
    delete *it;
    m_icon_list.erase(it);
    m_icon_container.removeItem(m_icon_container.find(*it));
    renderTheme();
}
