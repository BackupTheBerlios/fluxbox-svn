// CurrentWindowCmd.cc for Fluxbox - an X11 Window manager
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

// $Id$

#include "CurrentWindowCmd.hh"

#include "fluxbox.hh"
#include "Window.hh"
#include "Screen.hh"
#include "WinClient.hh"

#include "FocusControl.hh"

CurrentWindowCmd::CurrentWindowCmd(Action act):m_action(act) { }

void CurrentWindowCmd::execute() {
    FluxboxWindow *win = FocusControl::focusedFbWindow();
    if (win)
        (win->*m_action)();
}


void KillWindowCmd::real_execute() {
    winclient().sendClose(true);
}

void SetHeadCmd::real_execute() {
    fbwindow().screen().setOnHead(fbwindow(), m_head);
}

void SendToWorkspaceCmd::real_execute() {
    fbwindow().screen().sendToWorkspace(m_workspace_num, &fbwindow(), false);
}

void SendToNextWorkspaceCmd::real_execute() {
    const int ws_nr =
        ( fbwindow().screen().currentWorkspaceID() + m_workspace_num ) %
          fbwindow().screen().numberOfWorkspaces();
    fbwindow().screen().sendToWorkspace(ws_nr, &fbwindow(), false);
}

void SendToPrevWorkspaceCmd::real_execute() {
    int ws_nr = fbwindow().screen().currentWorkspaceID() - m_workspace_num;
    if ( ws_nr < 0 )
        ws_nr += fbwindow().screen().numberOfWorkspaces();
    fbwindow().screen().sendToWorkspace(ws_nr, &fbwindow(), false);
}

void TakeToWorkspaceCmd::real_execute() {
    fbwindow().screen().sendToWorkspace(m_workspace_num, &fbwindow());
}

void TakeToNextWorkspaceCmd::real_execute() {
    unsigned int workspace_num=
        ( fbwindow().screen().currentWorkspaceID() + m_workspace_num ) %
          fbwindow().screen().numberOfWorkspaces();
    fbwindow().screen().sendToWorkspace(workspace_num, &fbwindow());
}

void TakeToPrevWorkspaceCmd::real_execute() {
    int workspace_num= fbwindow().screen().currentWorkspaceID() - m_workspace_num;
    if ( workspace_num < 0 )
        workspace_num += fbwindow().screen().numberOfWorkspaces();
    fbwindow().screen().sendToWorkspace(workspace_num, &fbwindow());
}

void GoToTabCmd::real_execute() {
    int num = m_tab_num + (m_tab_num > 0 ? 0 : fbwindow().numClients() + 1);
    if (num < 1 || num > fbwindow().numClients())
        return;

    FluxboxWindow::ClientList::iterator it = fbwindow().clientList().begin();

    while (--num > 0) ++it;

    (*it)->focus();
}

void WindowHelperCmd::execute() {
    if (FocusControl::focusedFbWindow()) // guarantee that fbwindow() exists too
        real_execute();
}

WinClient &WindowHelperCmd::winclient() {
    // will exist from execute above
    return *FocusControl::focusedWindow();
}

FluxboxWindow &WindowHelperCmd::fbwindow() {
    // will exist from execute above
    return *FocusControl::focusedFbWindow();
}

MoveCmd::MoveCmd(const int step_size_x, const int step_size_y) :
  m_step_size_x(step_size_x), m_step_size_y(step_size_y) { }

void MoveCmd::real_execute() {
  fbwindow().move(
      fbwindow().x() + m_step_size_x,
      fbwindow().y() + m_step_size_y);
}

ResizeCmd::ResizeCmd(const int step_size_x, const int step_size_y) :
    m_step_size_x(step_size_x), m_step_size_y(step_size_y) { }

void ResizeCmd::real_execute() {

    int w = std::max<int>(static_cast<int>(fbwindow().width() +
                                      m_step_size_x * fbwindow().winClient().width_inc),
                     fbwindow().frame().titlebarHeight() * 2 + 10);
    int h = std::max<int>(static_cast<int>(fbwindow().height() +
                                      m_step_size_y * fbwindow().winClient().height_inc),
                     fbwindow().frame().titlebarHeight() + 10);
    fbwindow().resize(w, h);
}

MoveToCmd::MoveToCmd(const int step_size_x, const int step_size_y, const unsigned int refc) :
    m_step_size_x(step_size_x), m_step_size_y(step_size_y), m_refc(refc) { }

void MoveToCmd::real_execute() {
    int x = 0;
    int y = 0;

    const int head = fbwindow().screen().getHead(fbwindow().fbWindow());

    if (m_refc & MoveToCmd::LOWER)
        y = fbwindow().screen().maxBottom(head) - fbwindow().height() - 2 * fbwindow().frame().window().borderWidth() - m_step_size_y;
    if (m_refc & MoveToCmd::UPPER)
        y = fbwindow().screen().maxTop(head) + m_step_size_y;
    if (m_refc & MoveToCmd::RIGHT)
        x = fbwindow().screen().maxRight(head) - fbwindow().width() - 2 * fbwindow().frame().window().borderWidth() - m_step_size_x;
    if (m_refc & MoveToCmd::LEFT)
        x = fbwindow().screen().maxLeft(head) + m_step_size_x;

    if (m_refc & MoveToCmd::IGNORE_X)
        x = fbwindow().x();
    if (m_refc & MoveToCmd::IGNORE_Y)
        y = fbwindow().y();

    fbwindow().move(x, y);
}


ResizeToCmd::ResizeToCmd(const int step_size_x, const int step_size_y) :
    m_step_size_x(step_size_x), m_step_size_y(step_size_y) { }

void ResizeToCmd::real_execute() {
    if (m_step_size_x > 0 && m_step_size_y > 0)
        fbwindow().resize(m_step_size_x, m_step_size_y);
}

FullscreenCmd::FullscreenCmd() { }
void FullscreenCmd::real_execute() {
    fbwindow().setFullscreen(!fbwindow().isFullscreen());
}

SetAlphaCmd::SetAlphaCmd(int focused, bool relative,
                         int unfocused, bool un_relative) :
    m_focus(focused), m_unfocus(unfocused),
    m_relative(relative), m_un_relative(un_relative) { }

void SetAlphaCmd::real_execute() {
    if (m_focus == 256 && m_unfocus == 256) {
        // made up signal to return to default
        fbwindow().setUseDefaultAlpha(true);
        return;
    }

    int new_alpha;
    if (m_relative) {
        new_alpha = fbwindow().getFocusedAlpha() + m_focus;
        if (new_alpha < 0) new_alpha = 0;
        if (new_alpha > 255) new_alpha = 255;
        fbwindow().setFocusedAlpha(new_alpha);
    } else
        fbwindow().setFocusedAlpha(m_focus);

    if (m_un_relative) {
        new_alpha = fbwindow().getUnfocusedAlpha() + m_unfocus;
        if (new_alpha < 0) new_alpha = 0;
        if (new_alpha > 255) new_alpha = 255;
        fbwindow().setUnfocusedAlpha(new_alpha);
    } else
        fbwindow().setUnfocusedAlpha(m_unfocus);
}
