// Workspace.cc for Fluxbox
// Copyright (c) 2001 - 2002 Henrik Kinnunen (fluxgen@linuxmail.org)
//
// Workspace.cc for Blackbox - an X11 Window manager
// Copyright (c) 1997 - 2000 Brad Hughes (bhughes@tcac.net)
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.	IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// $Id: Workspace.cc,v 1.12 2002/02/26 22:34:49 fluxgen Exp $

// use GNU extensions
#ifndef	 _GNU_SOURCE
#define	 _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef		HAVE_CONFIG_H
#	include "../config.h"
#endif // HAVE_CONFIG_H

#include "i18n.hh"
#include "fluxbox.hh"
#include "Clientmenu.hh"
#include "Screen.hh"
#include "Toolbar.hh"
#include "Window.hh"
#include "Workspace.hh"
#include "Windowmenu.hh"
#include "StringUtil.hh"

#ifdef		HAVE_STDIO_H
#	include <stdio.h>
#endif // HAVE_STDIO_H

#ifdef		STDC_HEADERS
#	include <string.h>
#endif // STDC_HEADERS

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <algorithm>
#include <iostream>
using namespace std;

#ifdef DEBUG
#include <iostream>
using namespace std;
#endif

Workspace::Workspace(BScreen *scrn, int i):
screen(scrn),
lastfocus(0),
name(""),
id(i),
cascade_x(32), cascade_y(32)
{

	clientmenu = new Clientmenu(this);

	char *tmp;
	screen->getNameOfWorkspace(id, &tmp);
	setName(tmp);

	if (tmp)
		delete [] tmp;
}


Workspace::~Workspace() {
	delete clientmenu;
}


const int Workspace::addWindow(FluxboxWindow *w, Bool place) {
	if (! w)
		return -1;

	if (place)
		placeWindow(w);

	w->setWorkspace(id);
	w->setWindowNumber(windowList.size());

	stackingList.push_front(w);
	windowList.push_back(w);

	clientmenu->insert((const char **) w->getTitle());
	clientmenu->update();

	screen->updateNetizenWindowAdd(w->getClientWindow(), id);

	raiseWindow(w);

	return w->getWindowNumber();
}


const int Workspace::removeWindow(FluxboxWindow *w) {
	if (! w) return -1;

	stackingList.remove(w);

	if (w->isFocused()) {
		if (screen->isSloppyFocus())
			Fluxbox::instance()->setFocusedWindow((FluxboxWindow *) 0);
		else if (w->isTransient() && w->getTransientFor() &&
				w->getTransientFor()->isVisible())
			w->getTransientFor()->setInputFocus();
		else {
			FluxboxWindow *top = 0;
			if (stackingList.size()!=0)
				top = stackingList.front();

			if (!top || !top->setInputFocus()) {
				Fluxbox::instance()->setFocusedWindow((FluxboxWindow *) 0);
				XSetInputFocus(Fluxbox::instance()->getXDisplay(),
					screen->getToolbar()->getWindowID(),
					RevertToParent, CurrentTime);						
			}
		}
	}
	
	if (lastfocus == w)
		lastfocus = (FluxboxWindow *) 0;

	windowList.erase(windowList.begin() + w->getWindowNumber());
	clientmenu->remove(w->getWindowNumber());
	clientmenu->update();

	screen->updateNetizenWindowDel(w->getClientWindow());

	{
		Windows::iterator it = windowList.begin();
		Windows::const_iterator it_end = windowList.end();
		for (int i = 0; it != it_end; ++it, ++i) {
			(*it)->setWindowNumber(i);
		}
	}

	return windowList.size();
}


void Workspace::showAll(void) {
	WindowStack::iterator it = stackingList.begin();
	WindowStack::iterator it_end = stackingList.end();
	for (; it != it_end; ++it) {
		(*it)->deiconify(False, False);
	}
}


