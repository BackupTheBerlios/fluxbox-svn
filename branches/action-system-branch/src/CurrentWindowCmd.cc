// CurrentWindowCmd.cc for Fluxbox - an X11 Window manager
// Copyright (c) 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
//                and Simon Bowden (rathnor at users.sourceforge.net)
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

// $Id: CurrentWindowCmd.cc,v 1.8.2.2 2004/01/28 11:02:55 rathnor Exp $

#include "CurrentWindowCmd.hh"

#include "fluxbox.hh"
#include "Window.hh"
#include "Screen.hh"
#include "WinClient.hh"
#include "FbWinFrame.hh"
#include "FbWinFrameTheme.hh"
#include "RootTheme.hh"
#include "Keys.hh"

#include "FbTk/ActionContext.hh"

#include <iostream>
using namespace std;

CurrentWindowCmd::CurrentWindowCmd(WindowOp op):m_windowop(op) { }

void CurrentWindowCmd::execute() {
    WinClient *client = Fluxbox::instance()->getFocusedWindow();
    if (client && client->fbwindow())
        (client->fbwindow()->*m_windowop)();
}

CurrentScreenCmd::CurrentScreenCmd(ScreenOp op, bool use_keyscreen):m_screenop(op), m_keyscreen(use_keyscreen) { }

void CurrentScreenCmd::execute() {
    BScreen *screen = 0;
    if (m_keyscreen)
        screen = Fluxbox::instance()->keyScreen();
    else
        screen = Fluxbox::instance()->mouseScreen();

    if (screen)
        (screen->*m_screenop)();
}


void KillWindowCmd::real_execute() {
    winclient().sendClose(true);
}

void SendToWorkspaceCmd::real_execute() {
    if (m_workspace_num >= 0 && m_workspace_num < fbwindow().screen().getNumberOfWorkspaces())
        fbwindow().screen().sendToWorkspace(m_workspace_num, &fbwindow());
}

void WindowHelperCmd::execute() {
    WinClient *client = Fluxbox::instance()->getFocusedWindow();
    if (client && client->fbwindow()) // guarantee that fbwindow() exists too
        real_execute();
}

WinClient &WindowHelperCmd::winclient() {
    // will exist from execute above
    return *Fluxbox::instance()->getFocusedWindow();
}

FluxboxWindow &WindowHelperCmd::fbwindow() {
    // will exist from execute above
    return *Fluxbox::instance()->getFocusedWindow()->fbwindow();
}

RaiseFocusAction::RaiseFocusAction(bool doraise, bool dofocus, int actlevel, bool passthrough):
    FbTk::Action(true, false, false, false, passthrough),
    raise(doraise),
    focus(dofocus),
    level(actlevel)
{}

bool RaiseFocusAction::start(FbTk::ActionContext &context) {
    cerr<<"RAISEFOCUS"<<endl;
    WinClient *client = Fluxbox::instance()->searchWindow(context.win, true);
    if (client == 0 || client->fbwindow() == 0)
        return false;

    cerr<<"Winclient = "<<client<<endl;

    if (raise)
        client->fbwindow()->raise();

    if (focus)
        client->fbwindow()->setCurrentClient(*client, true);

    return true;
}

int RaiseFocusAction::getLevel() {
    return level;
}

FbWindowTransform::FbWindowTransform():
    m_window(0), m_screen(0)
{}

void FbWindowTransform::setFbWindow(FluxboxWindow *window) {
    m_window = window;
    FbTk::FbWindow *fbwin = 0;
    if (window) {
        fbwin = &window->frame().window();
        m_screen = &window->screen();
        setDrawable(m_screen->rootWindow().window(), m_screen->rootTheme().opGC());
    } else {
        m_screen = 0;
    }

    setWindow(fbwin);
}

void FbWindowTransform::moveWindow(int x, int y) {
    if (m_window)
        m_window->move(x, y);
}

