// CurrentWindowCmd.hh for Fluxbox - an X11 Window manager
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

// $Id: CurrentWindowCmd.hh,v 1.6.2.2 2004/01/28 11:02:56 rathnor Exp $

#ifndef CURRENTWINDOWCMD_HH
#define CURRENTWINDOWCMD_HH

#include "Command.hh"
#include "FbTk/Action.hh"
#include "FbTk/WindowTools.hh"

class FluxboxWindow;
class WinClient;
class BScreen;

/// command that calls FluxboxWindow::<the function> on execute()
/// similar to FbTk::SimpleCommand<T>
class CurrentWindowCmd: public FbTk::Command {
public:
    typedef void (FluxboxWindow::* WindowOp)();
    explicit CurrentWindowCmd(WindowOp windowop);
    void execute();
private:
    WindowOp m_windowop;
};

/// command that calls BScreen::<the function> on execute()
/// similar to FbTk::SimpleCommand<T>
class CurrentScreenCmd: public FbTk::Command {
public:
    typedef void (BScreen::* ScreenOp)();
    explicit CurrentScreenCmd(ScreenOp screenop, bool use_keyscreen);
    void execute();
private:
    ScreenOp m_screenop;
    bool m_keyscreen; // false = mouse screen
};

/// helper class for window commands
/// calls real_execute if there's a focused window or a window in button press/release window
class WindowHelperCmd: public FbTk::Command {
public:
    void execute();

protected:

    WinClient &winclient();
    FluxboxWindow &fbwindow();
    virtual void real_execute() = 0;

};

class KillWindowCmd: public WindowHelperCmd {
protected:
    void real_execute();
};

class SendToWorkspaceCmd: public WindowHelperCmd {
public:
    explicit SendToWorkspaceCmd(int workspace_num):m_workspace_num(workspace_num) { }
protected:
    void real_execute();
private:
    const int m_workspace_num;
};

class RaiseFocusAction : public FbTk::Action {
public:
    RaiseFocusAction(bool doraise, bool dofocus, int actlevel, bool passthrough = true);

    bool start(FbTk::ActionContext &context);

    int getLevel();

private:
    bool raise, focus;
    int level;

};

class FbWindowTransform : public FbTk::WindowTransform {
public:
    FbWindowTransform();

    void moveWindow(int x, int y);
    void resizeWindow(unsigned int width, unsigned int height);
    void moveResizeWindow(int x, int y, unsigned int width, unsigned int height);

    void setFbWindow(FluxboxWindow *window);

    // current window being manipulated
    inline FluxboxWindow *fbwindow() { return m_window; }
    inline BScreen *screen() { return m_screen; }

private:
    FluxboxWindow *m_window;
    BScreen *m_screen;

};

// user drags the window to move it
class MoveAction : public FbTk::Action, public FbWindowTransform {
public:
    MoveAction(bool is_toplevel = true);
    ~MoveAction();

    bool start(FbTk::ActionContext &context);
    bool motion(FbTk::ActionContext &context);
    bool stop(FbTk::ActionContext &context);

    Cursor cursor();

    int getLevel();

protected:

    bool toplevel;
    int m_grab_x, m_grab_y;
};


// user drags the window to resize it
class ResizeAction : public MoveAction {
public:
    ResizeAction(bool is_toplevel = true);
    ~ResizeAction();

    bool start(FbTk::ActionContext &context);
    bool motion(FbTk::ActionContext &context);
    bool stop(FbTk::ActionContext &context);

    enum ResizeCorner {
      LEFTTOP,
      LEFTBOTTOM,
      RIGHTBOTTOM,
      RIGHTTOP
    };

    Cursor cursor();

private:

    ResizeCorner m_corner;

};


// user drags a tab to relocate it
// if dropped on a tab, inserted before that tab
// if dropped somewhere else in a window, inserted at end of tabs
// if dropped on no window, own frame is created
class MoveTabAction : public FbTk::Action, public FbTk::WindowTransform {
public:
    MoveTabAction();
    ~MoveTabAction();

    bool start(FbTk::ActionContext &context);
    bool motion(FbTk::ActionContext &context);
    bool stop(FbTk::ActionContext &context);

    Cursor cursor();

    int getLevel();

    void moveWindow(int x, int y);
    void moveResizeWindow(int x, int y, unsigned int width, unsigned int height);

protected:

    int m_grab_x, m_grab_y, orig_x, orig_y;

    WinClient *m_client;

    const FbTk::FbWindow *orig_parent;
};


// move cmd, relative position
class MoveCmd: public WindowHelperCmd {
public:
    explicit MoveCmd(const int step_size_x, const int step_size_y);
protected:
    void real_execute();

private:
    const int m_step_size_x;
    const int m_step_size_y;
};

// resize cmd, relative size
class ResizeCmd: public WindowHelperCmd{
public:
  explicit ResizeCmd(int step_size_x, int step_size_y);
protected:
  void real_execute();

private:

  const int m_step_size_x;
  const int m_step_size_y;
};

class MoveToCmd: public WindowHelperCmd {
public:
    explicit MoveToCmd(const int step_size_x, const int step_size_y);
protected:
    void real_execute();

private:
    const int m_step_size_x;
    const int m_step_size_y;
};

// resize cmd
class ResizeToCmd: public WindowHelperCmd{
public:
  explicit ResizeToCmd(int step_size_x, int step_size_y);
protected:
  void real_execute();

private:

  const int m_step_size_x;
  const int m_step_size_y;
};
#endif // CURRENTWINDOWCMD_HH