void Workspace::hideAll(void) {
	WindowStack::reverse_iterator it = stackingList.rbegin();
	WindowStack::reverse_iterator it_end = stackingList.rend();
	for (; it != it_end; ++it) {
		if (! (*it)->isStuck())
			(*it)->withdraw();
	}
}


void Workspace::removeAll(void) {
	Windows::iterator it = windowList.begin();
	Windows::const_iterator it_end = windowList.end();
	for (; it != it_end; ++it) {
		(*it)->iconify();
	}
}


void Workspace::raiseWindow(FluxboxWindow *w) {
	FluxboxWindow *win = (FluxboxWindow *) 0, *bottom = w;

	while (bottom->isTransient() && bottom->getTransientFor() &&
			bottom->getTransientFor() != bottom) //prevent infinite loop
		bottom = bottom->getTransientFor();

	int i = 1;
	win = bottom;
	while (win->hasTransient() && win->getTransient() &&
			win->getTransient() != win) {//prevent infinite loop
		win = win->getTransient();

		i++;
	}

	Window *nstack = new Window[i], *curr = nstack;
	Workspace *wkspc;

	win = bottom;
	while (True) {
		*(curr++) = win->getFrameWindow();
		screen->updateNetizenWindowRaise(win->getClientWindow());

		if (! win->isIconic()) {
			wkspc = screen->getWorkspace(win->getWorkspaceNumber());
			wkspc->stackingList.remove(win);
			wkspc->stackingList.push_front(win);
		}

		if (! win->hasTransient() || ! win->getTransient() ||
			win->getTransient() == win) //prevent infinite loop
			break;
		
		win = win->getTransient();
	}

	screen->raiseWindows(nstack, i);

	delete [] nstack;
}

void Workspace::lowerWindow(FluxboxWindow *w) {
	FluxboxWindow *win = (FluxboxWindow *) 0, *bottom = w;

	while (bottom->isTransient() && bottom->getTransientFor()
			&& bottom->getTransientFor() != bottom) //prevent infinite loop
		bottom = bottom->getTransientFor();

	int i = 1;
	win = bottom;
	while (win->hasTransient() && win->getTransient() && 
			win->getTransient() != win) { //prevent infinite loop
		win = win->getTransient();

		i++;
	}

	Window *nstack = new Window[i], *curr = nstack;
	Workspace *wkspc;

	while (True) {
		*(curr++) = win->getFrameWindow();
		screen->updateNetizenWindowLower(win->getClientWindow());

		if (! win->isIconic()) {
			wkspc = screen->getWorkspace(win->getWorkspaceNumber());
			wkspc->stackingList.remove(win);
			wkspc->stackingList.push_back(win);
		}

		if (! win->getTransientFor() || 
			win->getTransientFor() == win)//prevent infinite loop
			break;

		win = win->getTransientFor();
	}

	Fluxbox::instance()->grab();

	XLowerWindow(screen->getBaseDisplay()->getXDisplay(), *nstack);
	XRestackWindows(screen->getBaseDisplay()->getXDisplay(), nstack, i);

	Fluxbox::instance()->ungrab();

	delete [] nstack;
}


void Workspace::reconfigure(void) {
	clientmenu->reconfigure();

	Windows::iterator it = windowList.begin();
	Windows::iterator it_end = windowList.end();
	for (; it != it_end; ++it) {
		if ((*it)->validateClient())
			(*it)->reconfigure();
	}
}


FluxboxWindow *Workspace::getWindow(int index) {
	if ((index >= 0) && (index < windowList.size()))
		return windowList[index];
	else
		return 0;
}


const int Workspace::getCount(void) {
	return windowList.size();
}


void Workspace::update(void) {
	clientmenu->update();
	screen->getToolbar()->redrawWindowLabel(True);
}


bool Workspace::isCurrent(void) {
	return (id == screen->getCurrentWorkspaceID());
}


