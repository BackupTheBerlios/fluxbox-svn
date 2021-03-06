// Slit.cc for Blackbox - an X11 Window manager
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

// stupid macros needed to access some functions in version 2 of the GNU C
// library
#ifndef	 _GNU_SOURCE
#define	 _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef		HAVE_CONFIG_H
#	include "../config.h"
#endif // HAVE_CONFIG_H

#ifdef		SLIT

#include <X11/keysym.h>

#include "i18n.hh"
#include "fluxbox.hh"
#include "Image.hh"
#include "Screen.hh"
#include "Slit.hh"
#include "Toolbar.hh"

#include <algorithm>


Slit::Slit(BScreen *scr) {
	screen = scr;
	fluxbox = Fluxbox::instance();

	on_top = screen->isSlitOnTop();
	hidden = do_auto_hide = screen->doSlitAutoHide();

	display = screen->getBaseDisplay()->getXDisplay();
	frame.window = frame.pixmap = None;

	timer = new BTimer(fluxbox, this);
	timer->setTimeout(fluxbox->getAutoRaiseDelay());
	timer->fireOnce(True);

	slitmenu = new Slitmenu(this);

	XSetWindowAttributes attrib;
	unsigned long create_mask = CWBackPixmap | CWBackPixel | CWBorderPixel |
		CWColormap | CWOverrideRedirect | CWEventMask;
	attrib.background_pixmap = None;
	attrib.background_pixel = attrib.border_pixel =
		screen->getBorderColor()->getPixel();
	attrib.colormap = screen->getColormap();
	attrib.override_redirect = True;
	attrib.event_mask = SubstructureRedirectMask | ButtonPressMask |
											EnterWindowMask | LeaveWindowMask;

	frame.x = frame.y = 0;
	frame.width = frame.height = 1;

	frame.window =
		XCreateWindow(display, screen->getRootWindow(), frame.x, frame.y,
			frame.width, frame.height, screen->getBorderWidth(),
									screen->getDepth(), InputOutput, screen->getVisual(),
									create_mask, &attrib);
	fluxbox->saveSlitSearch(frame.window, this);

	reconfigure();
}


Slit::~Slit() {
	fluxbox->grab();

	delete timer;

	delete slitmenu;

	screen->getImageControl()->removeImage(frame.pixmap);

	fluxbox->removeSlitSearch(frame.window);

	XDestroyWindow(display, frame.window);

	fluxbox->ungrab();
}


void Slit::addClient(Window w) {
	fluxbox->grab();

	if (fluxbox->validateWindow(w)) {
		SlitClient *client = new SlitClient;
		client->client_window = w;

		XWMHints *wmhints = XGetWMHints(display, w);

		if (wmhints) {
			if ((wmhints->flags & IconWindowHint) &&
					(wmhints->icon_window != None)) {
				XMoveWindow(display, client->client_window, screen->getWidth() + 10,
					screen->getHeight() + 10);
				XMapWindow(display, client->client_window);				
				client->icon_window = wmhints->icon_window;
				client->window = client->icon_window;
			} else {
				client->icon_window = None;
				client->window = client->client_window;
			}

			XFree(wmhints);
		} else {
			client->icon_window = None;
			client->window = client->client_window;
		}
	#ifndef KDE 
		XWindowAttributes attrib;
		if (XGetWindowAttributes(display, client->window, &attrib)) {
			client->width = attrib.width;
			client->height = attrib.height;
		} else {
			client->width = client->height = 64;
		}
	#else //KDE stuff starts here
	  XWindowAttributes attrib;
		//Check and see if new client is a KDE dock applet
		//If so force reasonable size
		bool iskdedockapp=false;
		Atom ajunk;
		int ijunk;
		unsigned long *data = (unsigned long *) 0, uljunk;

		// Check if KDE v2.x dock applet
		if (XGetWindowProperty(fluxbox->getXDisplay(), w,
				fluxbox->getKWM2DockwindowAtom(), 0l, 1l, False,
				fluxbox->getKWM2DockwindowAtom(),
				&ajunk, &ijunk, &uljunk, &uljunk,
				(unsigned char **) &data) == Success) {
			iskdedockapp = (data && data[0] != 0);
			XFree((char *) data);
		}

		// Check if KDE v1.x dock applet
		if (!iskdedockapp) {
			if (XGetWindowProperty(fluxbox->getXDisplay(), w,
					fluxbox->getKWM1DockwindowAtom(), 0l, 1l, False,
					fluxbox->getKWM1DockwindowAtom(),
					&ajunk, &ijunk, &uljunk, &uljunk,
					(unsigned char **) &data) == Success) {
				iskdedockapp = (data && data[0] != 0);
	 		    XFree((char *) data);
	  	 	}
		}

		if (iskdedockapp)
			client->width = client->height = 24;
		else {
			if (XGetWindowAttributes(display, client->window, &attrib)) {
				client->width = attrib.width;
				client->height = attrib.height;
			}	else
				client->width = client->height = 64;
		}
	#endif // KDE

		XSetWindowBorderWidth(display, client->window, 0);

		XSelectInput(display, frame.window, NoEventMask);
		XSelectInput(display, client->window, NoEventMask);

		XReparentWindow(display, client->window, frame.window, 0, 0);
		XMapRaised(display, client->window);
		XChangeSaveSet(display, client->window, SetModeInsert);

		XSelectInput(display, frame.window, SubstructureRedirectMask |
			ButtonPressMask | EnterWindowMask | LeaveWindowMask);
		XSelectInput(display, client->window, StructureNotifyMask |
			SubstructureNotifyMask | EnterWindowMask);
		XFlush(display);

		clientList.push_back(client);

		fluxbox->saveSlitSearch(client->client_window, this);
		fluxbox->saveSlitSearch(client->icon_window, this);
		reconfigure();
	}

	fluxbox->ungrab();
}