void FbWindowTransform::resizeWindow(unsigned int width, unsigned int height) {
    if (m_window)
        m_window->resize(width, height);

}

void FbWindowTransform::moveResizeWindow(int x, int y, 
                                  unsigned int width, 
                                  unsigned int height) {
    if (m_window)
        m_window->moveResize(x, y, width, height);
}

MoveAction::MoveAction(bool is_toplevel) :
    FbTk::Action(true, true, true, false),
    toplevel(is_toplevel)
{}

MoveAction::~MoveAction() {
    if (fbwindow() != 0) {
        FbTk::ActionContext tempcontext;
        stop(tempcontext); // stop doesn't use context
    }
}

bool MoveAction::start(FbTk::ActionContext &context) {
    if (fbwindow() != 0)
        return false; // eeek, this really shouldn't happen

    // first find the window that it is
    WinClient *client = Fluxbox::instance()->searchWindow(context.win, true);
    cerr<<"move start client = "<<client<<", win = "<<hex<<context.win<<dec<<endl;
    if (client == 0 || client->fbwindow() == 0 
        || Fluxbox::instance()->getIgnoreBorder() && client->fbwindow()->onBorder(context.x_root, context.y_root))
        return false;

    setFbWindow(client->fbwindow());

    useOutline(!screen()->doOpaqueMove());
    fbwindow()->startMoving(this);

    FbWinFrame &frame = fbwindow()->frame();

    int borderW = frame.window().borderWidth();

    m_grab_x = context.x_root - frame.x() - borderW;
    m_grab_y = context.y_root - frame.y() - borderW;

    if (screen()->doShowWindowPos()) {
        FbWinFrame &frame = fbwindow()->frame();
        screen()->showPosition(frame.x(), frame.y());
    }

    return true;
}


bool MoveAction::motion(FbTk::ActionContext &context) {
    if (fbwindow() == 0) {
#ifdef DEBUG
        cerr<<"move motion on window = "<<hex<<context.win<<dec<<" without an active FluxboxWindow!"<<endl;
#endif // DEBUG
        return false; // eeek, this really shouldn't happen
    }

    int dx = context.x_root - m_grab_x, 
        dy = context.y_root - m_grab_y;

    FbWinFrame &frame = fbwindow()->frame();

    int borderW = frame.window().borderWidth();

    dx -= borderW;
    dy -= borderW;


    // Warp to next or previous workspace?, must have moved sideways some
    int moved_x = context.x_root - m_grab_x - currX();
    bool warped = false;

    if (moved_x && screen()->isWorkspaceWarping()) {
        unsigned int cur_id = screen()->currentWorkspaceID();
        unsigned int new_id = cur_id;
        const int warpPad = screen()->getEdgeSnapThreshold();
        // 1) if we're inside the border threshold
        // 2) if we moved in the right direction
        if (context.x_root >= int(screen()->width()) - warpPad - 1 &&
            moved_x > 0) {
            //warp right
            new_id = (cur_id + 1) % screen()->getCount();
            dx = - context.x_root; // move mouse back to x=0
        } else if (context.x_root <= warpPad &&
                   moved_x < 0) {
            //warp left
            new_id = (cur_id + screen()->getCount() - 1) % screen()->getCount();
            dx = screen()->width() - context.x_root-1; // move mouse to screen width - 1
        }

        if (new_id != cur_id) {
            warped = true;
            XWarpPointer(FbTk::App::instance()->display(), None, None, 0, 0, 0, 0, dx, 0);

            screen()->changeWorkspaceID(new_id);

            // dx is the difference, so our new x is what it would  have been
            // without the warp, plus the difference.
            dx += context.x_root - m_grab_x;
        }
    }
    // dx = current left side, dy = current top
    fbwindow()->snapWindowPos(dx, dy);

    move(dx, dy);

    if (screen()->doShowWindowPos())
        screen()->showPosition(dx, dy);

    return true;
}

