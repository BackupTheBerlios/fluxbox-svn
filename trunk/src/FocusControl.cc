// FocusControl.cc
// Copyright (c) 2006 Fluxbox Team (fluxgen at fluxbox dot org)
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

// $Id$

#include "FocusControl.hh"

#include "Screen.hh"
#include "Window.hh"
#include "WinClient.hh"
#include "Workspace.hh"
#include "fluxbox.hh"
#include "FbWinFrameTheme.hh"

#include "FbTk/EventManager.hh"

#include <string>
#include <iostream>
#ifdef HAVE_CSTRING
  #include <cstring>
#else
  #include <string.h>
#endif

using std::cerr;
using std::endl;
using std::string;

WinClient *FocusControl::s_focused_window = 0;
FluxboxWindow *FocusControl::s_focused_fbwindow = 0;
bool FocusControl::s_reverting = false;

FocusControl::FocusControl(BScreen &screen):
    m_screen(screen),
    m_focus_model(screen.resourceManager(), 
                  CLICKFOCUS, 
                  screen.name()+".focusModel", 
                  screen.altName()+".FocusModel"),
    m_tab_focus_model(screen.resourceManager(), 
                      CLICKTABFOCUS, 
                      screen.name()+".tabFocusModel", 
                      screen.altName()+".TabFocusModel"),
    m_focus_new(screen.resourceManager(), true, 
                screen.name()+".focusNewWindows", 
                screen.altName()+".FocusNewWindows"),
    m_cycling_list(0),
    m_was_iconic(false),
    m_cycling_last(0) {

    m_cycling_window = m_focused_list.end();
    
}

// true if the windows should be skiped else false
bool doSkipWindow(const WinClient &winclient, int opts) {
    const FluxboxWindow *win = winclient.fbwindow();
    return (!win ||
    // skip if stuck
    (opts & FocusControl::CYCLESKIPSTUCK) != 0 && win->isStuck() || 
    // skip if not active client (i.e. only visit each fbwin once)
    (opts & FocusControl::CYCLEGROUPS) != 0 && win->winClient().window() != winclient.window() ||
    // skip if shaded
    (opts & FocusControl::CYCLESKIPSHADED) != 0 && win->isShaded() || 
    // skip if iconic
    (opts & FocusControl::CYCLESKIPICONIC) != 0 && win->isIconic() ||
    // skip if hidden
    win->isFocusHidden()
    ); 
}

void FocusControl::cycleFocus(FocusedWindows *window_list, int opts, bool cycle_reverse) {

    if (!m_cycling_list) {
        if (&m_screen == FbTk::EventManager::instance()->grabbingKeyboard())
            // only set this when we're waiting for modifiers
            m_cycling_list = window_list;
        m_was_iconic = 0;
        m_cycling_last = 0;
    } else if (m_cycling_list != window_list)
        m_cycling_list = window_list;

    // too many things can go wrong with remembering this
    m_cycling_window = find(window_list->begin(),window_list->end(),s_focused_window);

    FocusedWindows::iterator it = m_cycling_window;
    FocusedWindows::iterator it_begin = window_list->begin();
    FocusedWindows::iterator it_end = window_list->end();

    // find the next window in the list that works
    while (true) {
        if (cycle_reverse && it == it_begin)
            it = it_end;
        else if (!cycle_reverse && it == it_end)
            it = it_begin;
        else
            cycle_reverse ? --it : ++it;
        // give up [do nothing] if we reach the current focused again
        if (it == m_cycling_window)
            break;
        if (it == it_end)
            continue;

        FluxboxWindow *fbwin = (*it)->fbwindow();
        if (fbwin && (fbwin->isStuck() 
             || fbwin->workspaceNumber() == m_screen.currentWorkspaceID())) {
            // either on this workspace, or stuck

            // keep track of the originally selected window in a set
            WinClient &last_client = fbwin->winClient();

            if (! (doSkipWindow(**it, opts) || !fbwin->setCurrentClient(**it)) ) {
                // moved onto a new fbwin
                if (!m_cycling_last || m_cycling_last->fbwindow() != fbwin) {
                    if (m_cycling_last) {
                        // already cycling, so restack to put windows back in
                        // their proper order
                        m_screen.layerManager().restack();

                        // set back to orig current Client in that fbwin
                        m_cycling_last->fbwindow()->setCurrentClient(*m_cycling_last, false);
                        if (m_was_iconic == m_cycling_last) {
                            s_reverting = true; // little hack
                            m_cycling_last->fbwindow()->iconify();
                            s_reverting = false;
                        }
                    }
                    m_cycling_last = &last_client;
                    if (fbwin->isIconic())
                        m_was_iconic = m_cycling_last;
                    if (m_cycling_list)
                        // else window will raise itself (if desired) on FocusIn
                        fbwin->tempRaise();
                }
                break;
            }
        }
    }
    m_cycling_window = it;
}