void Slit::removeClient(SlitClient *client, bool remap) {
	fluxbox->removeSlitSearch(client->client_window);
	fluxbox->removeSlitSearch(client->icon_window);

    clientList.remove(client);

	screen->removeNetizen(client->window);

	if (remap && fluxbox->validateWindow(client->window)) {
		XSelectInput(display, frame.window, NoEventMask);
		XSelectInput(display, client->window, NoEventMask);
		XReparentWindow(display, client->window, screen->getRootWindow(),
			client->x, client->y);
		XChangeSaveSet(display, client->window, SetModeDelete);
		XSelectInput(display, frame.window, SubstructureRedirectMask |
			ButtonPressMask | EnterWindowMask | LeaveWindowMask);
		XFlush(display);
	}

	delete client;
}


void Slit::removeClient(Window w, bool remap) {
	fluxbox->grab();

	bool reconf = false;

	SlitClients::iterator it = clientList.begin();
	SlitClients::iterator it_end = clientList.end();
	for (; it != it_end; ++it) {
		if ((*it)->window == w) {
			removeClient((*it), remap);
			reconf = true;

			break;
		}
	}
	if (reconf) reconfigure();

	fluxbox->ungrab();
}


void Slit::reconfigure(void) {
	frame.width = 0;
	frame.height = 0;

	switch (screen->getSlitDirection()) {
	case VERTICAL:
		{
			SlitClients::iterator it = clientList.begin();
			SlitClients::iterator it_end = clientList.end();
			for (; it != it_end; ++it) {
				frame.height += (*it)->height + screen->getBevelWidth();

				if (frame.width < (*it)->width)
					frame.width = (*it)->width;
			}
		}

		if (frame.width < 1)
			frame.width = 1;
		else
			frame.width += (screen->getBevelWidth() * 2);

		if (frame.height < 1)
			frame.height = 1;
		else
			frame.height += screen->getBevelWidth();

		break;

	case HORIZONTAL:
		{
			SlitClients::iterator it = clientList.begin();
			SlitClients::iterator it_end = clientList.end();
			for (; it != it_end; ++it) {
				frame.width += (*it)->width + screen->getBevelWidth();

				if (frame.height < (*it)->height)
					frame.height = (*it)->height;
			}
		}

		if (frame.width < 1)
			frame.width = 1;
		else
			frame.width += screen->getBevelWidth();

		if (frame.height < 1)
			frame.height = 1;
		else
			frame.height += (screen->getBevelWidth() * 2);

		break;
	}

	reposition();

	XSetWindowBorderWidth(display ,frame.window, screen->getBorderWidth());
	XSetWindowBorder(display, frame.window,
		screen->getBorderColor()->getPixel());

	if (clientList.size()==0)
		XUnmapWindow(display, frame.window);
	else
		XMapWindow(display, frame.window);

	Pixmap tmp = frame.pixmap;
	BImageControl *image_ctrl = screen->getImageControl();
	BTexture *texture = &(screen->getToolbarStyle()->toolbar);
	if (texture->getTexture() == (BImage::FLAT | BImage::SOLID)) {
		frame.pixmap = None;
		XSetWindowBackground(display, frame.window,
			 texture->getColor()->getPixel());
	} else {
		frame.pixmap = image_ctrl->renderImage(frame.width, frame.height,
			texture);
		XSetWindowBackgroundPixmap(display, frame.window, frame.pixmap);
	}
	if (tmp) image_ctrl->removeImage(tmp);
	XClearWindow(display, frame.window);

	int x, y;

	switch (screen->getSlitDirection()) {
	case VERTICAL:
		x = 0;
		y = screen->getBevelWidth();

		{
			SlitClients::iterator it = clientList.begin();
			SlitClients::iterator it_end = clientList.end();
			for (; it != it_end; ++it) {
				x = (frame.width - (*it)->width) / 2;

				XMoveResizeWindow(display, (*it)->window, x, y,
					(*it)->width, (*it)->height);
				XMapWindow(display, (*it)->window);

				// for ICCCM compliance
				(*it)->x = x;
				(*it)->y = y;

				XEvent event;
				event.type = ConfigureNotify;

				event.xconfigure.display = display;
				event.xconfigure.event = (*it)->window;
				event.xconfigure.window = (*it)->window;
				event.xconfigure.x = x;
				event.xconfigure.y = y;
				event.xconfigure.width = (*it)->width;
				event.xconfigure.height = (*it)->height;
				event.xconfigure.border_width = 0;
				event.xconfigure.above = frame.window;
				event.xconfigure.override_redirect = False;

				XSendEvent(display, (*it)->window, False, StructureNotifyMask,
					&event);

				y += (*it)->height + screen->getBevelWidth();
			}
		}

		break;

	case HORIZONTAL:
		x = screen->getBevelWidth();
		y = 0;

		{
			SlitClients::iterator it = clientList.begin();
			SlitClients::iterator it_end = clientList.end();
			for (; it != it_end; ++it) {
				y = (frame.height - (*it)->height) / 2;

				XMoveResizeWindow(display, (*it)->window, x, y,
					(*it)->width, (*it)->height);
				XMapWindow(display, (*it)->window);

				// for ICCCM compliance
				(*it)->x = x;
				(*it)->y = y;

				XEvent event;
				event.type = ConfigureNotify;

				event.xconfigure.display = display;
				event.xconfigure.event = (*it)->window;
				event.xconfigure.window = (*it)->window;
				event.xconfigure.x = frame.x + x + screen->getBorderWidth();
				event.xconfigure.y = frame.y + y + screen->getBorderWidth();
				event.xconfigure.width = (*it)->width;
				event.xconfigure.height = (*it)->height;
				event.xconfigure.border_width = 0;
				event.xconfigure.above = frame.window;
				event.xconfigure.override_redirect = False;

				XSendEvent(display, (*it)->window, False, StructureNotifyMask,
					&event);

				x += (*it)->width + screen->getBevelWidth();
			}
		}

		break;
	}

	slitmenu->reconfigure();
}


