// WorkspaceCmd.cc for Fluxbox - an X11 Window manager
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

// $Id: WorkspaceCmd.cc,v 1.6.2.1 2003/10/28 21:34:52 rathnor Exp $

#include "WorkspaceCmd.hh"

#include "Workspace.hh"
#include "Window.hh"
#include "Screen.hh"
#include "fluxbox.hh"

#include "FbTk/KeyUtil.hh"
#include "FbTk/ActionContext.hh"

#include <algorithm>
#include <functional>

CycleWindowAction::~CycleWindowAction() {
    // free all the binding info
    resetBindings();
}

void CycleWindowAction::resetBindings() {
    BindingOptions::iterator it = m_options.begin();
    BindingOptions::iterator it_end = m_options.end();
    for (; it != it_end; ++it) {
        delete it->first;
        delete it->second;
    }
    m_options.clear();
}

void CycleWindowAction::addBinding(FbTk::ActionBinding *binding, bool forward, int options) {
    // check if it's already there
    if (binding != 0 && m_options.find(binding) != m_options.end())
        return;

    CycleOptions *opts = new CycleOptions(forward, options);
    FbTk::ActionBinding *new_binding;
    if (binding)
        new_binding = new FbTk::ActionBinding(*binding);
    else
        new_binding = new FbTk::ActionBinding();

    m_options[new_binding] = opts;
}

CycleWindowAction::CycleOptions *CycleWindowAction::getOptions(FbTk::ActionBinding *binding) {
    if (binding == 0)
        return 0;
    BindingOptions::iterator it = m_options.find(binding);
    if (it == m_options.end())
        return 0;
    else
        return it->second;
}


void CycleWindowAction::start(FbTk::ActionContext &context) {
    // we don't have to do anything special on start, since
    // nextFocus starts if it hasn't already

    // the start implies a motion
    motion(context);

}

void CycleWindowAction::motion(FbTk::ActionContext &context) {
    CycleOptions *opts = getOptions(context.binding);

    if (!opts)
        return;

    BScreen *screen = Fluxbox::instance()->keyScreen();
    if (screen == 0) 
        return;

    // if we have no mods thatcan be grabbed, then we must go linear
    int force_opts = 0;
    if (context.binding->mods() == 0)
        force_opts = BScreen::CYCLELINEAR;

    if (opts->forward)
        screen->nextFocus(opts->options | force_opts);
    else
        screen->prevFocus(opts->options | force_opts);
}

void CycleWindowAction::stop(FbTk::ActionContext &context) {
    BScreen *screen = Fluxbox::instance()->keyScreen();
    if (screen != 0)
        screen->stopFocusCycling();
    
}

void SimpleNextWindowCmd::execute() {
    BScreen *screen = Fluxbox::instance()->keyScreen();
    if (screen != 0)
        screen->nextFocus(m_option | BScreen::CYCLELINEAR);
}

void SimplePrevWindowCmd::execute() {
    BScreen *screen = Fluxbox::instance()->keyScreen();
    if (screen != 0)
        screen->prevFocus(m_option | BScreen::CYCLELINEAR);
}

void NextWorkspaceCmd::execute() {
    BScreen *screen = Fluxbox::instance()->mouseScreen();
    if (screen != 0)
        screen->nextWorkspace();
}

void PrevWorkspaceCmd::execute() {
    BScreen *screen = Fluxbox::instance()->mouseScreen();
    if (screen != 0)
        screen->prevWorkspace();
}

void LeftWorkspaceCmd::execute() {
    BScreen *screen = Fluxbox::instance()->mouseScreen();
    if (screen != 0)
        screen->leftWorkspace(m_param);
}

void RightWorkspaceCmd::execute() {
    BScreen *screen = Fluxbox::instance()->mouseScreen();
    if (screen != 0)
        screen->rightWorkspace(m_param);
}

JumpToWorkspaceCmd::JumpToWorkspaceCmd(int workspace_num):m_workspace_num(workspace_num) { }

void JumpToWorkspaceCmd::execute() {
    BScreen *screen = Fluxbox::instance()->mouseScreen();
    if (screen != 0 && m_workspace_num >= 0 && m_workspace_num < screen->getNumberOfWorkspaces())
        screen->changeWorkspaceID(m_workspace_num);
}


void ArrangeWindowsCmd::execute() {
    BScreen *screen = Fluxbox::instance()->mouseScreen();
    if (screen == 0)
        return;

    Workspace *space = screen->currentWorkspace();
    const unsigned int win_count = space->windowList().size();
  
    if (win_count == 0) 
        return;

    const unsigned int max_width = screen->width();
    const unsigned int max_heigth = screen->height();

	// try to get the same number of rows as columns.
	unsigned int rows = int(sqrt(float(win_count)));  // truncate to lower
	unsigned int cols = int(0.99 + float(win_count) / float(rows));
	if (max_width<max_heigth) {	// rotate
		unsigned int tmp;
		tmp = rows;	
		rows = cols;
		cols = tmp;
	}
    const unsigned int cal_width = max_width/cols; // calculated width ratio (width of every window)
    const unsigned int cal_heigth = max_heigth/rows; // heigth ratio (heigth of every window)

    // Resizes and sets windows positions in columns and rows.
    unsigned int x_offs = 0; // window position offset in x
    unsigned int y_offs = 0; // window position offset in y
    unsigned int window = 0; // current window 
    Workspace::Windows &windowlist = space->windowList();
    for (unsigned int i = 0; i < rows; ++i) {
        x_offs = 0;
        for (unsigned int j = 0; j < cols && window < win_count; ++j, ++window) {
			if (window==(win_count-1)) {
				// the last window gets everything that is left.
	            windowlist[window]->moveResize(x_offs, y_offs, max_width-x_offs, cal_heigth);
			} else {
	            windowlist[window]->moveResize(x_offs, y_offs, cal_width, cal_heigth);
			}
            // next x offset
            x_offs += cal_width;
        }
        // next y offset
        y_offs += cal_heigth;
    }
}

void ShowDesktopCmd::execute() {
    BScreen *screen = Fluxbox::instance()->mouseScreen();
    if (screen == 0)
        return;

    Workspace *space = screen->currentWorkspace();
    std::for_each(space->windowList().begin(),
                  space->windowList().end(),
                  std::mem_fun(&FluxboxWindow::iconify));
}