void FocusControl::addFocusBack(WinClient &client) {
    m_focused_list.push_back(&client);
    m_creation_order_list.push_back(&client);
}

void FocusControl::addFocusFront(WinClient &client) {
    m_focused_list.push_front(&client);
    m_creation_order_list.push_back(&client);
}

// move all clients in given window to back of focused list
void FocusControl::setFocusBack(FluxboxWindow *fbwin) {
    // do nothing if there are no windows open
    // don't change focus order while cycling
    if (m_focused_list.empty() || s_reverting)
        return;

    FocusedWindows::iterator it = m_focused_list.begin();
    // use back to avoid an infinite loop
    FocusedWindows::iterator it_back = --m_focused_list.end();

    while (it != it_back) {
        if ((*it)->fbwindow() == fbwin) {
            m_focused_list.push_back(*it);
            it = m_focused_list.erase(it);
        } else
            ++it;
    }
    // move the last one, if necessary, in order to preserve focus order
    if ((*it)->fbwindow() == fbwin) {
        m_focused_list.push_back(*it);
        m_focused_list.erase(it);
    }
}
    
void FocusControl::stopCyclingFocus() {
    // nothing to do
    if (m_cycling_list == 0)
        return;

    FocusedWindows::iterator it_end = m_cycling_list->end();
    m_cycling_last = 0;
    m_cycling_list = 0;

    // put currently focused window to top
    // the iterator may be invalid if the window died
    // in which case we'll do a proper revert focus
    if (m_cycling_window != it_end && (*m_cycling_window)->fbwindow() &&
        (*m_cycling_window)->fbwindow()->isVisible()) {
        WinClient *client = *m_cycling_window;
        m_focused_list.remove(client);
        m_focused_list.push_front(client);
        client->fbwindow()->raise();
    } else
        revertFocus(m_screen);
}

/**
 * Used to find out which window was last focused on the given workspace
 * If workspace is outside the ID range, then the absolute last focused window
 * is given.
 */
WinClient *FocusControl::lastFocusedWindow(int workspace) {
    if (m_focused_list.empty()) return 0;
    if (workspace < 0 || workspace >= (int) m_screen.numberOfWorkspaces())
        return m_focused_list.front();

    FocusedWindows::iterator it = m_focused_list.begin();    
    FocusedWindows::iterator it_end = m_focused_list.end();
    for (; it != it_end; ++it) {
        if ((*it)->fbwindow() &&
            ((((int)(*it)->fbwindow()->workspaceNumber()) == workspace ||
             (*it)->fbwindow()->isStuck()) && (*it)->acceptsFocus() &&
             !(*it)->fbwindow()->isIconic()))
            return *it;
    }
    return 0;
}

/**
 * Used to find out which window was last active in the given group
 * If ignore_client is given, it excludes that client.
 * Stuck, iconic etc don't matter within a group
 */
WinClient *FocusControl::lastFocusedWindow(FluxboxWindow &group, WinClient *ignore_client) {
    if (m_focused_list.empty()) return 0;

    FocusedWindows::iterator it = m_focused_list.begin();    
    FocusedWindows::iterator it_end = m_focused_list.end();
    for (; it != it_end; ++it) {
        if (((*it)->fbwindow() == &group) &&
            (*it) != ignore_client)
            return *it;
    }
    return 0;
}

void FocusControl::setScreenFocusedWindow(WinClient &win_client) {

     // raise newly focused window to the top of the focused list
     // don't change the order if we're cycling or shutting down
     // don't change on startup, as it may add windows that aren't listed yet
    if (!isCycling() && !m_screen.isShuttingdown() && !s_reverting &&
            !Fluxbox::instance()->isStartup()) {
        m_focused_list.remove(&win_client);
        m_focused_list.push_front(&win_client);
    }
}

void FocusControl::setFocusModel(FocusModel model) {
    m_focus_model = model;
}