void Slit::reposition(void) {
	// place the slit in the appropriate place
	switch (screen->getSlitPlacement()) {
	case TOPLEFT:
		frame.x = 0;
		frame.y = 0;
		if (screen->getSlitDirection() == VERTICAL) {
			frame.x_hidden = screen->getBevelWidth() -
				screen->getBorderWidth() - frame.width;
			frame.y_hidden = 0;
		} else {
			frame.x_hidden = 0;
			frame.y_hidden = screen->getBevelWidth() -
				screen->getBorderWidth() - frame.height;
		}
		break;

	case CENTERLEFT:
		frame.x = 0;
		frame.y = (screen->getHeight() - frame.height) / 2;
		frame.x_hidden = screen->getBevelWidth() -
			screen->getBorderWidth() - frame.width;
		frame.y_hidden = frame.y;
		break;

	case BOTTOMLEFT:
		frame.x = 0;
		frame.y = screen->getHeight() - frame.height - screen->getBorderWidth2x();
		if (screen->getSlitDirection() == VERTICAL) {
			frame.x_hidden = screen->getBevelWidth() - screen->getBorderWidth()
								 - frame.width;
			frame.y_hidden = frame.y;
		} else {
			frame.x_hidden = 0;
			frame.y_hidden = screen->getHeight() -
				screen->getBevelWidth() - screen->getBorderWidth();
		}
		break;

	case TOPCENTER:
		frame.x = (screen->getWidth() - frame.width) / 2;
		frame.y = 0;
		frame.x_hidden = frame.x;
		frame.y_hidden = screen->getBevelWidth() -
			screen->getBorderWidth() - frame.height;
		break;

	case BOTTOMCENTER:
		frame.x = (screen->getWidth() - frame.width) / 2;
		frame.y = screen->getHeight() - frame.height - screen->getBorderWidth2x();
		frame.x_hidden = frame.x;
		frame.y_hidden = screen->getHeight() -
			screen->getBevelWidth() - screen->getBorderWidth();
		break;

	case TOPRIGHT:
		frame.x = screen->getWidth() - frame.width - screen->getBorderWidth2x();
		frame.y = 0;
		if (screen->getSlitDirection() == VERTICAL) {
			frame.x_hidden = screen->getWidth() -
				screen->getBevelWidth() - screen->getBorderWidth();
			frame.y_hidden = 0;
		} else {
			frame.x_hidden = frame.x;
			frame.y_hidden = screen->getBevelWidth() -
				screen->getBorderWidth() - frame.height;
		}
		break;

	case CENTERRIGHT:
	default:
		frame.x = screen->getWidth() - frame.width - screen->getBorderWidth2x();
		frame.y = (screen->getHeight() - frame.height) / 2;
		frame.x_hidden = screen->getWidth() -
			screen->getBevelWidth() - screen->getBorderWidth();
		frame.y_hidden = frame.y;
		break;

	case BOTTOMRIGHT:
		frame.x = screen->getWidth() - frame.width - screen->getBorderWidth2x();
		frame.y = screen->getHeight() - frame.height - screen->getBorderWidth2x();
		if (screen->getSlitDirection() == VERTICAL) {
			frame.x_hidden = screen->getWidth() - 
				screen->getBevelWidth() - screen->getBorderWidth();
			frame.y_hidden = frame.y;
		} else {
			frame.x_hidden = frame.x;
			frame.y_hidden = screen->getHeight() - 
				screen->getBevelWidth() - screen->getBorderWidth();
		}
		break;
	}

	Toolbar *tbar = screen->getToolbar();
	int sw = frame.width + screen->getBorderWidth2x(),
		sh = frame.height + screen->getBorderWidth2x(),
		tw = tbar->getWidth() + screen->getBorderWidth(),
		th = tbar->getHeight() + screen->getBorderWidth();

	if (tbar->getX() < frame.x + sw && tbar->getX() + tw > frame.x &&
			tbar->getY() < frame.y + sh && tbar->getY() + th > frame.y) {
		if (frame.y < th) {
			frame.y += tbar->getExposedHeight();
			if (screen->getSlitDirection() == VERTICAL)
				frame.y_hidden += tbar->getExposedHeight();
			else
				frame.y_hidden = frame.y;
		} else {
			frame.y -= tbar->getExposedHeight();
			if (screen->getSlitDirection() == VERTICAL)
				frame.y_hidden -= tbar->getExposedHeight();
			else
				frame.y_hidden = frame.y;
		}
	}

	if (hidden)
		XMoveResizeWindow(display, frame.window, frame.x_hidden,
			frame.y_hidden, frame.width, frame.height);
	else
		XMoveResizeWindow(display, frame.window, frame.x,
			frame.y, frame.width, frame.height);
}