bool MoveAction::stop(FbTk::ActionContext &context) {
    if (fbwindow() == 0)
        return false; // eeek, this really shouldn't happen

    // stop moving first so that moveResize does normal event sending etc
    fbwindow()->stopMoving();

    commit();

    screen()->hideGeometry();

    setFbWindow(0);
    return true;
}

Cursor MoveAction::cursor() {
    if (fbwindow() != 0)
        return fbwindow()->frame().theme().moveCursor();
    else
        return None;
}

int MoveAction::getLevel() {
    return ((toplevel)?(Keys::TOPLEVEL):0) | Keys::TOPNOTCLIENT | Keys::TAB;
}

ResizeAction::ResizeAction(bool is_toplevel) :
    MoveAction(is_toplevel)
{}

ResizeAction::~ResizeAction() {
    if (fbwindow() != 0) {
        FbTk::ActionContext tempcontext;
        stop(tempcontext); // stop doesn't use context
    }
}

bool ResizeAction::start(FbTk::ActionContext &context) {
    if (fbwindow() != 0)
        return false; // eeek, this really shouldn't happen

    // first find the window that it is
    WinClient *client = Fluxbox::instance()->searchWindow(context.win, true);

    if (client == 0 
        || client->fbwindow() == 0 
        || !client->fbwindow()->isResizable()
        || Fluxbox::instance()->getIgnoreBorder() 
           && client->fbwindow()->onBorder(context.x_root, context.y_root))
    return false;

    setFbWindow(client->fbwindow());

    int cx = fbwindow()->width() / 2; // centre of window x
    int cy = fbwindow()->height() / 2;

    if (context.x < cx)
        m_corner = (context.y < cy) ? LEFTTOP : LEFTBOTTOM;
    else
        m_corner = (context.y < cy) ? RIGHTTOP : RIGHTBOTTOM;

    useOutline(!screen()->doOpaqueMove());
    fbwindow()->startMoving(this);

    // use root for resize
    m_grab_x = context.x_root;
    m_grab_y = context.y_root;

    if (screen()->doShowWindowPos()) {
        FbWinFrame &frame = fbwindow()->frame();
        int w = frame.width(),
            h = frame.height(),
            user_w = w,
            user_h = h;

        fbwindow()->snapWindowSize(w, h, &user_w, &user_h);

        screen()->showGeometry(user_w, user_h);
    }

    return true;
}


bool ResizeAction::motion(FbTk::ActionContext &context) {
    if (fbwindow() == 0) {
#ifdef DEBUG
        cerr<<"resize motion on window = "<<hex<<context.win<<dec<<" without an active FluxboxWindow!"<<endl;
#endif // DEBUG
        return false; // eeek, this really shouldn't happen
    }

    bool leftwards = m_corner == LEFTTOP || m_corner == LEFTBOTTOM;
    bool upwards = m_corner == LEFTTOP || m_corner == RIGHTTOP;

    int new_w = static_cast<signed>(origWidth()),
        new_h = static_cast<signed>(origHeight());

    if (leftwards)
        new_w += m_grab_x - context.x_root;
    else
        new_w -= m_grab_x - context.x_root;

    if (upwards)
        new_h += m_grab_y - context.y_root;
    else
        new_h -= m_grab_y - context.y_root;

    if (new_w <= 0)
        new_w = 1;
    if (new_h <= 0)
        new_h = 1;

    int new_x = currX(), new_y = currY();

    int user_w = 0, user_h = 0;
    fbwindow()->snapWindowSize(new_w, new_h, &user_w, &user_h);

    // move X/Y if necessary
    if (leftwards)
        new_x += static_cast<signed>(currWidth()) - new_w;

    if (upwards)
        new_y += static_cast<signed>(currHeight()) - new_h;

    moveResize(new_x, new_y, new_w, new_h);

    if (screen()->doShowWindowPos())
        screen()->showGeometry(user_w, user_h);

    return true;
}