void FocusControl::setTabFocusModel(TabFocusModel model) {
    m_tab_focus_model = model;
}

void FocusControl::dirFocus(FluxboxWindow &win, FocusDir dir) {
    // change focus to the window in direction dir from the given window

    // we scan through the list looking for the window that is "closest"
    // in the given direction
    
    FluxboxWindow *foundwin = 0;
    int weight = 999999, exposure = 0; // extreme values
    int borderW = m_screen.winFrameTheme().border().width(),
        top = win.y() + borderW, 
        bottom = win.y() + win.height() + borderW,
        left = win.x() + borderW,
        right = win.x() + win.width() + borderW;

    Workspace::Windows &wins = m_screen.currentWorkspace()->windowList();
    Workspace::Windows::iterator it = wins.begin();
    for (; it != wins.end(); ++it) {
        if ((*it) == &win 
            || (*it)->isIconic() 
            || (*it)->isFocusHidden() 
            || !(*it)->winClient().acceptsFocus()) 
            continue; // skip self
        
        // we check things against an edge, and within the bounds (draw a picture)
        int edge=0, upper=0, lower=0, oedge=0, oupper=0, olower=0;

        int otop = (*it)->y() + borderW, 
            // 2 * border = border on each side 
            obottom = (*it)->y() + (*it)->height() + borderW,
            oleft = (*it)->x() + borderW,
            // 2 * border = border on each side
            oright = (*it)->x() + (*it)->width() + borderW;

        // check if they intersect
        switch (dir) {
        case FOCUSUP:
            edge = obottom;
            oedge = bottom;
            upper = left;
            oupper = oleft;
            lower = right;
            olower = oright;
            break;
        case FOCUSDOWN:
            edge = top;
            oedge = otop;
            upper = left;
            oupper = oleft;
            lower = right;
            olower = oright;
            break;
        case FOCUSLEFT:
            edge = oright;
            oedge = right;
            upper = top;
            oupper = otop;
            lower = bottom;
            olower = obottom;
            break;
        case FOCUSRIGHT:
            edge = left;
            oedge = oleft;
            upper = top;
            oupper = otop;
            lower = bottom;
            olower = obottom;
            break;
        }

        if (oedge < edge) 
            continue; // not in the right direction

        if (olower <= upper || oupper >= lower) {
            // outside our horz bounds, get a heavy weight penalty
            int myweight = 100000 + oedge - edge + abs(upper-oupper)+abs(lower-olower);
            if (myweight < weight) {
                foundwin = *it;
                exposure = 0;
                weight = myweight;
            }
        } else if ((oedge - edge) < weight) {
            foundwin = *it;
            weight = oedge - edge;
            exposure = ((lower < olower)?lower:olower) - ((upper > oupper)?upper:oupper);
        } else if (foundwin && oedge - edge == weight) {
            int myexp = ((lower < olower)?lower:olower) - ((upper > oupper)?upper:oupper);
            if (myexp > exposure) {
                foundwin = *it;
                // weight is same
                exposure = myexp;
            }
        } // else not improvement
    }

    if (foundwin) 
        foundwin->setInputFocus();

}

void FocusControl::removeClient(WinClient &client) {
    if (client.screen().isShuttingdown())
        return;

    WinClient *cyc = 0;
    if (m_cycling_list && m_cycling_window != m_cycling_list->end())
        cyc = *m_cycling_window;

    m_focused_list.remove(&client);
    m_creation_order_list.remove(&client);

    if (cyc == &client) {
        m_cycling_window = m_cycling_list->end();
        stopCyclingFocus();
    }
}

void FocusControl::shutdown() {
    // restore windows backwards so they get put back correctly on restart
    FocusedWindows::reverse_iterator it = m_focused_list.rbegin();
    for (; it != m_focused_list.rend(); ++it) {
        if (*it && (*it)->fbwindow())
            (*it)->fbwindow()->restore(*it, true);
    }
}

/**
 * This function is called whenever we aren't quite sure what
 * focus is meant to be, it'll make things right ;-)
 * last_focused is set to something if we want to make use of the
 * previously focused window (it must NOT be set focused now, it
 *   is probably dying).
 */