void Slit::shutdown(void) {
	while (clientList.size() != 0)
		removeClient(clientList.front());
}


void Slit::buttonPressEvent(XButtonEvent *e) {
	if (e->window != frame.window) return;

	if (e->button == Button1 && (! on_top)) {
		Window w[1] = { frame.window };
		screen->raiseWindows(w, 1);
	} else if (e->button == Button2 && (! on_top)) {
		XLowerWindow(display, frame.window);
	} else if (e->button == Button3) {
		if (! slitmenu->isVisible()) {
			int x, y;

			x = e->x_root - (slitmenu->getWidth() / 2);
			y = e->y_root - (slitmenu->getHeight() / 2);

			if (x < 0)
				x = 0;
			else if (x + slitmenu->getWidth() > screen->getWidth())
				x = screen->getWidth() - slitmenu->getWidth();

			if (y < 0)
				y = 0;
			else if (y + slitmenu->getHeight() > screen->getHeight())
				y = screen->getHeight() - slitmenu->getHeight();

			slitmenu->move(x, y);
			slitmenu->show();
		} else
			slitmenu->hide();
	}
}


void Slit::enterNotifyEvent(XCrossingEvent *) {
	if (! do_auto_hide)
		return;

	if (hidden) {
		if (! timer->isTiming()) timer->start();
	} else {
		if (timer->isTiming()) timer->stop();
	}
}