bool Workspace::isLastWindow(FluxboxWindow *w) {
	return (w == windowList.back());
}

void Workspace::setCurrent(void) {
	screen->changeWorkspaceID(id);
}


void Workspace::setName(char *new_name) {

	if (new_name) {
		name = new_name;
	} else {
		char tname[128];
		sprintf(tname, I18n::instance()->
			getMessage(
#ifdef		NLS
			WorkspaceSet, WorkspaceDefaultNameFormat,
#else // !NLS
			0, 0,
#endif // NLS
			"Workspace %d"), id + 1);
		name = tname;
	}
	
	screen->updateWorkspaceNamesAtom();
	
	clientmenu->setLabel(name.c_str());
	clientmenu->update();
}

//------------ shutdown ---------
// Calles  restore on all windows
// in the workspace and then
// clears the windowList
//-------------------------------
void Workspace::shutdown(void) {

	while (!windowList.empty()) {
		windowList.back()->restore();
		delete windowList.back(); //delete window (the window removes it self from windowList)
	}
}


void Workspace::placeWindow(FluxboxWindow *win) {
	Bool placed = False;
	int win_w = win->getWidth() + (screen->getBorderWidth2x() * 2),
		win_h = win->getHeight() + (screen->getBorderWidth2x() * 2),
#ifdef		SLIT
		slit_x = screen->getSlit()->getX() - screen->getBorderWidth(),
		slit_y = screen->getSlit()->getY() - screen->getBorderWidth(),
		slit_w = screen->getSlit()->getWidth() +
			(screen->getBorderWidth2x() * 2),
		slit_h = screen->getSlit()->getHeight() +
			(screen->getBorderWidth2x() * 2),
#endif // SLIT
		toolbar_x = screen->getToolbar()->getX() - screen->getBorderWidth(),
		toolbar_y = screen->getToolbar()->getY() - screen->getBorderWidth(),
		toolbar_w = screen->getToolbar()->getWidth() +
			(screen->getBorderWidth2x() * 2),
		toolbar_h = screen->getToolbar()->getHeight() +
			(screen->getBorderWidth2x() * 2),
		place_x = 0, place_y = 0, change_x = 1, change_y = 1;

	if (screen->getColPlacementDirection() == BScreen::BOTTOMTOP)
		change_y = -1;
	if (screen->getRowPlacementDirection() == BScreen::RIGHTLEFT)
		change_x = -1;

	register int test_x, test_y, curr_w, curr_h;

	switch (screen->getPlacementPolicy()) {
	case BScreen::ROWSMARTPLACEMENT: {
		test_y = screen->getBorderWidth() + screen->getEdgeSnapThreshold();
		if (screen->getColPlacementDirection() == BScreen::BOTTOMTOP)
			test_y = screen->getHeight() - win_h - test_y;

		while (((screen->getColPlacementDirection() == BScreen::BOTTOMTOP) ?
			test_y > 0 :		test_y + win_h < (signed) screen->getHeight()) &&
		 ! placed) {
			test_x = screen->getBorderWidth() + screen->getEdgeSnapThreshold();
			if (screen->getRowPlacementDirection() == BScreen::RIGHTLEFT)
				test_x = screen->getWidth() - win_w - test_x;

			while (((screen->getRowPlacementDirection() == BScreen::RIGHTLEFT) ?
				test_x > 0 : test_x + win_w < (signed) screen->getWidth()) &&
			 ! placed) {
				placed = True;

					Windows::iterator it = windowList.begin();
					Windows::iterator it_end = windowList.end();
					for (; it != it_end && placed; ++it) {
					curr_w = (*it)->getWidth() + screen->getBorderWidth2x() +
						screen->getBorderWidth2x();
					curr_h =
						(((*it)->isShaded())
							? (*it)->getTitleHeight()
							: (*it)->getHeight()) +
							  screen->getBorderWidth2x() +
							  screen->getBorderWidth2x();

					if ((*it)->getXFrame() < test_x + win_w &&
							(*it)->getXFrame() + curr_w > test_x &&
							(*it)->getYFrame() < test_y + win_h &&
							(*it)->getYFrame() + curr_h > test_y)
						placed = False;
				}

				if ((toolbar_x < test_x + win_w &&
						 toolbar_x + toolbar_w > test_x &&
						 toolbar_y < test_y + win_h &&
						 toolbar_y + toolbar_h > test_y)
#ifdef		SLIT
						 ||
						(slit_x < test_x + win_w &&
						 slit_x + slit_w > test_x &&
						 slit_y < test_y + win_h &&
						 slit_y + slit_h > test_y)
#endif // SLIT
					)
					placed = False;

				if (placed) {
					place_x = test_x;
					place_y = test_y;

					break;
				}

				test_x += change_x;
			}

			test_y += change_y;
		}

		break; }

	case BScreen::COLSMARTPLACEMENT: {
		test_x = screen->getBorderWidth() + screen->getEdgeSnapThreshold();
		if (screen->getRowPlacementDirection() == BScreen::RIGHTLEFT)
			test_x = screen->getWidth() - win_w - test_x;

		while (((screen->getRowPlacementDirection() == BScreen::RIGHTLEFT) ?
			test_x > 0 : test_x + win_w < (signed) screen->getWidth()) &&
		 ! placed) {
			test_y = screen->getBorderWidth() + screen->getEdgeSnapThreshold();
			if (screen->getColPlacementDirection() == BScreen::BOTTOMTOP)
				test_y = screen->getHeight() - win_h - test_y;

			while (((screen->getColPlacementDirection() == BScreen::BOTTOMTOP) ?
				test_y > 0 : test_y + win_h < (signed) screen->getHeight()) &&
			 ! placed) {
				placed = True;

				Windows::iterator it = windowList.begin();
				Windows::iterator it_end = windowList.end();
				for (; it != it_end && placed; ++it) {
					curr_w = (*it)->getWidth() + screen->getBorderWidth2x() +
						screen->getBorderWidth2x();
					curr_h =
						(((*it)->isShaded())
							? (*it)->getTitleHeight()
							: (*it)->getHeight()) +
							  screen->getBorderWidth2x() +
								screen->getBorderWidth2x();

					if ((*it)->getXFrame() < test_x + win_w &&
							(*it)->getXFrame() + curr_w > test_x &&
							(*it)->getYFrame() < test_y + win_h &&
							(*it)->getYFrame() + curr_h > test_y)
						placed = False;
				}

				if ((toolbar_x < test_x + win_w &&
						toolbar_x + toolbar_w > test_x &&
						toolbar_y < test_y + win_h &&
						toolbar_y + toolbar_h > test_y)
#ifdef		SLIT
						||
						(slit_x < test_x + win_w &&
						slit_x + slit_w > test_x &&
						slit_y < test_y + win_h &&
						slit_y + slit_h > test_y)
#endif // SLIT
						)
					placed = False;

			if (placed) {
				place_x = test_x;
				place_y = test_y;

				break;
			}

			test_y += change_y;
		}

		test_x += change_x;
	 }

	 break; }
	}

	if (! placed) {
		if (((unsigned) cascade_x > (screen->getWidth() / 2)) ||
				((unsigned) cascade_y > (screen->getHeight() / 2)))
			cascade_x = cascade_y = 32;

		place_x = cascade_x;
		place_y = cascade_y;

		cascade_x += win->getTitleHeight();
		cascade_y += win->getTitleHeight();
	}

	if (place_x + win_w > (signed) screen->getWidth())
		place_x = (((signed) screen->getWidth()) - win_w) / 2;
	if (place_y + win_h > (signed) screen->getHeight())
		place_y = (((signed) screen->getHeight()) - win_h) / 2;

	win->configure(place_x, place_y, win->getWidth(), win->getHeight());
}
