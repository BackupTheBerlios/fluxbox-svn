// WorkspaceCmd.hh for Fluxbox - an X11 Window manager
// Copyright (c) 2003 Henrik Kinnunen (fluxgen{<a*t>}users.sourceforge.net)
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

// $Id: WorkspaceCmd.hh,v 1.2.2.2 2004/01/28 11:03:16 rathnor Exp $

#ifndef WORKSPACECMD_HH
#define WORKSPACECMD_HH
#include "FbTk/Command.hh"
#include "FbTk/Action.hh"
#include "FbTk/ActionContext.hh"

#include <map>

/**
 * This is a full action, as it needs to keep keyboard
 * focus as people alternate. Using a key motion action
 * lets the actionhandler do the tricky stuff.
 * Since prev and next happen in the same action, we 
 * only create one CycleWindowAction that knows what's
 * going on.
 */
class CycleWindowAction: public FbTk::Action {
public:
    CycleWindowAction(): 
        FbTk::Action(true, true, true, true) {} 
    ~CycleWindowAction();

    bool start(FbTk::ActionContext &context);
    bool motion(FbTk::ActionContext &context);
    bool stop(FbTk::ActionContext &context);

    // associate this binding with a particular type of action
    void addBinding(FbTk::ActionBinding *binding, bool forward, int options);
    void resetBindings();

private:
    typedef struct CycleOptions {
        CycleOptions(bool tforward, int toptions):
            forward(tforward), options(toptions) {}

        bool forward;
        int options;
    } CycleOptions;

    typedef std::map<FbTk::ActionBinding *, CycleOptions *, FbTk::ltActionBinding> BindingOptions;

    CycleOptions *getOptions(FbTk::ActionBinding *binding);

    // we use this with the context to get the options
    // and type of cycling (forward or reverse)
    BindingOptions m_options;

};


// These are simply single-hit commands.
// Stacked cycling cannot work with these.
// If you need stacked, then you need to go through the action system
class SimpleNextWindowCmd: public FbTk::Command {
public:
    explicit SimpleNextWindowCmd(int option):m_option(option) { }
    void execute();
private:
    const int m_option;
};

class SimplePrevWindowCmd: public FbTk::Command {
public:
    explicit SimplePrevWindowCmd(int option):m_option(option) { }
    void execute();
private:
    const int m_option;
};

class NextWorkspaceCmd: public FbTk::Command {
public:
    void execute();
};

class PrevWorkspaceCmd: public FbTk::Command {
public:
    void execute();
};

class LeftWorkspaceCmd: public FbTk::Command {
public:
    explicit LeftWorkspaceCmd(int num=1):m_param(num == 0 ? 1 : num) { }
    void execute();
private:
    const int m_param;
};

class RightWorkspaceCmd: public FbTk::Command {
public:
    explicit RightWorkspaceCmd(int num=1):m_param(num == 0 ? 1 : num) { }
    void execute();
private:
    const int m_param;
};

class JumpToWorkspaceCmd: public FbTk::Command {
public:
    explicit JumpToWorkspaceCmd(int workspace_num);
    void execute();
private:
    const int m_workspace_num;
};

/// arranges windows in current workspace to rows and columns
class ArrangeWindowsCmd: public FbTk::Command {
public:
    void execute();
};

class ShowDesktopCmd: public FbTk::Command {
public:
    void execute();
};

#endif // WORKSPACECMD_HH