void Slit::leaveNotifyEvent(XCrossingEvent *) {
	if (! do_auto_hide)
		return;

	if (hidden) {
		if (timer->isTiming()) timer->stop();
	} else if (! slitmenu->isVisible()) {
		if (! timer->isTiming()) timer->start();
	}
}


void Slit::configureRequestEvent(XConfigureRequestEvent *e) {
	fluxbox->grab();

	if (fluxbox->validateWindow(e->window)) {
		bool reconf = false;
		XWindowChanges xwc;

		xwc.x = e->x;
		xwc.y = e->y;
		xwc.width = e->width;
		xwc.height = e->height;
		xwc.border_width = 0;
		xwc.sibling = e->above;
		xwc.stack_mode = e->detail;

		XConfigureWindow(display, e->window, e->value_mask, &xwc);

		SlitClients::iterator it = clientList.begin();
		SlitClients::iterator it_end = clientList.end();
		for (; it != it_end; ++it)
			if ((*it)->window == e->window)
				if ((*it)->width != ((unsigned) e->width) ||
						(*it)->height != ((unsigned) e->height)) {
					(*it)->width = (unsigned) e->width;
					(*it)->height = (unsigned) e->height;

					reconf = true;

					break;
				}

		if (reconf) reconfigure();

	}

	fluxbox->ungrab();
}


void Slit::timeout(void) {
	hidden = ! hidden;
	if (hidden)
		XMoveWindow(display, frame.window, frame.x_hidden, frame.y_hidden);
	else
		XMoveWindow(display, frame.window, frame.x, frame.y);
}


Slitmenu::Slitmenu(Slit *sl) : Basemenu(sl->screen) {
	slit = sl;
	I18n *i18n = I18n::instance();
	
	setLabel(i18n->getMessage(
#ifdef		NLS
					SlitSet, SlitSlitTitle,
#else // !NLS
					0, 0,
#endif // NLS
					"Slit"));
	setInternalMenu();

	directionmenu = new Directionmenu(this);
	placementmenu = new Placementmenu(this);

	insert(i18n->getMessage(
#ifdef		NLS
				CommonSet, CommonDirectionTitle,
#else // !NLS
				0, 0,
#endif // NLS
				"Direction"),
	 directionmenu);
	insert(i18n->getMessage(
#ifdef		NLS
				CommonSet, CommonPlacementTitle,
#else // !NLS
				0, 0,
#endif // NLS
				"Placement"),
	 placementmenu);
	insert(i18n->getMessage(
#ifdef		NLS
				CommonSet, CommonAlwaysOnTop,
#else // !NLS
				0, 0,
#endif // NLS
				"Always on top"), 1);
	insert(i18n->getMessage(
#ifdef		NLS
				CommonSet, CommonAutoHide,
#else // !NLS
				0, 0,
#endif // NLS
				"Auto hide"), 2);

	update();

	if (slit->isOnTop()) setItemSelected(2, True);
	if (slit->doAutoHide()) setItemSelected(3, True);
}


Slitmenu::~Slitmenu(void) {
	delete directionmenu;
	delete placementmenu;
}


void Slitmenu::itemSelected(int button, int index) {
	if (button == 1) {
		BasemenuItem *item = find(index);
		if (! item) return;

		switch (item->function()) {
		case 1: // always on top
			{
	Bool change = ((slit->isOnTop()) ?	False : True);
	slit->on_top = change;
	setItemSelected(2, change);

	if (slit->isOnTop()) slit->screen->raiseWindows((Window *) 0, 0);
	break;
			}

		case 2: // auto hide
			{
	Bool change = ((slit->doAutoHide()) ?	False : True);
	slit->do_auto_hide = change;
	setItemSelected(3, change);

	break;
			}
		}
	}
}


void Slitmenu::internal_hide(void) {
	Basemenu::internal_hide();
	if (slit->doAutoHide())
		slit->timeout();
}