bool ResizeAction::stop(FbTk::ActionContext &context) {
    if (fbwindow() == 0)
        return false; // eeek, this really shouldn't happen

    // stop moving first so that moveResize does normal event sending etc
    fbwindow()->stopMoving();

    commit();

    screen()->hideGeometry();

    setFbWindow(0);

    return true;
}

Cursor ResizeAction::cursor() {
    if (fbwindow() != 0) {
        FbWinFrame &frame = fbwindow()->frame();
        return
            (m_corner == LEFTTOP) ? frame.theme().upperLeftAngleCursor() :
            (m_corner == RIGHTTOP) ? frame.theme().upperRightAngleCursor() :
            (m_corner == RIGHTBOTTOM) ? frame.theme().lowerRightAngleCursor() :
            frame.theme().lowerLeftAngleCursor();
    } else
        return None;
}


MoveTabAction::MoveTabAction() :
    FbTk::Action(true, true, true, false), 
    m_client(0),
    orig_parent(0)
{}

MoveTabAction::~MoveTabAction() {
    if (m_client != 0) {
        FbTk::ActionContext tempcontext;
        stop(tempcontext); // stop doesn't use context
    }
}

bool MoveTabAction::start(FbTk::ActionContext &context) {
    cerr<<"==== Movetab action start"<<endl;
    if (m_client != 0)
        return false; // eeek, this really shouldn't happen

    cerr<<"==== Movetab action start 2"<<endl;

    // first find the window that it is
    m_client = Fluxbox::instance()->searchTabs(context.win, true);
    if (m_client == 0 || m_client->fbwindow() == 0) {
        m_client = 0;
        return false;
    }

    FluxboxWindow &fbwin = *m_client->fbwindow();
    FbTk::FbWindow *win = fbwin.getLabelWindow(m_client);
    if (!win)
        return false; // this really shouldn't happen...

    BScreen &screen = fbwin.screen();
    setDrawable(screen.rootWindow().window(), screen.rootTheme().opGC());
    setWindow(win);

    m_grab_x = context.x;
    m_grab_y = context.y;
    orig_x = win->x();
    orig_y = win->y();

    useOutline(!screen.doOpaqueMove());

    // Reparent the fbwin to root just for this special move
    // also need to remember the original parent+placement of course
    if (!outline() && win->parent() != 0) {
        orig_parent = win->parent();
        win->reparent(screen.rootWindow(), context.x_root - context.x, context.y_root - context.y);
        win->raise();
        win->show();
        win->clear();
    }

    return motion(context);

}

bool MoveTabAction::motion(FbTk::ActionContext &context) {

    if (m_client == 0 || m_client->fbwindow() == 0)
        return false; // eeek, this really shouldn't happen
  

    FluxboxWindow &fbwindow = *m_client->fbwindow();
    BScreen &screen = fbwindow.screen();

    int dx = context.x_root - m_grab_x, 
        dy = context.y_root - m_grab_y;

    int borderW = getWindow()->borderWidth();

    dx -= borderW;
    dy -= borderW;

    // Warp to next or previous workspace?, must have moved sideways some
    int moved_x = context.x_root - m_grab_x - currX();
    bool warped = false;

    if (moved_x && screen.isWorkspaceWarping()) {
        unsigned int cur_id = screen.currentWorkspaceID();
        unsigned int new_id = cur_id;
        const int warpPad = screen.getEdgeSnapThreshold();
        // 1) if we're inside the border threshold
        // 2) if we moved in the right direction
        if (context.x_root >= int(screen.width()) - warpPad - 1 &&
            moved_x > 0) {
            //warp right
            new_id = (cur_id + 1) % screen.getCount();
            dx = - context.x_root; // move mouse back to x=0
        } else if (context.x_root <= warpPad &&
                   moved_x < 0) {
            //warp left
            new_id = (cur_id + screen.getCount() - 1) % screen.getCount();
            dx = screen.width() - context.x_root-1; // move mouse to screen width - 1
        }

        if (new_id != cur_id) {
            warped = true;
            XWarpPointer(FbTk::App::instance()->display(), None, None, 0, 0, 0, 0, dx, 0);

            screen.changeWorkspaceID(new_id);

            // dx is the difference, so our new x is what it would  have been
            // without the warp, plus the difference.
            dx += context.x_root - m_grab_x;
        }
    }
    // dx = current left side, dy = current top
    move(dx, dy);

    return true;
}