void FocusControl::revertFocus(BScreen &screen) {
    if (s_reverting)
        return;

    FocusControl::s_reverting = true;

    WinClient *next_focus = 
        screen.focusControl().lastFocusedWindow(screen.currentWorkspaceID());

    // if setting focus fails, or isn't possible, fallback correctly
    if (!(next_focus && next_focus->focus())) {
        setFocusedWindow(0); // so we don't get dangling m_focused_window pointer
        // if there's a menu open, focus it
        if (FbTk::Menu::shownMenu())
            FbTk::Menu::shownMenu()->grabInputFocus();
        else {
            switch (screen.focusControl().focusModel()) {
            case FocusControl::MOUSEFOCUS:
                XSetInputFocus(screen.rootWindow().display(),
                               PointerRoot, None, CurrentTime);
                break;
            case FocusControl::CLICKFOCUS:
                screen.rootWindow().setInputFocus(RevertToPointerRoot,
                                                  CurrentTime);
                break;
            }
        }
    }

    FocusControl::s_reverting = false;
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
void FocusControl::unfocusWindow(WinClient &client,
                                 bool full_revert, 
                                 bool unfocus_frame) {
    // go up the transient tree looking for a focusable window

    FluxboxWindow *fbwin = client.fbwindow();
    if (fbwin == 0)
        return; // nothing more we can do

    BScreen &screen = fbwin->screen();

    if (!unfocus_frame) {
        WinClient *last_focus = screen.focusControl().lastFocusedWindow(*fbwin, &client);
        if (last_focus != 0 &&
            fbwin->setCurrentClient(*last_focus, 
                                    s_focused_window == &client)) {
            return;
        }
    }

    if (full_revert && s_focused_window == &client)
        revertFocus(screen);

}


void FocusControl::setFocusedWindow(WinClient *client) {
    BScreen *screen = client ? &client->screen() : 0;
    BScreen *old_screen = 
        FocusControl::focusedWindow() ? 
        &FocusControl::focusedWindow()->screen() : 0;

#ifdef DEBUG
    cerr<<"------------------"<<endl;
    cerr<<"Setting Focused window = "<<client<<endl;
    if (client != 0)
        cerr<<"title: "<<client->title()<<endl;
    cerr<<"Current Focused window = "<<s_focused_window<<endl;
    cerr<<"------------------"<<endl;
#endif // DEBUG

    // Update the old focused client to non focus
    if (s_focused_fbwindow)
        s_focused_fbwindow->setFocusFlag(false);

    if (client && client->fbwindow() && !client->fbwindow()->isIconic()) {
        // screen should be ok
        s_focused_fbwindow = client->fbwindow();        
        s_focused_window = client;     // update focused window
        s_focused_fbwindow->setCurrentClient(*client, 
                              false); // don't set inputfocus
        s_focused_fbwindow->setFocusFlag(true); // set focus flag

    } else {
        s_focused_window = 0;
        s_focused_fbwindow = 0;
    }

    // update AtomHandlers and/or other stuff...
    Fluxbox::instance()->updateFocusedWindow(screen, old_screen);
}

////////////////////// FocusControl RESOURCES
namespace FbTk {

template<>
std::string FbTk::Resource<FocusControl::FocusModel>::getString() const {
    switch (m_value) {
    case FocusControl::MOUSEFOCUS:
        return string("MouseFocus");
    case FocusControl::CLICKFOCUS:
        return string("ClickFocus");
    }
    // default string
    return string("ClickFocus");
}

template<>
void FbTk::Resource<FocusControl::FocusModel>::
setFromString(char const *strval) {
    if (strcasecmp(strval, "MouseFocus") == 0) 
        m_value = FocusControl::MOUSEFOCUS;
    else if (strcasecmp(strval, "ClickToFocus") == 0) 
        m_value = FocusControl::CLICKFOCUS;
    else
        setDefaultValue();
}

template<>
std::string FbTk::Resource<FocusControl::TabFocusModel>::getString() const {
    switch (m_value) {
    case FocusControl::MOUSETABFOCUS:
        return string("SloppyTabFocus");
    case FocusControl::CLICKTABFOCUS:
        return string("ClickToTabFocus");
    }
    // default string
    return string("ClickToTabFocus");
}

template<>
void FbTk::Resource<FocusControl::TabFocusModel>::
setFromString(char const *strval) {

    if (strcasecmp(strval, "SloppyTabFocus") == 0 )
        m_value = FocusControl::MOUSETABFOCUS;
    else if (strcasecmp(strval, "ClickToTabFocus") == 0) 
        m_value = FocusControl::CLICKTABFOCUS;
    else
        setDefaultValue();
}
} // end namespace FbTk