void Slitmenu::reconfigure(void) {
	directionmenu->reconfigure();
	placementmenu->reconfigure();

	Basemenu::reconfigure();
}


Slitmenu::Directionmenu::Directionmenu(Slitmenu *sm) : Basemenu(sm->slit->screen) {
	slitmenu = sm;
	I18n *i18n = I18n::instance();
	
	setLabel(i18n->getMessage(
#ifdef		NLS
					SlitSet, SlitSlitDirection,
#else // !NLS
					0, 0,
#endif // NLS
					"Slit Direction"));
	setInternalMenu();

	insert(i18n->getMessage(
#ifdef		NLS
				CommonSet, CommonDirectionHoriz,
#else // !NLS
				0, 0,
#endif // NLS
				"Horizontal"),
	 Slit::HORIZONTAL);
	insert(i18n->getMessage(
#ifdef		NLS
				CommonSet, CommonDirectionVert,
#else // !NLS
				0, 0,
#endif // NLS
				"Vertical"),
	 Slit::VERTICAL);

	update();

	if (sm->slit->screen->getSlitDirection() == Slit::HORIZONTAL)
		setItemSelected(0, True);
	else
		setItemSelected(1, True);
}


void Slitmenu::Directionmenu::itemSelected(int button, int index) {
	if (button == 1) {
		BasemenuItem *item = find(index);
		if (! item) return;

		slitmenu->slit->screen->saveSlitDirection(item->function());

		if (item->function() == Slit::HORIZONTAL) {
			setItemSelected(0, True);
			setItemSelected(1, False);
		} else {
			setItemSelected(0, False);
			setItemSelected(1, True);
		}

		hide();
		slitmenu->slit->reconfigure();
	}
}


Slitmenu::Placementmenu::Placementmenu(Slitmenu *sm) : Basemenu(sm->slit->screen) {
	slitmenu = sm;
	I18n *i18n = I18n::instance();
	
	setLabel(i18n->getMessage(
#ifdef		NLS
					SlitSet, SlitSlitPlacement,
#else // !NLS
					0, 0,
#endif // NLS
					"Slit Placement"));
	setMinimumSublevels(3);
	setInternalMenu();

	insert(i18n->getMessage(
#ifdef		NLS
				CommonSet, CommonPlacementTopLeft,
#else // !NLS
				0, 0,
#endif // NLS
				"Top Left"),
	 Slit::TOPLEFT);
	insert(i18n->getMessage(
#ifdef		NLS
				CommonSet, CommonPlacementCenterLeft,
#else // !NLS
				0, 0,
#endif // NLS
				"Center Left"),
	 Slit::CENTERLEFT);
	insert(i18n->getMessage(
#ifdef		NLS
				CommonSet, CommonPlacementBottomLeft,
#else // !NLS
				0, 0,
#endif // NLS
				"Bottom Left"),
	 Slit::BOTTOMLEFT);
	insert(i18n->getMessage(
#ifdef		NLS
				CommonSet, CommonPlacementTopCenter,
#else // !NLS
				0, 0,
#endif // NLS
				"Top Center"),
	 Slit::TOPCENTER);
	insert("");
	insert(i18n->getMessage(
#ifdef		NLS
				CommonSet, CommonPlacementBottomCenter,
#else // !NLS
				0, 0,
#endif // NLS
				"Bottom Center"),
	 Slit::BOTTOMCENTER);
	insert(i18n->getMessage(
#ifdef		NLS
				CommonSet, CommonPlacementTopRight,
#else // !NLS
				0, 0,
#endif // NLS
				"Top Right"),
	 Slit::TOPRIGHT);
	insert(i18n->getMessage(
#ifdef		NLS
				CommonSet, CommonPlacementCenterRight,
#else // !NLS
				0, 0,
#endif // NLS
				"Center Right"),
	 Slit::CENTERRIGHT);
	insert(i18n->getMessage(
#ifdef		NLS
				CommonSet, CommonPlacementBottomRight,
#else // !NLS
				0, 0,
#endif // NLS
				"Bottom Right"),
	 Slit::BOTTOMRIGHT);

	update();
}


void Slitmenu::Placementmenu::itemSelected(int button, int index) {
	if (button == 1) {
		BasemenuItem *item = find(index);
		if (! item) return;

		if (item->function()) {
			slitmenu->slit->screen->saveSlitPlacement(item->function());
			hide();
			slitmenu->slit->reconfigure();
		}
	}
}


#endif // SLIT