bool MoveTabAction::stop(FbTk::ActionContext &context) {
    if (m_client == 0 || m_client->fbwindow() == 0)
        return false; // eeek, this really shouldn't happen

    cerr<<"==== Movetab action stop"<<endl;

    FluxboxWindow &fbwin = *m_client->fbwindow();

    if (!outline() && orig_parent != 0) {
        getWindow()->reparent(*orig_parent, orig_x, orig_y);
        orig_parent = 0;
    }

    commit();

    Display *display = FbTk::App::instance()->display();
    Window child = 0;
    int dest_x = 0, dest_y = 0;
    int x = context.x_root;
    int y = context.y_root;

    if (XTranslateCoordinates(display, fbwin.parent().window(), 
                              fbwin.parent().window(),
                              x, y, &dest_x, &dest_y, &child)) {        
        // search for a fluxboxwindow 
        WinClient *client = Fluxbox::instance()->searchWindow(child, true);
        FluxboxWindow *attach_to_win = 0;
        if (client)
            attach_to_win = client->fbwindow();

        if (attach_to_win != &fbwin &&
            attach_to_win != 0) {
            cerr<<"attach_client -> "<<attach_to_win<<endl;
            attach_to_win->attachClient(*m_client);
        } else if (attach_to_win != &fbwin) { // disconnect client if we didn't drop on a window
            cerr<<"detach_client -> "<<attach_to_win<<", "<<&fbwin<<endl;
            fbwin.detachClient(*m_client, x - m_grab_x, y - m_grab_y, true);
            // TODO this should put it where the mouse is
        }
                    
    }

    setWindow(0);
    m_client = 0;

    return true;
}

void MoveTabAction::moveWindow(int x, int y) {
    if (orig_parent != 0)
        getWindow()->move(x, y);
}

void MoveTabAction::moveResizeWindow(int x, int y, unsigned int width, unsigned int height) {
    moveWindow(x, y);
}


Cursor MoveTabAction::cursor() {
    if (m_client != 0 && m_client->fbwindow() != 0)
        return m_client->fbwindow()->frame().theme().moveCursor();
    else
        return None;
}

int MoveTabAction::getLevel() {
    return Keys::TAB;
}

MoveCmd::MoveCmd(const int step_size_x, const int step_size_y) :
  m_step_size_x(step_size_x), m_step_size_y(step_size_y) { }

void MoveCmd::real_execute() {
  fbwindow().move(fbwindow().x() + m_step_size_x,
                  fbwindow().y() + m_step_size_y);
}

ResizeCmd::ResizeCmd(const int step_size_x, const int step_size_y) :
  m_step_size_x(step_size_x), m_step_size_y(step_size_y) { }

void ResizeCmd::real_execute() {
  fbwindow().resize(fbwindow().width() + 
                    m_step_size_x * fbwindow().winClient().width_inc,
                    fbwindow().height() + 
                    m_step_size_y * fbwindow().winClient().height_inc);
}

MoveToCmd::MoveToCmd(const int step_size_x, const int step_size_y) :
    m_step_size_x(step_size_x), m_step_size_y(step_size_y) { }

void MoveToCmd::real_execute() {
    fbwindow().move(m_step_size_x, m_step_size_y);
}

ResizeToCmd::ResizeToCmd(const int step_size_x, const int step_size_y) :
    m_step_size_x(step_size_x), m_step_size_y(step_size_y) { }

void ResizeToCmd::real_execute() {
    fbwindow().resize(m_step_size_x, m_step_size_y);
}
