// Window.cc for Fluxbox Window Manager
// Copyright (c) 2001 - 2002 Henrik Kinnunen (fluxgen@linuxmail.org)
//
// Window.cc for Blackbox - an X11 Window manager
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

// $Id: Window.cc,v 1.100 2002/11/17 12:50:20 fluxgen Exp $

#include "Window.hh"

#include "i18n.hh"
#include "fluxbox.hh"
#include "Iconmenu.hh"
#include "Screen.hh"
#include "Toolbar.hh"
#include "Windowmenu.hh"
#include "StringUtil.hh"
#include "Netizen.hh"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef SLIT
#include "Slit.hh"
#endif // SLIT

//use GNU extensions
#ifndef	 _GNU_SOURCE
#define	 _GNU_SOURCE
#endif // _GNU_SOURCE

#include <X11/Xatom.h>
#include <X11/keysym.h>

#include <cstring>
#include <cstdio>
#include <iostream>

using namespace std;

FluxboxWindow::FluxboxWindow(Window w, BScreen *s):
m_hintsig(*this),
m_statesig(*this),
m_workspacesig(*this),
image_ctrl(0),
moving(false), resizing(false), shaded(false), maximized(false),
visible(false), iconic(false), transient(false), focused(false),
stuck(false), modal(false), send_focus_message(false), managed(false),
screen(0),
timer(this),
display(0),
lastButtonPressTime(0),
m_windowmenu(0),
m_layer(LAYER_NORMAL),
tab(0) {
	
	lastFocusTime.tv_sec = lastFocusTime.tv_usec = 0;
#ifdef DEBUG
	fprintf(stderr,
		I18n::instance()->
		getMessage(
			FBNLS::WindowSet, FBNLS::WindowCreating,
			"FluxboxWindow::FluxboxWindow(): creating 0x%lx\n"),
			w);

#endif //DEBUG

	Fluxbox *fluxbox = Fluxbox::instance();
	display = fluxbox->getXDisplay();
	
	blackbox_attrib.workspace = workspace_number = window_number = -1;

	blackbox_attrib.flags = blackbox_attrib.attrib = blackbox_attrib.stack = 0l;
	blackbox_attrib.premax_x = blackbox_attrib.premax_y = 0;
	blackbox_attrib.premax_w = blackbox_attrib.premax_h = 0;
	//use tab as default
	decorations.tab = true;
	client.window = w;
	frame.window = frame.plate = frame.title = frame.handle = None;
	frame.right_grip = frame.left_grip = None;

	frame.utitle = frame.ftitle = frame.uhandle = frame.fhandle = None;
	frame.ulabel = frame.flabel = frame.ubutton = frame.fbutton = None;
	frame.pbutton = frame.ugrip = frame.fgrip = None;

	//get decorations
	vector<Fluxbox::Titlebar> dir = fluxbox->getTitlebarLeft();
	for (char c=0; c<2; c++) {
		for (unsigned int i=0; i<dir.size(); i++) {
			switch (dir[i]) {
				case Fluxbox::SHADE:
					decorations.shade = true;
				break;
				case Fluxbox::MAXIMIZE:
					decorations.maximize = true;	
				break;
				case Fluxbox::MINIMIZE:
					decorations.iconify = true;			
				break;
				case Fluxbox::STICK:
					decorations.sticky = true;
				break;
				case Fluxbox::CLOSE:
					decorations.close = true;
				break;
				case Fluxbox::MENU:
					decorations.menu = true;
				break;
				default:
				break;
			}
		}
		//next right
		dir = fluxbox->getTitlebarRight();
	}
	
	decorations.menu = true;	//override menu option
	
	decorations.titlebar = decorations.border = decorations.handle = true;
	functions.resize = functions.move = functions.iconify = functions.maximize = true;
	functions.close = decorations.close = false;

	client.wm_hint_flags = client.normal_hint_flags = 0;
	client.transient_for = 0;
	client.mwm_hint = (MwmHints *) 0;
	client.blackbox_hint = 0;

	fluxbox->grab();
	if (! validateClient())
		return;

	// fetch client size and placement
	XWindowAttributes wattrib;
	if ((! XGetWindowAttributes(display, client.window, &wattrib)) ||
			(! wattrib.screen) || wattrib.override_redirect) {
		fluxbox->ungrab();
		return;
	}

	if (s)
		screen = s;
	else
		screen = fluxbox->searchScreen(RootWindowOfScreen(wattrib.screen));

	if (!screen) {
		fluxbox->ungrab();
		return;
	}
	
	image_ctrl = screen->getImageControl();

	client.x = wattrib.x;
	client.y = wattrib.y;
	client.width = wattrib.width;
	client.height = wattrib.height;
	client.old_bw = wattrib.border_width;


	timer.setTimeout(fluxbox->getAutoRaiseDelay());
	timer.fireOnce(true);

	getBlackboxHints();
	if (! client.blackbox_hint)
		getMWMHints();

	// get size, aspect, minimum/maximum size and other hints set by the
	// client
	getWMProtocols();
	getWMHints();
	getWMNormalHints();
#ifdef SLIT

	if (client.initial_state == WithdrawnState) {
		screen->getSlit()->addClient(client.window);
		fluxbox->ungrab();
		return;
	}
#endif // SLIT

	managed = true; //mark for usage

	fluxbox->saveWindowSearch(client.window, this);

	// determine if this is a transient window
	checkTransient();
	
	// adjust the window decorations based on transience and window sizes
	if (transient) {
		decorations.maximize =  functions.maximize = false;
		decorations.handle = decorations.border = false;
	}	
	
	if ((client.normal_hint_flags & PMinSize) &&
			(client.normal_hint_flags & PMaxSize) &&
			client.max_width != 0 && client.max_width <= client.min_width &&
			client.max_height != 0 && client.max_height <= client.min_height) {
		decorations.maximize = decorations.handle =
			functions.resize = functions.maximize = false;
			decorations.tab = false; //no tab for this window
	}

	upsize();
	
	bool place_window = true;
	if (fluxbox->isStartup() || transient ||
			client.normal_hint_flags & (PPosition|USPosition)) {
		setGravityOffsets();

		if (! fluxbox->isStartup()) {
#ifdef XINERAMA
			unsigned int head = 0;
			if (screen->hasXinerama()) {
				head = screen->getHead(frame.x, frame.y);
			}
#endif // XINERAMA

			int real_x = frame.x;
			int real_y = frame.y;

			if (decorations.tab) {
				if (screen->getTabPlacement() == Tab::PTOP) {
					real_y -= screen->getTabHeight();
				} else if (screen->getTabPlacement() == Tab::PLEFT) {
					real_x -= (screen->isTabRotateVertical())
						? screen->getTabHeight()
						: screen->getTabWidth();
				}
			}

#ifdef XINERAMA
			// check is within the current head, so it won't overlap heads
			if (real_x >= screen->getHeadX(head) && 
					real_y + frame.y_border >= screen->getHeadY(head) &&
					(real_x + frame.width) <=
						(screen->getHeadX(head) + screen->getHeadWidth(head)) &&
					(real_y + frame.height) <=
						(screen->getHeadY(head) + screen->getHeadHeight(head)) )
				place_window = false;
#else // !XINERAMA
			if (real_x >= 0 && 
					real_y + frame.y_border >= 0 &&
					real_x <= (signed) screen->getWidth() &&
					real_y <= (signed) screen->getHeight())
				place_window = false;
#endif // XIENRAMA

		} else
			place_window = false;

	}

	frame.window = createToplevelWindow(frame.x, frame.y, frame.width,
		frame.height,
		screen->getBorderWidth());	//create frame window
	
	fluxbox->saveWindowSearch(frame.window, this);	//save frame window

	frame.plate = createChildWindow(frame.window); //Create plate window
	fluxbox->saveWindowSearch(frame.plate, this);	//save plate window
	
	createTitlebar();
	createHandle();

	associateClientWindow();

	grabButtons();
				
	positionWindows();

	// use tab? and don't create a tab on windows that's not
	// maximizable as default (such as dialogs)
	if (decorations.tab && fluxbox->useTabs() && isGroupable())
		tab = new Tab(this, 0, 0);

	decorate();
	
	XRaiseWindow(display, frame.plate);
	XMapSubwindows(display, frame.plate);
	if (decorations.titlebar)
		XMapSubwindows(display, frame.title);
	
	XMapSubwindows(display, frame.window);

	if (decorations.menu) {
		std::auto_ptr<Windowmenu> tmp(new Windowmenu(*this));
		m_windowmenu = tmp;
	}

	if (workspace_number < 0 || workspace_number >= screen->getCount())
		screen->getCurrentWorkspace()->addWindow(this, place_window);
	else
		screen->getWorkspace(workspace_number)->addWindow(this, place_window);

	configure(frame.x, frame.y, frame.width, frame.height);

	if (shaded) {
		shaded = false;
		shade();
	}

	if (maximized && functions.maximize) {
		int m = maximized;
		maximized = false;
		maximize(m);
	}	

	if (stuck) {
		stuck = false;
		stick();
		deiconify(); //omnipresent, so show it
	}

	setFocusFlag(false);

#ifdef DEBUG
	fprintf(stderr, "%s(%d): FluxboxWindow(this=%p)\n", __FILE__, __LINE__, this);
#endif // DEBUG

	fluxbox->ungrab();
}


FluxboxWindow::~FluxboxWindow() {
	if (screen == 0) //the window wasn't created 
		return;
	timer.stop();

	Fluxbox *fluxbox = Fluxbox::instance();
	
	if (moving || resizing) {
		screen->hideGeometry();
		XUngrabPointer(display, CurrentTime);
	}
	
	if (!iconic) {
		Workspace *workspace = screen->getWorkspace(workspace_number);
		if (workspace)
			workspace->removeWindow(this);
	} else //it's iconic
		screen->removeIcon(this);
	
	if (tab != 0) {
		delete tab;	
		tab = 0;
	}
	
	if (client.mwm_hint != 0) {
		XFree(client.mwm_hint);
		client.mwm_hint = 0;
	}

	if (client.blackbox_hint != 0) {
		XFree(client.blackbox_hint);
		client.blackbox_hint = 0;
	}


	if (client.transient_for != 0) {
		if (client.transient_for == this) {
			client.transient_for = 0;
		}

		fluxbox->setFocusedWindow(client.transient_for);

		if (client.transient_for) {
			client.transient_for->client.transients.remove(this);			
			client.transient_for->setInputFocus();
			client.transient_for = 0;
		}
	}
	
	while (!client.transients.empty()) {
		client.transients.back()->client.transient_for = 0;
		client.transients.pop_back();
	}
	
	if (client.window_group) {
		fluxbox->removeGroupSearch(client.window_group);
		client.window_group = 0;
	}
		
	destroyTitlebar();	

	destroyHandle();

	if (frame.fbutton) {
		image_ctrl->removeImage(frame.fbutton);
		frame.fbutton = 0;
	}

	if (frame.ubutton) {
		image_ctrl->removeImage(frame.ubutton);
		frame.ubutton = 0;
	}

	if (frame.pbutton) {
		image_ctrl->removeImage(frame.pbutton);
		frame.pbutton = 0;
	}

	
	if (frame.plate) {
		fluxbox->removeWindowSearch(frame.plate);
		XDestroyWindow(display, frame.plate);
		frame.plate = 0;
	}

	if (frame.window) {
		fluxbox->removeWindowSearch(frame.window);
		XDestroyWindow(display, frame.window);
		frame.window = 0;
	}

	fluxbox->removeWindowSearch(client.window);

#ifdef DEBUG
	cerr<<__FILE__<<"("<<__LINE__<<"): ~FluxboxWindow("<<this<<")"<<endl;
#endif // DEBUG
}

bool FluxboxWindow::isGroupable() const {
	if (isResizable() && isMaximizable() && !isTransient())
		return true;
	return false;
}

Window FluxboxWindow::createToplevelWindow(
	int x, int y, unsigned int width,
	unsigned int height,
	unsigned int borderwidth)
{
	XSetWindowAttributes attrib_create;
	unsigned long create_mask = CWBackPixmap | CWBorderPixel | CWColormap |
		CWOverrideRedirect | CWEventMask;

	attrib_create.background_pixmap = None;
	attrib_create.colormap = screen->colormap();
	attrib_create.override_redirect = True;
	attrib_create.event_mask = ButtonPressMask | ButtonReleaseMask |
		ButtonMotionMask | EnterWindowMask;

	return (XCreateWindow(display, screen->getRootWindow(), x, y, width, height,
		borderwidth, screen->getDepth(), InputOutput,
		screen->getVisual(), create_mask,
		&attrib_create));
}


Window FluxboxWindow::createChildWindow(Window parent, Cursor cursor) {
	XSetWindowAttributes attrib_create;
	unsigned long create_mask = CWBackPixmap | CWBorderPixel | CWEventMask;

	attrib_create.background_pixmap = None;
	attrib_create.event_mask = ButtonPressMask | ButtonReleaseMask |
		ButtonMotionMask | ExposureMask |
		EnterWindowMask | LeaveWindowMask;

	if (cursor) {
		create_mask |= CWCursor;
		attrib_create.cursor = cursor;
	}

	return (XCreateWindow(display, parent, 0, 0, 1, 1, 0,
		screen->getDepth(), InputOutput, screen->getVisual(),
		create_mask, &attrib_create));
}


void FluxboxWindow::associateClientWindow() {
	XSetWindowBorderWidth(display, client.window, 0);
	getWMName();
	getWMIconName();

	XChangeSaveSet(display, client.window, SetModeInsert);
	XSetWindowAttributes attrib_set;

	XSelectInput(display, frame.plate, NoEventMask);
	XReparentWindow(display, client.window, frame.plate, 0, 0);
	XSelectInput(display, frame.plate, SubstructureRedirectMask);

	XFlush(display);

	attrib_set.event_mask = 
		PropertyChangeMask | StructureNotifyMask | FocusChangeMask;
	attrib_set.do_not_propagate_mask = 
		ButtonPressMask | ButtonReleaseMask | ButtonMotionMask;

	XChangeWindowAttributes(display, client.window, CWEventMask|CWDontPropagate,
		&attrib_set);

#ifdef		SHAPE
	if (Fluxbox::instance()->hasShapeExtensions()) {
		XShapeSelectInput(display, client.window, ShapeNotifyMask);

		int foo;
		unsigned int ufoo;

		XShapeQueryExtents(display, client.window, &frame.shaped, &foo, &foo,
			&ufoo, &ufoo, &foo, &foo, &foo, &ufoo, &ufoo);

		if (frame.shaped) {
			XShapeCombineShape(display, frame.window, ShapeBounding,
				frame.mwm_border_w, frame.y_border +
				frame.mwm_border_w, client.window,
				ShapeBounding, ShapeSet);

			int num = 1;
			XRectangle xrect[2];
			xrect[0].x = xrect[0].y = 0;
			xrect[0].width = frame.width;
			xrect[0].height = frame.y_border;

			if (decorations.handle) {
				xrect[1].x = 0;
				xrect[1].y = frame.y_handle;
				xrect[1].width = frame.width;
				xrect[1].height = frame.handle_h + screen->getBorderWidth();
				num++;
			}

			XShapeCombineRectangles(display, frame.window, ShapeBounding, 0, 0,
				xrect, num, ShapeUnion, Unsorted);
		}
	}
#endif // SHAPE
	//create the buttons
	if (decorations.iconify) 
		createButton(Fluxbox::MINIMIZE, FluxboxWindow::iconifyPressed_cb, 
			FluxboxWindow::iconifyButton_cb, FluxboxWindow::iconifyDraw_cb);
	if (decorations.maximize)
		createButton(Fluxbox::MAXIMIZE, FluxboxWindow::maximizePressed_cb, 
			FluxboxWindow::maximizeButton_cb, FluxboxWindow::maximizeDraw_cb);
	if (decorations.close) 
		createButton(Fluxbox::CLOSE, FluxboxWindow::closePressed_cb, 
			FluxboxWindow::closeButton_cb, FluxboxWindow::closeDraw_cb);		
	if (decorations.sticky)
		createButton(Fluxbox::STICK, FluxboxWindow::stickyPressed_cb, 
			FluxboxWindow::stickyButton_cb, FluxboxWindow::stickyDraw_cb);

	if (decorations.menu)//TODO
		createButton(Fluxbox::MENU, 0, 0, 0);

	if (decorations.shade)
		createButton(Fluxbox::SHADE, 0, FluxboxWindow::shadeButton_cb, FluxboxWindow::shadeDraw_cb);

	if (frame.ubutton) {
		for (unsigned int i=0; i<buttonlist.size(); i++)
			XSetWindowBackgroundPixmap(display, buttonlist[i].win, frame.ubutton);	 
	
	} else {
		for (unsigned int i=0; i<buttonlist.size(); i++)
			XSetWindowBackground(display, buttonlist[i].win, frame.ubutton_pixel);	 
	}
}


void FluxboxWindow::decorate() {
		
	if (tab)
		tab->decorate();

	Pixmap tmp = frame.fbutton;
	FbTk::Texture *texture = &(screen->getWindowStyle()->b_focus);
	if (texture->type() == (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
		frame.fbutton = None;
		frame.fbutton_pixel = texture->color().pixel();
	} else
		frame.fbutton =
			image_ctrl->renderImage(frame.button_w, frame.button_h, texture);
	if (tmp) image_ctrl->removeImage(tmp);

	tmp = frame.ubutton;
	texture = &(screen->getWindowStyle()->b_unfocus);
	if (texture->type() == (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
		frame.ubutton = None;
		frame.ubutton_pixel = texture->color().pixel();
	} else
		frame.ubutton =
			image_ctrl->renderImage(frame.button_w, frame.button_h, texture);
	if (tmp) image_ctrl->removeImage(tmp);

	tmp = frame.pbutton;
	texture = &(screen->getWindowStyle()->b_pressed);
	if (texture->type() == (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
		frame.pbutton = None;
		frame.pbutton_pixel = texture->color().pixel();
	} else
		frame.pbutton =
			image_ctrl->renderImage(frame.button_w, frame.button_h, texture);
	if (tmp) image_ctrl->removeImage(tmp);

	if (decorations.titlebar) {
		tmp = frame.ftitle;
		texture = &(screen->getWindowStyle()->t_focus);
		if (texture->type() == (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
			frame.ftitle = None;
			frame.ftitle_pixel = texture->color().pixel();
		} else
			frame.ftitle =
				image_ctrl->renderImage(frame.width, frame.title_h, texture);
		
		if (tmp) 
			image_ctrl->removeImage(tmp);

		tmp = frame.utitle;
		texture = &(screen->getWindowStyle()->t_unfocus);
		if (texture->type() == (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
			frame.utitle = None;
			frame.utitle_pixel = texture->color().pixel();
		} else
			frame.utitle =
				image_ctrl->renderImage(frame.width, frame.title_h, texture);
		if (tmp) image_ctrl->removeImage(tmp);

		XSetWindowBorder(display, frame.title,
			 screen->getBorderColor()->pixel());

		decorateLabel();
		
	}

	if (decorations.border) {
		frame.fborder_pixel = screen->getWindowStyle()->f_focus.pixel();
		frame.uborder_pixel = screen->getWindowStyle()->f_unfocus.pixel();
	}

	if (decorations.handle) {
		tmp = frame.fhandle;
		texture = &(screen->getWindowStyle()->h_focus);
		if (texture->type() == (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
			frame.fhandle = None;
			frame.fhandle_pixel = texture->color().pixel();
		} else
			frame.fhandle =
				image_ctrl->renderImage(frame.width, frame.handle_h, texture);
		if (tmp) image_ctrl->removeImage(tmp);

		tmp = frame.uhandle;
		texture = &(screen->getWindowStyle()->h_unfocus);
		if (texture->type() == (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
			frame.uhandle = None;
			frame.uhandle_pixel = texture->color().pixel();
		} else
			frame.uhandle =
				image_ctrl->renderImage(frame.width, frame.handle_h, texture);
		if (tmp)
			image_ctrl->removeImage(tmp);

		tmp = frame.fgrip;
		texture = &(screen->getWindowStyle()->g_focus);
		if (texture->type() == (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
			frame.fgrip = None;
			frame.fgrip_pixel = texture->color().pixel();
		} else
			frame.fgrip =
				image_ctrl->renderImage(frame.grip_w, frame.grip_h, texture);
		if (tmp)
			image_ctrl->removeImage(tmp);

		tmp = frame.ugrip;
		texture = &(screen->getWindowStyle()->g_unfocus);
		if (texture->type() == (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
			frame.ugrip = None;
			frame.ugrip_pixel = texture->color().pixel();
		} else
			frame.ugrip =
				image_ctrl->renderImage(frame.grip_w, frame.grip_h, texture);
		if (tmp) image_ctrl->removeImage(tmp);

		XSetWindowBorder(display, frame.handle,
			screen->getBorderColor()->pixel());
		XSetWindowBorder(display, frame.left_grip,
			screen->getBorderColor()->pixel());
		XSetWindowBorder(display, frame.right_grip,
			screen->getBorderColor()->pixel());
	}
		
	XSetWindowBorder(display, frame.window,
		screen->getBorderColor()->pixel());
}


void FluxboxWindow::decorateLabel() {
	Pixmap tmp = frame.flabel;
	FbTk::Texture *texture = &(screen->getWindowStyle()->l_focus);
	if (texture->type() == (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
		frame.flabel = None;
		frame.flabel_pixel = texture->color().pixel();		
	} else
		frame.flabel = image_ctrl->renderImage(frame.label_w, frame.label_h, texture);
			
	if (tmp) image_ctrl->removeImage(tmp);

	tmp = frame.ulabel;
	texture = &(screen->getWindowStyle()->l_unfocus);
	if (texture->type() == (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
		frame.ulabel = None;
		frame.ulabel_pixel = texture->color().pixel();
	} else
		frame.ulabel = image_ctrl->renderImage(frame.label_w, frame.label_h, texture);
		
	if (tmp) image_ctrl->removeImage(tmp);
}

void FluxboxWindow::grabButtons() {
	Fluxbox *fluxbox = Fluxbox::instance();

	XGrabButton(display, Button1, AnyModifier, 
		frame.plate, True, ButtonPressMask,
		GrabModeSync, GrabModeSync, None, None);		
	XUngrabButton(display, Button1, Mod1Mask|Mod2Mask|Mod3Mask, frame.plate);


	XGrabButton(display, Button1, Mod1Mask, frame.window, True,
		ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
		GrabModeAsync, None, fluxbox->getMoveCursor());

	//----grab with "all" modifiers
	grabButton(display, Button1, frame.window, fluxbox->getMoveCursor());
	
	XGrabButton(display, Button2, Mod1Mask, frame.window, True,
		ButtonReleaseMask, GrabModeAsync, GrabModeAsync, None, None);
		
	XGrabButton(display, Button3, Mod1Mask, frame.window, True,
		ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
		GrabModeAsync, None, fluxbox->getLowerRightAngleCursor());
	
	//---grab with "all" modifiers
	grabButton(display, Button3, frame.window, fluxbox->getLowerRightAngleCursor());
}

void FluxboxWindow::createButton(int type, ButtonEventProc pressed, ButtonEventProc released, ButtonDrawProc draw) {
	Button b;
	b.win = createChildWindow(frame.title);
	Fluxbox::instance()->saveWindowSearch(b.win, this);
	b.type = type;
	b.used = true;
	b.draw = draw;
	b.pressed = pressed;
	b.released = released;
	buttonlist.push_back(b);
}


Window FluxboxWindow::findTitleButton(int type) {
	for (unsigned int i=0; i<buttonlist.size(); i++) {
		if (buttonlist[i].type == type)
			return buttonlist[i].win;
	}
	return 0;
}
void FluxboxWindow::stickyButton_cb(FluxboxWindow *t, XButtonEvent *be) {
	t->stick();
	FluxboxWindow::stickyDraw_cb(t, be->window, false);
}

void FluxboxWindow::stickyPressed_cb(FluxboxWindow *t, XButtonEvent *be) {
	FluxboxWindow::stickyDraw_cb(t, be->window, true);
}

void FluxboxWindow::iconifyButton_cb(FluxboxWindow *t, XButtonEvent *be) {
	t->iconify();
	FluxboxWindow::iconifyDraw_cb(t, be->window, false);
}

void FluxboxWindow::iconifyPressed_cb(FluxboxWindow *t, XButtonEvent *be) {
	t->screen->getWorkspace(t->workspace_number)->lowerWindow(t);
	FluxboxWindow::iconifyDraw_cb(t, be->window, true);
}

void FluxboxWindow::maximizeButton_cb(FluxboxWindow *t, XButtonEvent *be) {
 FluxboxWindow::maximizeDraw_cb(t, be->window, false);
 t->maximize(be->button); 
}

void FluxboxWindow::maximizePressed_cb(FluxboxWindow *t, XButtonEvent *be) {
 FluxboxWindow::maximizeDraw_cb(t, be->window, true); 
}

void FluxboxWindow::closeButton_cb(FluxboxWindow *t, XButtonEvent *be) {
	t->close();
	FluxboxWindow::closeDraw_cb(t, be->window, false);
}

void FluxboxWindow::closePressed_cb(FluxboxWindow *t, XButtonEvent *be) {
	FluxboxWindow::closeDraw_cb(t, be->window, true);
}

void FluxboxWindow::shadeButton_cb(FluxboxWindow *t, XButtonEvent *be) {
	FluxboxWindow::shadeDraw_cb(t, be->window, false);
	t->shade();
	if (t->tab)
		t->tab->shade();
}

void FluxboxWindow::stickyDraw_cb(FluxboxWindow *t, Window w, bool pressed) {
	t->drawButtonBase(w, pressed);
	if (t->stuck) {
		XFillRectangle(t->display, w, 
			((t->focused) ? t->screen->getWindowStyle()->b_pic_focus_gc :
			t->screen->getWindowStyle()->b_pic_unfocus_gc),
			t->frame.button_w/2-t->frame.button_w/4, t->frame.button_h/2-t->frame.button_h/4,
			t->frame.button_w/2, t->frame.button_h/2);
	} else {
		XFillRectangle(t->display, w, 
			((t->focused) ? t->screen->getWindowStyle()->b_pic_focus_gc :
			t->screen->getWindowStyle()->b_pic_unfocus_gc),
			t->frame.button_w/2, t->frame.button_h/2,
			t->frame.button_w/5, t->frame.button_h/5);
	}
}

void FluxboxWindow::iconifyDraw_cb(FluxboxWindow *t, Window w, bool pressed) {
	t->drawButtonBase(w, pressed);
	XDrawRectangle(t->display, w,
		((t->focused) ? t->screen->getWindowStyle()->b_pic_focus_gc :
		t->screen->getWindowStyle()->b_pic_unfocus_gc),
		2, t->frame.button_h - 5, t->frame.button_w - 5, 2);
}

void FluxboxWindow::maximizeDraw_cb(FluxboxWindow *t, Window w, bool pressed) {
	t->drawButtonBase(w, pressed);
	XDrawRectangle(t->display, w,
		((t->focused) ? t->screen->getWindowStyle()->b_pic_focus_gc :
			t->screen->getWindowStyle()->b_pic_unfocus_gc),
			2, 2, t->frame.button_w - 5, t->frame.button_h - 5);
	XDrawLine(t->display, w,
			((t->focused) ? t->screen->getWindowStyle()->b_pic_focus_gc :
				t->screen->getWindowStyle()->b_pic_unfocus_gc),
			2, 3, t->frame.button_w - 3, 3);
}

void FluxboxWindow::closeDraw_cb(FluxboxWindow *t, Window w, bool pressed) {
	t->drawButtonBase(w, pressed);
	XDrawLine(t->display, w,
			((t->focused) ? t->screen->getWindowStyle()->b_pic_focus_gc :
				t->screen->getWindowStyle()->b_pic_unfocus_gc), 2, 2,
				t->frame.button_w - 3, t->frame.button_h - 3);
	XDrawLine(t->display, w,
			((t->focused) ? t->screen->getWindowStyle()->b_pic_focus_gc :
				t->screen->getWindowStyle()->b_pic_unfocus_gc), 2,
				t->frame.button_h - 3,
				t->frame.button_w - 3, 2);
}

void FluxboxWindow::shadeDraw_cb(FluxboxWindow *t, Window w, bool pressed) {
	t->drawButtonBase(w, pressed);
}

void FluxboxWindow::grabButton(Display *display, unsigned int button, 
				Window window, Cursor cursor) {

	//numlock
	XGrabButton(display, button, Mod1Mask|Mod2Mask, window, True,
				ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
				GrabModeAsync, None, cursor);
	//scrolllock
	XGrabButton(display, button, Mod1Mask|Mod5Mask, window, True,
				ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
				GrabModeAsync, None, cursor);
	
	//capslock
	XGrabButton(display, button, Mod1Mask|LockMask, window, True,
				ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
				GrabModeAsync, None, cursor);

	//capslock+numlock
	XGrabButton(display, Button1, Mod1Mask|LockMask|Mod2Mask, window, True,
				ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
				GrabModeAsync, None, cursor);

	//capslock+scrolllock
	XGrabButton(display, button, Mod1Mask|LockMask|Mod5Mask, window, True,
				ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
				GrabModeAsync, None, cursor);
	
	//capslock+numlock+scrolllock
	XGrabButton(display, button, Mod1Mask|LockMask|Mod2Mask|Mod5Mask, window, True,
				ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
				GrabModeAsync, None, cursor);

	//numlock+scrollLock
	XGrabButton(display, button, Mod1Mask|Mod2Mask|Mod5Mask, window, True,
				ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
				GrabModeAsync, None, cursor);
	
}

void FluxboxWindow::drawButtonBase(Window w, bool pressed) {
	if (! pressed) {
		if (focused) {
			if (frame.fbutton)
				XSetWindowBackgroundPixmap(display, w, frame.fbutton);
			else
				XSetWindowBackground(display, w, frame.fbutton_pixel);
		} else {
			if (frame.ubutton)
				XSetWindowBackgroundPixmap(display, w, frame.ubutton);
			else
				XSetWindowBackground(display, w, frame.ubutton_pixel);
		}
	} else {
		if (frame.pbutton)
			XSetWindowBackgroundPixmap(display, w, frame.pbutton);
		else
			XSetWindowBackground(display, w, frame.pbutton_pixel);
	}
	XClearWindow(display, w);

}

void FluxboxWindow::positionButtons(bool redecorate_label) {
	unsigned int bw = frame.button_w + frame.bevel_w + 1,
		by = frame.bevel_w + 1, lx = by, lw = frame.width - by;	
	Fluxbox *fluxbox = Fluxbox::instance();
	vector<Fluxbox::Titlebar> left = fluxbox->getTitlebarLeft();
	vector<Fluxbox::Titlebar> right = fluxbox->getTitlebarRight();

	//left side buttons
	for (unsigned int i=0; i<left.size(); i++) {
		Window w = findTitleButton(left[i]);	//get Window of button
		if (w) {
			XMoveResizeWindow(display, w, lx, by,
					frame.button_w, frame.button_h);
			XMapWindow(display, w);
			XClearWindow(display, w);
			lx += bw;
			lw -= bw;
		}		
	}
	
	int bx = frame.width - bw;

	//right side buttons
	for (int i=right.size()-1; i>=0; i--) {
		Window w = findTitleButton(right[i]);	//get window of button
		if (w) {
			XMoveResizeWindow(display, w, bx, by,
					frame.button_w, frame.button_h);
			XMapWindow(display, w);
			XClearWindow(display, w);
			bx -= bw;
			lw -= bw;
		}
	}
	
	//Draw the label
	frame.label_w = lw - by;
	XMoveResizeWindow(display, frame.label, lx, frame.bevel_w,
		frame.label_w, frame.label_h);
	if (redecorate_label) 
		decorateLabel();
	if (tab) {
		tab->setPosition();
		tab->draw(false);
	}
	redrawLabel();
	redrawAllButtons();
}


void FluxboxWindow::reconfigure() {
	upsize();
	
	if (Fluxbox::instance()->useTabs()) {
		//no tab and we allowed to use tab? then create it 
		if (!tab && isGroupable()) {
			tab = new Tab(this, 0, 0);		
			if (current_state == IconicState)
				tab->iconify();
			else if (current_state == WithdrawnState)
				tab->withdraw();
		}
	} else {		
		if (tab != 0) { //got a tab? then destroy it
			delete tab;
			tab = 0;
		}
	}
	
	client.x = frame.x + frame.mwm_border_w + screen->getBorderWidth();
	client.y = frame.y + frame.y_border + frame.mwm_border_w +
			 screen->getBorderWidth();

	if (getTitle().size() > 0) {
		client.title_text_w = screen->getWindowStyle()->font.textWidth(getTitle().c_str(), getTitle().size());

		client.title_text_w += (frame.bevel_w * 4);
	}

		
	positionWindows();
	decorate();

				
	XClearWindow(display, frame.window);
	setFocusFlag(focused);

	configure(frame.x, frame.y, frame.width, frame.height);
	
	grabButtons();

	if (m_windowmenu.get()) {
		m_windowmenu->move(m_windowmenu->x(), frame.y + frame.title_h);
		m_windowmenu->reconfigure();
	}
		
	
}


void FluxboxWindow::positionWindows() {
	XResizeWindow(display, frame.window, frame.width,
							((shaded) ? frame.title_h : frame.height));
	XSetWindowBorderWidth(display, frame.window, screen->getBorderWidth());
	XSetWindowBorderWidth(display, frame.plate, screen->getFrameWidth() *
			decorations.border);
	XMoveResizeWindow(display, frame.plate, 0, frame.y_border,
								client.width, client.height);
	XMoveResizeWindow(display, client.window, 0, 0, client.width, client.height);

	if (decorations.titlebar) {
		XSetWindowBorderWidth(display, frame.title, screen->getBorderWidth());
		XMoveResizeWindow(display, frame.title, -screen->getBorderWidth(),
					-screen->getBorderWidth(), frame.width, frame.title_h);

		positionButtons();
	} else if (frame.title)
		XUnmapWindow(display, frame.title);

	if (decorations.handle) {
		XSetWindowBorderWidth(display, frame.handle, screen->getBorderWidth());
		XSetWindowBorderWidth(display, frame.left_grip, screen->getBorderWidth());
		XSetWindowBorderWidth(display, frame.right_grip, screen->getBorderWidth());

		XMoveResizeWindow(display, frame.handle, -screen->getBorderWidth(),
			frame.y_handle - screen->getBorderWidth(),
			frame.width, frame.handle_h);
		XMoveResizeWindow(display, frame.left_grip, -screen->getBorderWidth(),
			-screen->getBorderWidth(), frame.grip_w, frame.grip_h);
		XMoveResizeWindow(display, frame.right_grip,
			frame.width - frame.grip_w - screen->getBorderWidth(),
			-screen->getBorderWidth(), frame.grip_w, frame.grip_h);
		XMapSubwindows(display, frame.handle);
	} else if (frame.handle)
		XUnmapWindow(display, frame.handle);
	if (tab)
		tab->setPosition();
}


void FluxboxWindow::getWMName() {

	XTextProperty text_prop;
	char **list;
	int num;
	I18n *i18n = I18n::instance();

	if (XGetWMName(display, client.window, &text_prop)) {
		if (text_prop.value && text_prop.nitems > 0) {
			if (text_prop.encoding != XA_STRING) {
				
				text_prop.nitems = strlen((char *) text_prop.value);
				
				if ((XmbTextPropertyToTextList(display, &text_prop,
							&list, &num) == Success) &&
						(num > 0) && *list) {
					client.title = static_cast<char *>(*list);
					XFreeStringList(list);
				} else
					client.title = (char *)text_prop.value;
					
			} else
				client.title = (char *)text_prop.value;
			XFree((char *) text_prop.value);
		} else
			client.title = i18n->getMessage(
				FBNLS::WindowSet, FBNLS::WindowUnnamed,
				"Unnamed");
	} else {
		client.title = i18n->getMessage(
			FBNLS::WindowSet, FBNLS::WindowUnnamed,
			"Unnamed");
	}
	//Note: repeated?
	client.title_text_w = screen->getWindowStyle()->font.textWidth(getTitle().c_str(), getTitle().size());
	client.title_text_w += (frame.bevel_w * 4);
	
}


void FluxboxWindow::getWMIconName() {

	XTextProperty text_prop;
	char **list;
	int num;

	if (XGetWMIconName(display, client.window, &text_prop)) {
		if (text_prop.value && text_prop.nitems > 0) {
			if (text_prop.encoding != XA_STRING) {
				text_prop.nitems = strlen((char *) text_prop.value);

				if ((XmbTextPropertyToTextList(display, &text_prop,
						&list, &num) == Success) &&
						(num > 0) && *list) {
					client.icon_title = (char *)*list;
					XFreeStringList(list);
				} else
					client.icon_title = (char *)text_prop.value;
			} else
				client.icon_title = (char *)text_prop.value;

			XFree((char *) text_prop.value);
		} else
			client.icon_title = getTitle();
	} else
		client.icon_title = getTitle();
	
}


void FluxboxWindow::getWMProtocols() {
	Atom *proto;
	int num_return = 0;
	Fluxbox *fluxbox = Fluxbox::instance();

	if (XGetWMProtocols(display, client.window, &proto, &num_return)) {
		for (int i = 0; i < num_return; ++i) {
			if (proto[i] == fluxbox->getWMDeleteAtom())
				functions.close = decorations.close = true;
			else if (proto[i] == fluxbox->getWMTakeFocusAtom())
				send_focus_message = true;
			else if (proto[i] == fluxbox->getFluxboxStructureMessagesAtom())
				screen->addNetizen(new Netizen(screen, client.window));
		}

		XFree(proto);
	}
}


void FluxboxWindow::getWMHints() {
	XWMHints *wmhint = XGetWMHints(display, client.window);
	if (! wmhint) {
		visible = true;
		iconic = false;
		focus_mode = F_PASSIVE;
		client.window_group = None;
		client.initial_state = NormalState;
	} else {
		client.wm_hint_flags = wmhint->flags;
		if (wmhint->flags & InputHint) {
			if (wmhint->input) {
				if (send_focus_message)
					focus_mode = F_LOCALLYACTIVE;
				else
					focus_mode = F_PASSIVE;
			} else {
				if (send_focus_message)
					focus_mode = F_GLOBALLYACTIVE;
				else
					focus_mode = F_NOINPUT;
			}
		} else
			focus_mode = F_PASSIVE;

		if (wmhint->flags & StateHint)
			client.initial_state = wmhint->initial_state;
		else
			client.initial_state = NormalState;

		if (wmhint->flags & WindowGroupHint) {
			if (! client.window_group) {
				client.window_group = wmhint->window_group;
				Fluxbox::instance()->saveGroupSearch(client.window_group, this);
			}
		} else
			client.window_group = None;

		XFree(wmhint);
	}
}


void FluxboxWindow::getWMNormalHints() {
	long icccm_mask;
	XSizeHints sizehint;
	if (! XGetWMNormalHints(display, client.window, &sizehint, &icccm_mask)) {
		client.min_width = client.min_height =
			client.base_width = client.base_height =
			client.width_inc = client.height_inc = 1;
		client.max_width = 0; // unbounded
		client.max_height = 0;
		client.min_aspect_x = client.min_aspect_y =
			client.max_aspect_x = client.max_aspect_y = 1;
		client.win_gravity = NorthWestGravity;
	} else {
		client.normal_hint_flags = sizehint.flags;

		if (sizehint.flags & PMinSize) {
			client.min_width = sizehint.min_width;
			client.min_height = sizehint.min_height;
		} else
			client.min_width = client.min_height = 1;

		if (sizehint.flags & PMaxSize) {
			client.max_width = sizehint.max_width;
			client.max_height = sizehint.max_height;
		} else {
			client.max_width = 0; // unbounded
			client.max_height = 0;
		}

		if (sizehint.flags & PResizeInc) {
			client.width_inc = sizehint.width_inc;
			client.height_inc = sizehint.height_inc;
		} else
			client.width_inc = client.height_inc = 1;

		if (sizehint.flags & PAspect) {
			client.min_aspect_x = sizehint.min_aspect.x;
			client.min_aspect_y = sizehint.min_aspect.y;
			client.max_aspect_x = sizehint.max_aspect.x;
			client.max_aspect_y = sizehint.max_aspect.y;
		} else
			client.min_aspect_x = client.min_aspect_y =
				client.max_aspect_x = client.max_aspect_y = 1;

		if (sizehint.flags & PBaseSize) {
			client.base_width = sizehint.base_width;
			client.base_height = sizehint.base_height;
		} else
			client.base_width = client.base_height = 0;

		if (sizehint.flags & PWinGravity)
			client.win_gravity = sizehint.win_gravity;
		else
			client.win_gravity = NorthWestGravity;
	}
}


void FluxboxWindow::getMWMHints() {
	int format;
	Atom atom_return;
	unsigned long num, len;
	Fluxbox *fluxbox = Fluxbox::instance();
	if (!XGetWindowProperty(display, client.window,
			fluxbox->getMotifWMHintsAtom(), 0,
			PropMwmHintsElements, false,
			fluxbox->getMotifWMHintsAtom(), &atom_return,
			 &format, &num, &len,
			(unsigned char **) &client.mwm_hint) == Success &&
			client.mwm_hint) {
		return;
	}
	if (num != PropMwmHintsElements)
		return;
	
	if (client.mwm_hint->flags & MwmHintsDecorations) {
		if (client.mwm_hint->decorations & MwmDecorAll) {
			decorations.titlebar = decorations.handle = decorations.border =
				decorations.iconify = decorations.maximize =
				decorations.close = decorations.menu = true;
		} else {
			decorations.titlebar = decorations.handle = decorations.border =
				decorations.iconify = decorations.maximize =
				decorations.close = decorations.tab = false;
			decorations.menu = true;
			if (client.mwm_hint->decorations & MwmDecorBorder)
				decorations.border = true;
			if (client.mwm_hint->decorations & MwmDecorHandle)
				decorations.handle = true;
			if (client.mwm_hint->decorations & MwmDecorTitle)
				decorations.titlebar = decorations.tab = true; //only tab on windows with titlebar
			if (client.mwm_hint->decorations & MwmDecorMenu)
				decorations.menu = true;
			if (client.mwm_hint->decorations & MwmDecorIconify)
				decorations.iconify = true;
			if (client.mwm_hint->decorations & MwmDecorMaximize)
				decorations.maximize = true;
		}
	}
	
	if (client.mwm_hint->flags & MwmHintsFunctions) {
		if (client.mwm_hint->functions & MwmFuncAll) {
			functions.resize = functions.move = functions.iconify =
				functions.maximize = functions.close = true;
		} else {
			functions.resize = functions.move = functions.iconify =
				functions.maximize = functions.close = false;

			if (client.mwm_hint->functions & MwmFuncResize)
				functions.resize = true;
			if (client.mwm_hint->functions & MwmFuncMove)
				functions.move = true;
			if (client.mwm_hint->functions & MwmFuncIconify)
				functions.iconify = true;
			if (client.mwm_hint->functions & MwmFuncMaximize)
				functions.maximize = true;
			if (client.mwm_hint->functions & MwmFuncClose)
				functions.close = true;
		}
	}
	
	
}


void FluxboxWindow::getBlackboxHints() {
	int format;
	Atom atom_return;
	unsigned long num, len;
	Fluxbox *fluxbox = Fluxbox::instance();
	
	if (XGetWindowProperty(display, client.window,
			fluxbox->getFluxboxHintsAtom(), 0,
			PropBlackboxHintsElements, False,
			fluxbox->getFluxboxHintsAtom(), &atom_return,
			 &format, &num, &len,
			(unsigned char **) &client.blackbox_hint) == Success &&
			client.blackbox_hint) {

		if (num == PropBlackboxHintsElements) {
			if (client.blackbox_hint->flags & BaseDisplay::ATTRIB_SHADED)
				shaded = (client.blackbox_hint->attrib & BaseDisplay::ATTRIB_SHADED);

			if ((client.blackbox_hint->flags & BaseDisplay::ATTRIB_MAXHORIZ) &&
					(client.blackbox_hint->flags & BaseDisplay::ATTRIB_MAXVERT))
				maximized = ((client.blackbox_hint->attrib &
											(BaseDisplay::ATTRIB_MAXHORIZ | BaseDisplay::ATTRIB_MAXVERT)) ?	1 : 0);
			else if (client.blackbox_hint->flags & BaseDisplay::ATTRIB_MAXVERT)
				maximized = ((client.blackbox_hint->attrib & BaseDisplay::ATTRIB_MAXVERT) ? 2 : 0);
			else if (client.blackbox_hint->flags & BaseDisplay::ATTRIB_MAXHORIZ)
				maximized = ((client.blackbox_hint->attrib & BaseDisplay::ATTRIB_MAXHORIZ) ? 3 : 0);

			if (client.blackbox_hint->flags & BaseDisplay::ATTRIB_OMNIPRESENT)
				stuck = (client.blackbox_hint->attrib & BaseDisplay::ATTRIB_OMNIPRESENT);

			if (client.blackbox_hint->flags & BaseDisplay::ATTRIB_WORKSPACE)
				workspace_number = client.blackbox_hint->workspace;


			if (client.blackbox_hint->flags & BaseDisplay::ATTRIB_DECORATION) {
				old_decoration = static_cast<Decoration>(client.blackbox_hint->decoration);
				setDecoration(old_decoration);
			}
		}
	}
}


void FluxboxWindow::configure(int dx, int dy,
	unsigned int dw, unsigned int dh) {

	//we don't want negative size
	if (dw < 0 || dh < 0)
		return;

	bool send_event = (frame.x != dx || frame.y != dy);

	if ((dw != frame.width) || (dh != frame.height)) {
		if ((((signed) frame.width) + dx) < 0) 
			dx = 0;
		if ((((signed) frame.height) + dy) < 0) 
			dy = 0;

		frame.x = dx;
		frame.y = dy;
		frame.width = dw;
		frame.height = dh;
		
		downsize();

#ifdef	SHAPE
		if (Fluxbox::instance()->hasShapeExtensions() && frame.shaped) {
			XShapeCombineShape(display, frame.window, ShapeBounding,
 						 frame.mwm_border_w, frame.y_border +
			 frame.mwm_border_w, client.window,
			 ShapeBounding, ShapeSet);

			int num = 1;
			XRectangle xrect[2];
			xrect[0].x = xrect[0].y = 0;
			xrect[0].width = frame.width;
			xrect[0].height = frame.y_border;

			if (decorations.handle) {
				xrect[1].x = 0;
				xrect[1].y = frame.y_handle;
				xrect[1].width = frame.width;
				xrect[1].height = frame.handle_h + screen->getBorderWidth();
				num++;
			}

			XShapeCombineRectangles(display, frame.window, ShapeBounding, 0, 0,
						xrect, num, ShapeUnion, Unsorted);
		}
#endif // SHAPE
		XMoveResizeWindow(display, frame.window, frame.x, frame.y,
							frame.width, frame.height);
		positionWindows();
		decorate();
		setFocusFlag(focused);
		redrawAllButtons();
		shaded = false;
	} else {
		frame.x = dx;
		frame.y = dy;

		XMoveWindow(display, frame.window, frame.x, frame.y);
		//move the tab and the chain		
		if (tab)
			tab->setPosition();
		
		if (! moving) send_event = true;
	}

	if (send_event && ! moving) {
		client.x = dx + frame.mwm_border_w + screen->getBorderWidth();
		client.y = dy + frame.y_border + frame.mwm_border_w +
			screen->getBorderWidth();

		XEvent event;
		event.type = ConfigureNotify;

		event.xconfigure.display = display;
		event.xconfigure.event = client.window;
		event.xconfigure.window = client.window;
		event.xconfigure.x = client.x;
		event.xconfigure.y = client.y;
		event.xconfigure.width = client.width;
		event.xconfigure.height = client.height;
		event.xconfigure.border_width = client.old_bw;
		event.xconfigure.above = frame.window;
		event.xconfigure.override_redirect = false;

		XSendEvent(display, client.window, False, StructureNotifyMask, &event);

		screen->updateNetizenConfigNotify(&event);
	}
}


bool FluxboxWindow::setInputFocus() {
	//TODO hint skip focus

	if (((signed) (frame.x + frame.width)) < 0) {
		if (((signed) (frame.y + frame.y_border)) < 0)
			configure(screen->getBorderWidth(), screen->getBorderWidth(),
								frame.width, frame.height);
		else if (frame.y > (signed) screen->getHeight())
			configure(screen->getBorderWidth(), screen->getHeight() - frame.height,
								frame.width, frame.height);
		else
			configure(screen->getBorderWidth(), frame.y + screen->getBorderWidth(),
								frame.width, frame.height);
	} else if (frame.x > (signed) screen->getWidth()) {
		if (((signed) (frame.y + frame.y_border)) < 0)
			configure(screen->getWidth() - frame.width, screen->getBorderWidth(),
								frame.width, frame.height);
		else if (frame.y > (signed) screen->getHeight())
			configure(screen->getWidth() - frame.width,
					screen->getHeight() - frame.height, frame.width, frame.height);
		else
			configure(screen->getWidth() - frame.width,
								frame.y + screen->getBorderWidth(), frame.width, frame.height);
	}

	if (! validateClient())
		return false;

	bool ret = false;

	if (client.transients.size() && modal) {
		std::list<FluxboxWindow *>::iterator it = client.transients.begin();
		std::list<FluxboxWindow *>::iterator it_end = client.transients.end();
		for (; it != it_end; ++it) {
			if ((*it)->modal)
				return (*it)->setInputFocus();
		}
	} else {
		if (focus_mode == F_LOCALLYACTIVE || focus_mode == F_PASSIVE) {
			XSetInputFocus(display, client.window,
				RevertToPointerRoot, CurrentTime);
		} else {
			return false;
		}
		
		Fluxbox *fb = Fluxbox::instance();
		fb->setFocusedWindow(this);
			
		if (send_focus_message) {
			XEvent ce;
			ce.xclient.type = ClientMessage;
			ce.xclient.message_type = fb->getWMProtocolsAtom();
			ce.xclient.display = display;
			ce.xclient.window = client.window;
			ce.xclient.format = 32;
			ce.xclient.data.l[0] = fb->getWMTakeFocusAtom();
			ce.xclient.data.l[1] = fb->getLastTime();
			ce.xclient.data.l[2] = 0l;
			ce.xclient.data.l[3] = 0l;
			ce.xclient.data.l[4] = 0l;
			XSendEvent(display, client.window, false, NoEventMask, &ce);
		}

		if ((screen->isSloppyFocus() || screen->isSemiSloppyFocus())
				&& screen->doAutoRaise())
			timer.start();

		ret = true;
	}

	return ret;
}

//------------ setTab --------------
// Enables or disables the tab on the window
//----------------------------------
void FluxboxWindow::setTab(bool flag) {
	if (flag) {
		if (!tab && isGroupable())
			tab = new Tab(this, 0, 0);
		
		if (tab) {
			tab->focus(); // draws the tab with correct texture
			tab->setPosition(); // set tab windows position
		}

	} else if (tab) {
		delete tab;
		tab = 0;		
	}	
	decorations.tab = flag;
}

//------------- iconify ----------------
// Unmaps the window and removes it from workspace list
//--------------------------------------
void FluxboxWindow::iconify() {

	if (iconic)
		return;

	if (m_windowmenu.get())
		m_windowmenu->hide();

	setState(IconicState);

	XSelectInput(display, client.window, NoEventMask);
	XUnmapWindow(display, client.window);
	XSelectInput(display, client.window,
					PropertyChangeMask | StructureNotifyMask | FocusChangeMask);

	XUnmapWindow(display, frame.window);
	visible = false;
	iconic = true;
	
	screen->getWorkspace(workspace_number)->removeWindow(this);

	if (client.transient_for) {
		if (! client.transient_for->iconic)
			client.transient_for->iconify();
	}
	screen->addIcon(this);

	if (tab) //if this window got a tab then iconify it too
		tab->iconify();
		
	if (client.transients.size()) {
		std::list<FluxboxWindow *>::iterator it = client.transients.begin();
		std::list<FluxboxWindow *>::iterator it_end = client.transients.end();
		for (; it != it_end; ++it) {
			if (! (*it)->iconic)
				(*it)->iconify();
		}
	}

}


void FluxboxWindow::deiconify(bool reassoc, bool raise) {
	if (iconic || reassoc) {
		screen->reassociateWindow(this, screen->getCurrentWorkspace()->workspaceID(), false);
	} else if (workspace_number != screen->getCurrentWorkspace()->workspaceID())
		return;

	setState(NormalState);

	XSelectInput(display, client.window, NoEventMask);
	XMapWindow(display, client.window);
	XSelectInput(display, client.window,
		PropertyChangeMask | StructureNotifyMask | FocusChangeMask);

	XMapSubwindows(display, frame.window);
	XMapWindow(display, frame.window);

	if (iconic && screen->doFocusNew())
		setInputFocus();

	visible = true;
	iconic = false;

	if (reassoc && client.transients.size()) {
		// deiconify all transients
		std::list<FluxboxWindow *>::iterator it = client.transients.begin();
		std::list<FluxboxWindow *>::iterator it_end = client.transients.end();
		for (; it != it_end; ++it) {
			(*it)->deiconify(true, false);
		}
	}
	
	if (tab)
		tab->deiconify();
			
	if (raise) {		
		screen->getWorkspace(workspace_number)->raiseWindow(this);
		if (tab)
			tab->raise();
	}
}


void FluxboxWindow::close() {
	Fluxbox *fluxbox = Fluxbox::instance();
	XEvent ce;
	ce.xclient.type = ClientMessage;
	ce.xclient.message_type = fluxbox->getWMProtocolsAtom();
	ce.xclient.display = display;
	ce.xclient.window = client.window;
	ce.xclient.format = 32;
	ce.xclient.data.l[0] = fluxbox->getWMDeleteAtom();
	ce.xclient.data.l[1] = CurrentTime;
	ce.xclient.data.l[2] = 0l;
	ce.xclient.data.l[3] = 0l;
	ce.xclient.data.l[4] = 0l;
	XSendEvent(display, client.window, false, NoEventMask, &ce);
}


void FluxboxWindow::withdraw() {
	visible = false;
	iconic = false;
	if (isMoving())
		stopMoving();
	if (isResizing())
		stopResizing();

	XUnmapWindow(display, frame.window);

	XSelectInput(display, client.window, NoEventMask);
	XUnmapWindow(display, client.window);
	XSelectInput(display, client.window,
				PropertyChangeMask | StructureNotifyMask | FocusChangeMask);

	if (m_windowmenu.get())
		m_windowmenu->hide();
	
	if (tab)
		tab->withdraw();
}


void FluxboxWindow::maximize(unsigned int button) {
	// deiconify if we're iconic
	if (isIconic())
		deiconify();

	if (! maximized) {
		int dx, dy;
		unsigned int dw, dh, slitModL = 0, slitModR = 0, slitModT = 0, slitModB = 0;

#ifdef XINERAMA
		// get the head the window is on, taking the middle of the window to
		// make it feel right, maybe someone will like client.x, client.y better
		// tough?
		unsigned int head = (screen->hasXinerama()) ?
			screen->getHead(client.x + (client.width / 2), client.y + (client.height / 2))
			: 0;
#endif // XINERAMA


#ifdef	SLIT

#ifdef XINERAMA
	// take the slit in account if it's on the same head
	// (always true if we don't have xinerama
	if (!screen->hasXinerama() || screen->getSlitOnHead() == head) {
#endif // XINERAMA

		Slit* mSlt = screen->getSlit();

		if(!screen->doMaxOverSlit() && !screen->doFullMax() && (mSlt->width() > 1))
		{
			switch(screen->getSlitDirection())
			{
			case Slit::VERTICAL:
				switch(screen->getSlitPlacement())
				{
				case Slit::TOPRIGHT:
				case Slit::CENTERRIGHT:
				case Slit::BOTTOMRIGHT:
					slitModR = mSlt->width() + screen->getBevelWidth();
					break;
				default:
					slitModL = mSlt->width() + screen->getBevelWidth();
					break;
				}
			break;
			case Slit::HORIZONTAL:
				switch(screen->getSlitPlacement())
				{
				case Slit::TOPLEFT:
				case Slit::TOPCENTER:
				case Slit::TOPRIGHT:
					slitModT = mSlt->height() + screen->getBevelWidth();
					switch (screen->getToolbarPlacement()) {
						case Toolbar::TOPLEFT:
						case Toolbar::TOPCENTER:
						case Toolbar::TOPRIGHT:
							slitModT -= screen->getToolbar()->getExposedHeight() + 
								screen->getBorderWidth();
							break;
						default:
							break;
					}				
					break;
				default:
					slitModB = mSlt->height() + screen->getBevelWidth();
					switch (screen->getToolbarPlacement()) {
						case Toolbar::BOTTOMLEFT:
						case Toolbar::BOTTOMCENTER:
						case Toolbar::BOTTOMRIGHT:
							slitModB -= screen->getToolbar()->getExposedHeight() + 
								screen->getBorderWidth();
							break;
						default:
							break;
					}	
					break;
				}	
				break;
			}
		}		
#ifdef XINERAMA
	}
#endif // XINERAMA

#endif // SLIT
	
		blackbox_attrib.premax_x = frame.x;
		blackbox_attrib.premax_y = frame.y;
		blackbox_attrib.premax_w = frame.width;
		blackbox_attrib.premax_h = frame.height;

#ifdef XINERAMA
		dw = screen->getHeadWidth(head) - slitModL - slitModR;
#else // !XINERAMA
		dw = screen->getWidth() - slitModL - slitModR;
#endif // XINERAMA
		dw -= screen->getBorderWidth2x();
		dw -= frame.mwm_border_w * 2;
		dw -= client.base_width;

#ifdef XINERAMA
		dh = screen->getHeadHeight(head) - slitModT - slitModB;
#else // !XINERAMA
		dh = screen->getHeight() - slitModT - slitModB;
#endif // XINERAMA
		dh -= screen->getBorderWidth2x();
		dh -= frame.mwm_border_w * 2;
		dh -= ((frame.handle_h + screen->getBorderWidth()) * decorations.handle);
		dh -= client.base_height;
		dh -= frame.y_border;

		if (! screen->doFullMax()) {
#ifdef XINERAMA
			if (screen->hasXinerama()) {
				// is the toolbar on this head?
				if ((screen->getToolbarOnHead() == (signed) head) ||
						(screen->getToolbarOnHead() < 0)) {
					dh -= screen->getToolbar()->getExposedHeight() +
						screen->getBorderWidth2x();
				}
			} else {
				dh -= screen->getToolbar()->getExposedHeight() +
					screen->getBorderWidth2x();
			}
#else // !XINERAMA
			dh -= screen->getToolbar()->getExposedHeight() +
						screen->getBorderWidth2x();
#endif // XINERAMA
		}

		if (dw < client.min_width) dw = client.min_width;
		if (dh < client.min_height) dh = client.min_height;
		if (client.max_width != 0 && dw > client.max_width) dw = client.max_width;
		if (client.max_height != 0 && dh > client.max_height) dh = client.max_height;

		dw -= (dw % client.width_inc);
		dw += client.base_width;
		dh -= (dh % client.height_inc);
		dh += client.base_height;

		dw += frame.mwm_border_w * 2;

		dh += frame.y_border;
		dh += (frame.handle_h + screen->getBorderWidth());
		dh += frame.mwm_border_w * 2;

#ifdef XINERAMA
		dx = screen->getHeadX(head) +
			((screen->getHeadWidth(head) + slitModL - slitModR - dw) / 2)
			- screen->getBorderWidth();
#else // !XINERAMA
		dx = ((screen->getWidth()+ slitModL - slitModR - dw) / 2) - screen->getBorderWidth();
#endif // XINERAMA

		if (screen->doFullMax()) {
#ifdef XINERAMA
			dy = screen->getHeadY(head) +
				((screen->getHeadHeight(head) - dh) / 2) - screen->getBorderWidth();
#else // !XINERAMA
			dy = ((screen->getHeight() - dh) / 2) - screen->getBorderWidth();
#endif // XINERAMA
		} else {
#ifdef XINERAMA
			if (screen->hasXinerama()) { // xinerama
				dy = screen->getHeadY(head);
				// is the toolbar on this head?
				if ((screen->getToolbarOnHead() == (signed) head) ||
						(screen->getToolbarOnHead() < 0)) {
					dy += (((screen->getHeadHeight(head) + slitModT - slitModB
						- (screen->getToolbar()->getExposedHeight())) - dh) / 2)
						- screen->getBorderWidth2x();
			 	} else {
					dy += ((screen->getHeadHeight(head) + slitModT - slitModB - dh) / 2) -
						screen->getBorderWidth();
				}
			} else { // no xinerama
				dy = (((screen->getHeight() + slitModT - slitModB -
					(screen->getToolbar()->getExposedHeight())) - dh) / 2) -
					screen->getBorderWidth2x();
			}
#else // !XINERAMA
			dy = (((screen->getHeight() + slitModT - slitModB - (screen->getToolbar()->getExposedHeight()))
			 - dh) / 2) - screen->getBorderWidth2x();
#endif // XINERAMA

			switch (screen->getToolbarPlacement()) {
			case Toolbar::TOPLEFT:
			case Toolbar::TOPCENTER:
			case Toolbar::TOPRIGHT:
#ifdef XINERAMA
				// if < 0 than it's over ALL heads, with no xinerama it's there too
				if (!screen->hasXinerama() ||
						(screen->getToolbarOnHead() == (signed) head) ||
						(screen->getToolbarOnHead() < 0)) {
					dy += screen->getToolbar()->getExposedHeight() +
						screen->getBorderWidth2x();
				}
#else // !XINERAMA
				dy += screen->getToolbar()->getExposedHeight() +
						screen->getBorderWidth2x();
#endif // XINERAMA
				break;
			default:
				break;
			}
		}

		if (hasTab()) {
			switch(screen->getTabPlacement()) {			
			case Tab::PTOP:
				dy += screen->getTabHeight(); 
				dh -= screen->getTabHeight();
				break;
			case Tab::PLEFT:
				if (screen->isTabRotateVertical()) {
					dx += screen->getTabHeight();
					dw -= screen->getTabHeight();
				} else {
					dx += screen->getTabWidth();
					dw -= screen->getTabWidth();
				}	
				break;
			case Tab::PRIGHT:
				if (screen->isTabRotateVertical())
					dw -= screen->getTabHeight();
				else
					dw -= screen->getTabWidth();	
				break;
			case Tab::PBOTTOM:
				dh -= screen->getTabHeight();
				break;
			default:
				dy += screen->getTabHeight();
				dh -= screen->getTabHeight();
				break;
			}
		}

		if (button == 2) { //expand max width
			dw = frame.width;
			dx = frame.x;
		} else if (button == 3) { //expand max height
			dh = frame.height;
			dy = frame.y;
		}

		switch(button) {
		case 1:
			blackbox_attrib.flags |= BaseDisplay::ATTRIB_MAXHORIZ | BaseDisplay::ATTRIB_MAXVERT;
			blackbox_attrib.attrib |= BaseDisplay::ATTRIB_MAXHORIZ | BaseDisplay::ATTRIB_MAXVERT;

			break;

		case 2:
			blackbox_attrib.flags |= BaseDisplay::ATTRIB_MAXVERT;
			blackbox_attrib.attrib |= BaseDisplay::ATTRIB_MAXVERT;

			break;

		case 3:
			blackbox_attrib.flags |= BaseDisplay::ATTRIB_MAXHORIZ;
			blackbox_attrib.attrib |= BaseDisplay::ATTRIB_MAXHORIZ;

			break;
		}

		if (shaded) {
			blackbox_attrib.flags ^= BaseDisplay::ATTRIB_SHADED;
			blackbox_attrib.attrib ^= BaseDisplay::ATTRIB_SHADED;
			shaded = false;
			if (hasTab())
				getTab()->shade();
		}

		maximized = true;

		configure(dx, dy, dw, dh);
		if (tab)
			tab->raise();
		screen->getWorkspace(workspace_number)->raiseWindow(this);
		setState(current_state);

	} else {
		maximized = false;

		if (isShaded()) {
			shade();
			if (hasTab())
				getTab()->shade();
		}

		blackbox_attrib.flags &= ! (BaseDisplay::ATTRIB_MAXHORIZ | BaseDisplay::ATTRIB_MAXVERT);
		blackbox_attrib.attrib &= ! (BaseDisplay::ATTRIB_MAXHORIZ | BaseDisplay::ATTRIB_MAXVERT);

		configure(blackbox_attrib.premax_x, blackbox_attrib.premax_y,
				blackbox_attrib.premax_w, blackbox_attrib.premax_h);

		blackbox_attrib.premax_x = blackbox_attrib.premax_y = 0;
		blackbox_attrib.premax_w = blackbox_attrib.premax_h = 0;

		redrawAllButtons();
		setState(current_state);
	}

	if (tab) //resize all the windows in the tab group
		tab->resize();
}


void FluxboxWindow::setWorkspace(int n) {
	workspace_number = n;

	blackbox_attrib.flags |= BaseDisplay::ATTRIB_WORKSPACE;
	blackbox_attrib.workspace = workspace_number;

	// notify workspace change
#ifdef DEBUG
	cerr<<this<<" notify workspace signal"<<endl;
#endif // DEBUG
	m_workspacesig.notify();
}


void FluxboxWindow::shade() {
	if (decorations.titlebar) {
		if (shaded) {
			XResizeWindow(display, frame.window, frame.width, frame.height);
			shaded = false;
			blackbox_attrib.flags ^= BaseDisplay::ATTRIB_SHADED;
			blackbox_attrib.attrib ^= BaseDisplay::ATTRIB_SHADED;

			setState(NormalState);
		} else {
			XResizeWindow(display, frame.window, frame.width, frame.title_h);
			shaded = true;
			blackbox_attrib.flags |= BaseDisplay::ATTRIB_SHADED;
			blackbox_attrib.attrib |= BaseDisplay::ATTRIB_SHADED;

			setState(IconicState);
		}
	}
}


void FluxboxWindow::stick() {

	if (tab) //if it got a tab then do tab's stick on all of the objects in the list
		tab->stick(); //this window will stick too.
	else if (stuck) {
		blackbox_attrib.flags ^= BaseDisplay::ATTRIB_OMNIPRESENT;
		blackbox_attrib.attrib ^= BaseDisplay::ATTRIB_OMNIPRESENT;

		stuck = false;

	} else {
		stuck = true;
		if (screen->getCurrentWorkspaceID() != workspace_number) {
			screen->reassociateWindow(this,screen->getCurrentWorkspaceID(), true);
		}
		
		blackbox_attrib.flags |= BaseDisplay::ATTRIB_OMNIPRESENT;
		blackbox_attrib.attrib |= BaseDisplay::ATTRIB_OMNIPRESENT;

	}
	//find a STICK button in window
	redrawAllButtons();
	setState(current_state);
}


void FluxboxWindow::setFocusFlag(bool focus) {
	focused = focus;

	// Record focus timestamp for window cycling enhancements, such as skipping lower tabs
	if (focused)
		gettimeofday(&lastFocusTime, 0);

	if (decorations.titlebar) {
		if (focused) {
			if (frame.ftitle)
				XSetWindowBackgroundPixmap(display, frame.title, frame.ftitle);
			else
				XSetWindowBackground(display, frame.title, frame.ftitle_pixel);
		} else {
			if (frame.utitle)
				XSetWindowBackgroundPixmap(display, frame.title, frame.utitle);
			else
				XSetWindowBackground(display, frame.title, frame.utitle_pixel);
		}
		XClearWindow(display, frame.title);
		
		redrawLabel();
		redrawAllButtons();
	}

	if (decorations.handle) {
		if (focused) {
			if (frame.fhandle)
				XSetWindowBackgroundPixmap(display, frame.handle, frame.fhandle);
			else
				XSetWindowBackground(display, frame.handle, frame.fhandle_pixel);

			if (frame.fgrip) {
				XSetWindowBackgroundPixmap(display, frame.right_grip, frame.fgrip);
				XSetWindowBackgroundPixmap(display, frame.left_grip, frame.fgrip);
			} else {
				XSetWindowBackground(display, frame.right_grip, frame.fgrip_pixel);
				XSetWindowBackground(display, frame.left_grip, frame.fgrip_pixel);
			}
		} else {
			if (frame.uhandle)
				XSetWindowBackgroundPixmap(display, frame.handle, frame.uhandle);
			else
				XSetWindowBackground(display, frame.handle, frame.uhandle_pixel);

			if (frame.ugrip) {
				XSetWindowBackgroundPixmap(display, frame.right_grip, frame.ugrip);
				XSetWindowBackgroundPixmap(display, frame.left_grip, frame.ugrip);
			} else {
				XSetWindowBackground(display, frame.right_grip, frame.ugrip_pixel);
				XSetWindowBackground(display, frame.left_grip, frame.ugrip_pixel);
			}
		}
		XClearWindow(display, frame.handle);
		XClearWindow(display, frame.right_grip);
		XClearWindow(display, frame.left_grip);
	}
	
	if (tab)
		tab->focus();
	
	if (decorations.border) {
		if (focused)
			XSetWindowBorder(display, frame.plate, frame.fborder_pixel);
		else
			XSetWindowBorder(display, frame.plate, frame.uborder_pixel);
	}

	if ((screen->isSloppyFocus() || screen->isSemiSloppyFocus()) &&
			screen->doAutoRaise())
		timer.stop();
}


void FluxboxWindow::installColormap(bool install) {
	Fluxbox *fluxbox = Fluxbox::instance();
	fluxbox->grab();
	if (! validateClient()) return;

	int i = 0, ncmap = 0;
	Colormap *cmaps = XListInstalledColormaps(display, client.window, &ncmap);
	XWindowAttributes wattrib;
	if (cmaps) {
		if (XGetWindowAttributes(display, client.window, &wattrib)) {
			if (install) {
				// install the window's colormap
				for (i = 0; i < ncmap; i++) {
					if (*(cmaps + i) == wattrib.colormap) {
						// this window is using an installed color map... do not install
						install = false;
						break; //end for-loop (we dont need to check more)
					}
				}
				// otherwise, install the window's colormap
				if (install)
					XInstallColormap(display, wattrib.colormap);
			} else {				
				for (i = 0; i < ncmap; i++) // uninstall the window's colormap
					if (*(cmaps + i) == wattrib.colormap)						
						XUninstallColormap(display, wattrib.colormap); // we found the colormap to uninstall
			}
		}

		XFree(cmaps);
	}

	fluxbox->ungrab();
}


void FluxboxWindow::setState(unsigned long new_state) {
	current_state = new_state;
	Fluxbox *fluxbox = Fluxbox::instance();
	unsigned long state[2];
	state[0] = (unsigned long) current_state;
	state[1] = (unsigned long) None;
	XChangeProperty(display, client.window, fluxbox->getWMStateAtom(),
			fluxbox->getWMStateAtom(), 32, PropModeReplace,
			(unsigned char *) state, 2);

	XChangeProperty(display, client.window, fluxbox->getFluxboxAttributesAtom(),
					fluxbox->getFluxboxAttributesAtom(), 32, PropModeReplace,
					(unsigned char *) &blackbox_attrib, PropBlackboxAttributesElements);

	//notify state changed
	m_statesig.notify();
}

//TODO: why ungrab in if-statement?
bool FluxboxWindow::getState() {
	current_state = 0;

	Atom atom_return;
	bool ret = false;
	int foo;
	unsigned long *state, ulfoo, nitems;
	Fluxbox *fluxbox = Fluxbox::instance();
	if ((XGetWindowProperty(display, client.window, fluxbox->getWMStateAtom(),
				0l, 2l, false, fluxbox->getWMStateAtom(),
				&atom_return, &foo, &nitems, &ulfoo,
				(unsigned char **) &state) != Success) ||
			(! state)) {
		fluxbox->ungrab();
		return false;
	}

	if (nitems >= 1) {
		current_state = static_cast<unsigned long>(state[0]);
		ret = true;
	}

	XFree(static_cast<void *>(state));

	return ret;
}


void FluxboxWindow::setGravityOffsets() {
	// translate x coordinate
	switch (client.win_gravity) {
		// handle Westward gravity
	case NorthWestGravity:
	case WestGravity:
	case SouthWestGravity:
	default:
		frame.x = client.x;
			break;

		// handle Eastward gravity
	case NorthEastGravity:
	case EastGravity:
	case SouthEastGravity:
		frame.x = (client.x + client.width) - frame.width;
		break;

		// no x translation desired - default
	case StaticGravity:
	case ForgetGravity:
	case CenterGravity:
		frame.x = client.x - frame.mwm_border_w + screen->getBorderWidth();
	}

	// translate y coordinate
	switch (client.win_gravity) {
		// handle Northbound gravity
	case NorthWestGravity:
	case NorthGravity:
	case NorthEastGravity:
	default:
		frame.y = client.y;
		break;

		// handle Southbound gravity
	case SouthWestGravity:
	case SouthGravity:
	case SouthEastGravity:
		frame.y = (client.y + client.height) - frame.height;
		break;

		// no y translation desired - default
	case StaticGravity:
	case ForgetGravity:
	case CenterGravity:
		frame.y = client.y - frame.y_border - frame.mwm_border_w -
			screen->getBorderWidth();
		break;
	}
}


void FluxboxWindow::restoreAttributes() {
	if (!getState())
		current_state = NormalState;

	Atom atom_return;
	int foo;
	unsigned long ulfoo, nitems;
	Fluxbox *fluxbox = Fluxbox::instance();
	
	BaseDisplay::BlackboxAttributes *net;
	if (XGetWindowProperty(display, client.window,
			 fluxbox->getFluxboxAttributesAtom(), 0l,
			 PropBlackboxAttributesElements, false,
			 fluxbox->getFluxboxAttributesAtom(), &atom_return, &foo,
			 &nitems, &ulfoo, (unsigned char **) &net) ==
			Success && net && nitems == PropBlackboxAttributesElements) {
		blackbox_attrib.flags = net->flags;
		blackbox_attrib.attrib = net->attrib;
		blackbox_attrib.workspace = net->workspace;
		blackbox_attrib.stack = net->stack;
		blackbox_attrib.premax_x = net->premax_x;
		blackbox_attrib.premax_y = net->premax_y;
		blackbox_attrib.premax_w = net->premax_w;
		blackbox_attrib.premax_h = net->premax_h;

		XFree(static_cast<void *>(net));
	} else
		return;

	if (blackbox_attrib.flags & BaseDisplay::ATTRIB_SHADED &&
			blackbox_attrib.attrib & BaseDisplay::ATTRIB_SHADED) {
		int save_state =
			((current_state == IconicState) ? NormalState : current_state);

		shaded = false;
		shade();
		if (tab)
			tab->shade();
			
		current_state = save_state;
	}

	if (( blackbox_attrib.workspace != screen->getCurrentWorkspaceID()) &&
			( blackbox_attrib.workspace < screen->getCount())) {
		screen->reassociateWindow(this, blackbox_attrib.workspace, true);

		if (current_state == NormalState) current_state = WithdrawnState;
	} else if (current_state == WithdrawnState)
		current_state = NormalState;

	if (blackbox_attrib.flags & BaseDisplay::ATTRIB_OMNIPRESENT &&
			blackbox_attrib.attrib & BaseDisplay::ATTRIB_OMNIPRESENT) {
		stuck = false;
		stick();

		current_state = NormalState;
	}

	if ((blackbox_attrib.flags & BaseDisplay::ATTRIB_MAXHORIZ) ||
			(blackbox_attrib.flags & BaseDisplay::ATTRIB_MAXVERT)) {
		int x = blackbox_attrib.premax_x, y = blackbox_attrib.premax_y;
		unsigned int w = blackbox_attrib.premax_w, h = blackbox_attrib.premax_h;
		maximized = false;

		int m;
		if ((blackbox_attrib.flags & BaseDisplay::ATTRIB_MAXHORIZ) &&
				(blackbox_attrib.flags & BaseDisplay::ATTRIB_MAXVERT))
			m = ((blackbox_attrib.attrib & (BaseDisplay::ATTRIB_MAXHORIZ | BaseDisplay::ATTRIB_MAXVERT)) ?
					 1 : 0);
		else if (blackbox_attrib.flags & BaseDisplay::ATTRIB_MAXVERT)
			m = ((blackbox_attrib.attrib & BaseDisplay::ATTRIB_MAXVERT) ? 2 : 0);
		else if (blackbox_attrib.flags & BaseDisplay::ATTRIB_MAXHORIZ)
			m = ((blackbox_attrib.attrib & BaseDisplay::ATTRIB_MAXHORIZ) ? 3 : 0);
		else
			m = 0;

		if (m) maximize(m);

		blackbox_attrib.premax_x = x;
		blackbox_attrib.premax_y = y;
		blackbox_attrib.premax_w = w;
		blackbox_attrib.premax_h = h;
	}

	setState(current_state);
}

void FluxboxWindow::showMenu(int mx, int my) {
	if (m_windowmenu.get() == 0)
		return;
	m_windowmenu->move(mx, my);
	m_windowmenu->show();		
	m_windowmenu->raise();
	m_windowmenu->getSendToMenu().raise();
	m_windowmenu->getSendGroupToMenu().raise();
}
				
void FluxboxWindow::restoreGravity() {
	// restore x coordinate
	switch (client.win_gravity) {
		// handle Westward gravity
	case NorthWestGravity:
	case WestGravity:
	case SouthWestGravity:
	default:
		client.x = frame.x;
		break;

		// handle Eastward gravity
	case NorthEastGravity:
	case EastGravity:
	case SouthEastGravity:
		client.x = (frame.x + frame.width) - client.width;
		break;
	}

	// restore y coordinate
	switch (client.win_gravity) {
		// handle Northbound gravity
		case NorthWestGravity:
		case NorthGravity:
		case NorthEastGravity:
		default:
			client.y = frame.y;
		break;

		// handle Southbound gravity
		case SouthWestGravity:
		case SouthGravity:
		case SouthEastGravity:
			client.y = (frame.y + frame.height) - client.height;
		break;
	}
}

bool FluxboxWindow::isLowerTab() const {
	Tab* chkTab = (tab ? tab->first() : 0);
	while (chkTab) {
		const FluxboxWindow* chkWin = chkTab->getWindow();
		if (chkWin && chkWin != this &&
				timercmp(&chkWin->lastFocusTime, &lastFocusTime, >))
			return true;
		chkTab = chkTab->next();
	}
	return false;
}

void FluxboxWindow::redrawLabel() {
	if (focused) {
		if (frame.flabel)
			XSetWindowBackgroundPixmap(display, frame.label, frame.flabel);
		else
			XSetWindowBackground(display, frame.label, frame.flabel_pixel);
	} else {
		if (frame.ulabel)
			XSetWindowBackgroundPixmap(display, frame.label, frame.ulabel);
		else
			XSetWindowBackground(display, frame.label, frame.ulabel_pixel);
	}

	XClearWindow(display, frame.label);

	//no need to draw the title if we don't have any
	if (getTitle().size() != 0) {
		GC gc = ((focused) ? screen->getWindowStyle()->l_text_focus_gc :
				 screen->getWindowStyle()->l_text_unfocus_gc);
		unsigned int l = client.title_text_w;
		int dlen = getTitle().size();
		int dx = frame.bevel_w;
		FbTk::Font &font = screen->getWindowStyle()->font;
		if (l > frame.label_w) {
			for (; dlen >= 0; dlen--) {
				l = font.textWidth(getTitle().c_str(), dlen) + frame.bevel_w*4; 
				if (l < frame.label_w)
					break;
			}
		}
		switch (screen->getWindowStyle()->justify) {
			case DrawUtil::Font::RIGHT:
				dx += frame.label_w - l;
			break;
			case DrawUtil::Font::CENTER:
				dx += (frame.label_w - l)/2;
			break;
		}

		font.drawText(
			frame.label,
			screen->getScreenNumber(),
			gc,
			getTitle().c_str(), getTitle().size(),
			dx, screen->getWindowStyle()->font.ascent() + 1);
	}
}


void FluxboxWindow::redrawAllButtons() {
	for (unsigned int i=0; i<buttonlist.size(); i++)
		if (buttonlist[i].draw)
			buttonlist[i].draw(this, buttonlist[i].win, false);

}

void FluxboxWindow::mapRequestEvent(XMapRequestEvent *re) {
#ifdef DEBUG
	fprintf(stderr,
		I18n::instance()->getMessage(
			 FBNLS::WindowSet, FBNLS::WindowMapRequest,
			 "FluxboxWindow::mapRequestEvent() for 0x%lx\n"),
			client.window);
#endif // DEBUG
	if (re->window != client.window)
		return;

	Fluxbox *fluxbox = Fluxbox::instance();
	fluxbox->grab();
	if (! validateClient())
		return;
	
	bool get_state_ret = getState();
	if (! (get_state_ret && fluxbox->isStartup())) {
		if ((client.wm_hint_flags & StateHint) &&
				(! (current_state == NormalState || current_state == IconicState))) {
			current_state = client.initial_state;
		} else
			current_state = NormalState;
	} else if (iconic)
		current_state = NormalState;
		
	switch (current_state) {
	case IconicState:
		iconify();
	break;

	case WithdrawnState:
		withdraw();
	break;

	case NormalState:
		//check WM_CLASS only when we changed state to NormalState from 
		// WithdrawnState (ICCC 4.1.2.5)
				
		XClassHint ch;

		if (XGetClassHint(display, getClientWindow(), &ch) == 0) {
			cerr<<"Failed to read class hint!"<<endl;
		} else {
			if (ch.res_name != 0) {
				m_instance_name = const_cast<char *>(ch.res_name);
				XFree(ch.res_name);
			} else
				m_instance_name = "";

			if (ch.res_class != 0) {
				m_class_name = const_cast<char *>(ch.res_class);
				XFree(ch.res_class);
			} else 
				m_class_name = "";

			Workspace *wsp = screen->getWorkspace(workspace_number);
			// we must be resizable AND maximizable to be autogrouped
			// TODO: there should be an isGroupable() function
			if (wsp != 0 && isResizable() && isMaximizable()) {
				wsp->checkGrouping(*this);
			}
		}		
		
		deiconify(false);
			
	break;
	case InactiveState:
	case ZoomState:
	default:
		deiconify(false);
		break;
	}

	fluxbox->ungrab();

}


void FluxboxWindow::mapNotifyEvent(XMapEvent *ne) {
	if ((ne->window == client.window) && (! ne->override_redirect) && (visible)) {
		Fluxbox *fluxbox = Fluxbox::instance();
		fluxbox->grab();
		if (! validateClient())
			return;

		if (decorations.titlebar)
			positionButtons();

		setState(NormalState);		
			
		redrawAllButtons();

		if (transient || screen->doFocusNew())
			setInputFocus();
		else
			setFocusFlag(false);			
		

		visible = true;
		iconic = false;

		// Auto-group from tab?
		if (!transient) {
			// Grab and clear the auto-group window
			FluxboxWindow* autoGroupWindow = screen->useAutoGroupWindow();
			if (autoGroupWindow) {
				Tab *groupTab = autoGroupWindow->getTab();
				if (groupTab)
					groupTab->addWindowToGroup(this);
			}
		}

		fluxbox->ungrab();
	}
}

//------------------- unmapNotify ------------------
// Unmaps frame window and client window if 
// event.window == client.window
// Returns true if *this should die
// else false
//-------------------------------------------------
bool FluxboxWindow::unmapNotifyEvent(XUnmapEvent *ue) {
	if (ue->window != client.window)
		return false;	
	
#ifdef DEBUG
	fprintf(stderr,
		I18n::instance()->getMessage(
		 FBNLS::WindowSet, FBNLS::WindowUnmapNotify,
		 "FluxboxWindow::unmapNotifyEvent() for 0x%lx\n"),
		client.window);
#endif // DEBUG

	restore(false);

	return true; // make sure this one deletes

}

//----------- destroyNotifyEvent -------------
// Checks if event is for client.window.
// Unmaps frame.window and returns true for destroy
// of this FluxboxWindow else returns false.
//--------------------------------------------
bool FluxboxWindow::destroyNotifyEvent(XDestroyWindowEvent *de) {
	if (de->window == client.window) {
#ifdef DEBUG
		cerr<<__FILE__<<"("<<__LINE__<<"): DestroyNotifyEvent this="<<this<<endl;
#endif // DEBUG
		XUnmapWindow(display, frame.window);		
		return true;
	}
	return false;
}


void FluxboxWindow::propertyNotifyEvent(Atom atom) {
 	Fluxbox *fluxbox = Fluxbox::instance();

	if (! validateClient()) 
		return;

	switch(atom) {
	case XA_WM_CLASS:
	case XA_WM_CLIENT_MACHINE:
	case XA_WM_COMMAND:
		break;

	case XA_WM_TRANSIENT_FOR:
		checkTransient();
		reconfigure();

		break;

	case XA_WM_HINTS:
		getWMHints();
		break;

	case XA_WM_ICON_NAME:
		getWMIconName();
		if (iconic)
			screen->iconUpdate();
		updateIcon();
		break;

	case XA_WM_NAME:
		getWMName();

		if (decorations.titlebar)
			redrawLabel();

		if (hasTab()) // update tab
			getTab()->draw(false);

		if (! iconic)
			screen->getWorkspace(workspace_number)->update();
		else
			updateIcon();
		 

		break;

	case XA_WM_NORMAL_HINTS: {
		getWMNormalHints();

		if ((client.normal_hint_flags & PMinSize) &&
			(client.normal_hint_flags & PMaxSize)) {
 
			if (client.max_width != 0 && client.max_width <= client.min_width &&
				client.max_height != 0 && client.max_height <= client.min_height) {
				decorations.maximize = false;
				decorations.handle = false;
				functions.resize=false;
				functions.maximize=false;
			} else {
				if (! isTransient()) {
					decorations.maximize = true;
					decorations.handle = true;
					functions.maximize = true;	        
				}
				functions.resize = true;
			}
 
    	}

		int x = frame.x, y = frame.y;
		unsigned int w = frame.width, h = frame.height;

		upsize();

		if ((x != frame.x) || (y != frame.y) ||
				(w != frame.width) || (h != frame.height))
			reconfigure();

		break; 
	}

	default:
		if (atom == fluxbox->getWMProtocolsAtom()) {
			getWMProtocols();

			if (decorations.close && !findTitleButton(Fluxbox::CLOSE)) {	
				createButton(Fluxbox::CLOSE, FluxboxWindow::closePressed_cb, 
					FluxboxWindow::closeButton_cb, FluxboxWindow::closeDraw_cb);

				if (decorations.titlebar)
					positionButtons(true);
				if (m_windowmenu.get())
					m_windowmenu->reconfigure();
			}
		} 
		break;
	}

}


void FluxboxWindow::exposeEvent(XExposeEvent *ee) {
	if (frame.label == ee->window && decorations.titlebar)
		redrawLabel();
	else 
		redrawAllButtons();

}


void FluxboxWindow::configureRequestEvent(XConfigureRequestEvent *cr) {
	if (cr->window == client.window) {
		Fluxbox *fluxbox = Fluxbox::instance();
		fluxbox->grab();
		if (! validateClient())
			return;

		int cx = frame.x, cy = frame.y;
		unsigned int cw = frame.width, ch = frame.height;

		if (cr->value_mask & CWBorderWidth)
			client.old_bw = cr->border_width;

		if (cr->value_mask & CWX)
			cx = cr->x - frame.mwm_border_w - screen->getBorderWidth();

		if (cr->value_mask & CWY)
			cy = cr->y - frame.y_border - frame.mwm_border_w -
				screen->getBorderWidth();

		if (cr->value_mask & CWWidth)
			cw = cr->width + (frame.mwm_border_w * 2);

		if (cr->value_mask & CWHeight) {
			ch = cr->height + (frame.y_border + (frame.mwm_border_w * 2) +
				screen->getBorderWidth()) * decorations.border + 
				frame.handle_h*decorations.handle;			
		}
		if (frame.x != cx || frame.y != cy ||
			frame.width != cw || frame.height != ch) {
			configure(cx, cy, cw, ch);
			if (tab)
				tab->resize();
		}
		if (cr->value_mask & CWStackMode) {
			switch (cr->detail) {
			case Above:
			case TopIf:
			default:
				if (iconic)
					deiconify();
//!!TODO check this and the line below..
//				if (tab)
//					tab->raise();
				screen->getWorkspace(workspace_number)->raiseWindow(this);
				break;

			case Below:
			case BottomIf:
				if (iconic)
					deiconify();

//				if (tab)
//					tab->raise();
				screen->getWorkspace(workspace_number)->lowerWindow(this);
				break;
			}
		}

		fluxbox->ungrab();
	}
}


void FluxboxWindow::buttonPressEvent(XButtonEvent *be) {
	Fluxbox *fluxbox = Fluxbox::instance();
	fluxbox->grab();
	
	if (! validateClient())	
		return;
	
	if (be->button == 1 || (be->button == 3 && be->state == Mod1Mask)) {
		if ((! focused) && (! screen->isSloppyFocus()))	 //check focus
			setInputFocus(); 
		
		//Redraw buttons
		for (unsigned int i=0; i<buttonlist.size(); i++) {
			if (be->window == buttonlist[i].win && buttonlist[i].draw)
					buttonlist[i].draw(this, be->window, true);
		}

		if (frame.plate == be->window) {
			
			if (m_windowmenu.get() && m_windowmenu->isVisible()) //hide menu if its visible
				m_windowmenu->hide();
			//raise tab first, if there is any, so the focus on windows get 
			//right and dont "hide" the tab behind other windows
			if (tab)
				tab->raise();
			
			screen->getWorkspace(workspace_number)->raiseWindow(this);

			XAllowEvents(display, ReplayPointer, be->time);
			
		} else {
		
			if (frame.title == be->window || frame.label == be->window) {
				if (((be->time - lastButtonPressTime) <=
			 		fluxbox->getDoubleClickInterval()) ||
			 	 			(be->state & ControlMask)) {
					lastButtonPressTime = 0;
					shade();
					if (tab) //shade windows in the tablist too
						tab->shade();
				} else
					lastButtonPressTime = be->time;			
			}
			
			
			frame.grab_x = be->x_root - frame.x - screen->getBorderWidth();
			frame.grab_y = be->y_root - frame.y - screen->getBorderWidth();

			if (m_windowmenu.get() && m_windowmenu->isVisible())
				m_windowmenu->hide();
			//raise tab first if there is any
			if (hasTab())
				tab->raise();
		
			screen->getWorkspace(workspace_number)->raiseWindow(this);
		}
	} else if (be->button == 2 && be->window == frame.label) {
		screen->getWorkspace(workspace_number)->lowerWindow(this);
		if (hasTab())
			getTab()->lower(); //lower the tab AND it's windows

	} else if (m_windowmenu.get() && be->button == 3 &&
			(frame.title == be->window || frame.label == be->window ||
			frame.handle == be->window)) {

		int mx = 0, my = 0;

		if (frame.title == be->window || frame.label == be->window) {
			mx = be->x_root - (m_windowmenu->width() / 2);
			my = frame.y + frame.title_h;
		} else if (frame.handle == be->window) {
			mx = be->x_root - (m_windowmenu->width() / 2);
			my = frame.y + frame.y_handle - m_windowmenu->height();
		} else {
			bool buttonproc=false;
			
			if (buttonproc)
				mx = be->x_root - (m_windowmenu->width() / 2);

			if (be->y <= (signed) frame.bevel_w)
				my = frame.y + frame.y_border;
			else
				my = be->y_root - (m_windowmenu->height() / 2);
		}

		if (mx > (signed) (frame.x + frame.width - m_windowmenu->width()))
			mx = frame.x + frame.width - m_windowmenu->width();
		if (mx < frame.x)
			mx = frame.x;

		if (my > (signed) (frame.y + frame.y_handle - m_windowmenu->height()))
			my = frame.y + frame.y_handle - m_windowmenu->height();
		if (my < (signed) (frame.y + ((decorations.titlebar) ? frame.title_h :
							frame.y_border)))
			my = frame.y +
			((decorations.titlebar) ? frame.title_h : frame.y_border);

		if (m_windowmenu.get()) {
			if (! m_windowmenu->isVisible()) { // if not window menu is visible then show it
				showMenu(mx, my);
			} else //else hide menu
				m_windowmenu->hide(); 
		}
		
	} else if (be->button == 4) { //scroll to tab right
		if (tab && tab->next()) {
			tab->next()->getWindow()->setInputFocus();
			screen->getWorkspace(workspace_number)->raiseWindow(tab->next()->getWindow());
		}
	} else if (be->button == 5) { //scroll to tab left
		if (tab && tab->prev()) {
			tab->prev()->getWindow()->setInputFocus();
			screen->getWorkspace(workspace_number)->raiseWindow(tab->prev()->getWindow());
		}
	}
	
	fluxbox->ungrab();
}


void FluxboxWindow::buttonReleaseEvent(XButtonEvent *re) {
	Fluxbox *fluxbox = Fluxbox::instance();
	fluxbox->grab();

	if (! validateClient())
		return;

	if (isMoving())
		stopMoving();		
	else if (isResizing())
		stopResizing();
	else if (re->window == frame.window) {
		if (re->button == 2 && re->state == Mod1Mask)
			XUngrabPointer(display, CurrentTime);
	} else {
		if ((re->x >= 0) && ((unsigned) re->x <= frame.button_w) &&
			(re->y >= 0) && ((unsigned) re->y <= frame.button_h)) {
			for (unsigned int i=0; i<buttonlist.size(); i++) {
				if (re->window == buttonlist[i].win &&
						buttonlist[i].released) {
					buttonlist[i].released(this, re);	
					break;					
				}
			}
		} 
		redrawAllButtons();
	}

	fluxbox->ungrab();
}


void FluxboxWindow::motionNotifyEvent(XMotionEvent *me) {
	if ((me->state & Button1Mask) && functions.move &&
			(frame.title == me->window || frame.label == me->window ||
			 frame.handle == me->window || frame.window == me->window) && !isResizing()) {
			 
		if (! isMoving()) {
			startMoving(me->window);			
		} else {			
			int dx = me->x_root - frame.grab_x, dy = me->y_root - frame.grab_y;

			dx -= screen->getBorderWidth();
			dy -= screen->getBorderWidth();

			if (screen->getEdgeSnapThreshold()) {
				int drx = screen->getWidth() - (dx + frame.snap_w);

				if (dx > 0 && dx < drx && dx < screen->getEdgeSnapThreshold()) 
					dx = 0;
				else if (drx > 0 && drx < screen->getEdgeSnapThreshold())
					dx = screen->getWidth() - frame.snap_w;

				int dtty, dbby, dty, dby;
				switch (screen->getToolbarPlacement()) {
				case Toolbar::TOPLEFT:
				case Toolbar::TOPCENTER:
				case Toolbar::TOPRIGHT:
					dtty = screen->getToolbar()->getExposedHeight() +
							screen->getBorderWidth();
					dbby = screen->getHeight();
					break;

				default:
					dtty = 0;
					dbby = screen->getToolbar()->y();
					break;
				}

				dty = dy - dtty;
				dby = dbby - (dy + frame.snap_h);

				if (dy > 0 && dty < screen->getEdgeSnapThreshold())
					dy = dtty;
				else if (dby > 0 && dby < screen->getEdgeSnapThreshold())
					dy = dbby - frame.snap_h;
			}
			// Warp to next or previous workspace?, must have moved sideways some
			int moved_x=me->x_root - frame.resize_x;
			// save last event point
			frame.resize_x = me->x_root;
			frame.resize_y = me->y_root;
			if (moved_x && screen->isWorkspaceWarping()) {
				int cur_id = screen->getCurrentWorkspaceID();
				int new_id = cur_id;
				const int warpPad = screen->getEdgeSnapThreshold();
				if (me->x_root >= int(screen->getWidth()) - warpPad - 1 &&
						frame.x < int(me->x_root - frame.grab_x - screen->getBorderWidth())) {
					//warp right
					new_id = (cur_id + 1) % screen->getCount();
					dx = -me->x_root;
				} else if (me->x_root <= warpPad &&
						frame.x > int(me->x_root - frame.grab_x - screen->getBorderWidth())) {
					//warp left
					new_id = (cur_id - 1 + screen->getCount()) % screen->getCount();
					dx = screen->getWidth() - me->x_root-1;
				}
				if (new_id != cur_id) {
					XWarpPointer(display, None, None, 0, 0, 0, 0, dx, 0);

					screen->changeWorkspaceID(new_id);

					frame.resize_x = me->x_root+dx;
					if (!screen->doOpaqueMove()) {
						dx += frame.move_x; // for rectangle in correct position
					} else {
						dx += frame.x; // for window in correct position
					}
				}
			}


			if (! screen->doOpaqueMove()) {
				XDrawRectangle(display, screen->getRootWindow(), screen->getOpGC(),
						frame.move_x, frame.move_y, frame.resize_w, frame.resize_h);

				frame.move_x = dx;
				frame.move_y = dy;

				XDrawRectangle(display, screen->getRootWindow(), screen->getOpGC(),
					 frame.move_x, frame.move_y, frame.resize_w,
					 frame.resize_h);
			} else {
				configure(dx, dy, frame.width, frame.height);
			}

			if (screen->doShowWindowPos())
				screen->showPosition(dx, dy);
		} // end if moving
	} else if (functions.resize &&
			(((me->state & Button1Mask) && (me->window == frame.right_grip ||
			 me->window == frame.left_grip)) ||
			 me->window == frame.window)) {
		bool left = (me->window == frame.left_grip);

		if (! resizing) {			
			startResizing(me, left); 
		} else if (resizing) {
			XDrawRectangle(display, screen->getRootWindow(), screen->getOpGC(),
				frame.resize_x, frame.resize_y,
				frame.resize_w-1, frame.resize_h-1);

			int gx, gy;

			frame.resize_h = frame.height + (me->y - frame.grab_y);
			if (frame.resize_h < 1)
				frame.resize_h = 1;

			if (left) {
				frame.resize_x = me->x_root - frame.grab_x;
				if (frame.resize_x > (signed) (frame.x + frame.width))
					frame.resize_x = frame.resize_x + frame.width - 1;

				left_fixsize(&gx, &gy);
			} else {
				frame.resize_w = frame.width + (me->x - frame.grab_x);
				if (frame.resize_w < 1)
					frame.resize_w = 1;

				right_fixsize(&gx, &gy);
			}

			XDrawRectangle(display, screen->getRootWindow(), screen->getOpGC(),
					frame.resize_x, frame.resize_y,
					frame.resize_w-1, frame.resize_h-1);

			if (screen->doShowWindowPos())
				screen->showGeometry(gx, gy);
		}
	}
}


#ifdef		SHAPE
void FluxboxWindow::shapeEvent(XShapeEvent *) {
	Fluxbox *fluxbox = Fluxbox::instance();
	
	if (fluxbox->hasShapeExtensions()) {
		if (frame.shaped) {			
			fluxbox->grab();
			if (! validateClient())
				return;
			XShapeCombineShape(display, frame.window, ShapeBounding,
				frame.mwm_border_w, frame.y_border +
				frame.mwm_border_w, client.window,
				ShapeBounding, ShapeSet);

			int num = 1;
			XRectangle xrect[2];
			xrect[0].x = xrect[0].y = 0;
			xrect[0].width = frame.width;
			xrect[0].height = frame.y_border;

			if (decorations.handle) {
				xrect[1].x = 0;
				xrect[1].y = frame.y_handle;
				xrect[1].width = frame.width;
				xrect[1].height = frame.handle_h + screen->getBorderWidth();
				num++;
			}

			XShapeCombineRectangles(display, frame.window, ShapeBounding, 0, 0,
					xrect, num, ShapeUnion, Unsorted);
			fluxbox->ungrab();
		}
	}
}
#endif // SHAPE

// TODO: functions should not be affected by decoration
void FluxboxWindow::setDecoration(Decoration decoration) {
	switch (decoration) {
	case DECOR_NONE:
		decorations.titlebar = decorations.border = decorations.handle =
		decorations.iconify = decorations.maximize =
			decorations.tab = false; //tab is also a decor
		decorations.menu = true; // menu is present
	//	functions.iconify = functions.maximize = true;
	//	functions.move = true;   // We need to move even without decor
	//	functions.resize = true; // We need to resize even without decor
		
	break;

	default:
	case DECOR_NORMAL:
		decorations.titlebar = decorations.border = decorations.handle =
		decorations.iconify = decorations.maximize =
			decorations.menu = true;
		functions.resize = functions.move = functions.iconify =
			functions.maximize = true;
		XMapSubwindows(display, frame.window);
		XMapWindow(display, frame.window);

	break;

	case DECOR_TINY:
		decorations.titlebar = decorations.iconify = decorations.menu =
			functions.move = functions.iconify = true;
		decorations.border = decorations.handle = decorations.maximize =
			functions.resize = functions.maximize = false;
		XMapSubwindows(display, frame.window);
		XMapWindow(display, frame.window);
	break;

	case DECOR_TOOL:
		decorations.titlebar = decorations.menu = functions.move = true;
			decorations.iconify = decorations.border = decorations.handle =
			decorations.maximize = functions.resize = functions.maximize =
			functions.iconify = false;
		decorate();
	break;
	}

	reconfigure();

}

void FluxboxWindow::toggleDecoration() {
	static bool decor = false;
	//don't toggle decor if the window is shaded
	if (isShaded())
		return;
	
	if (!decor) { //remove decorations
		setDecoration(DECOR_NONE); 
		decor = true;
	} else { //revert back to old decoration
		setDecoration(old_decoration);
		decor = false;
	}
}

bool FluxboxWindow::validateClient() {
	XSync(display, false);

	XEvent e;
	if (XCheckTypedWindowEvent(display, client.window, DestroyNotify, &e) ||
			XCheckTypedWindowEvent(display, client.window, UnmapNotify, &e)) {
		XPutBackEvent(display, &e);
		Fluxbox::instance()->ungrab();

		return false;
	}

	return true;
}

void FluxboxWindow::startMoving(Window win) {
	moving = true;
	Fluxbox *fluxbox = Fluxbox::instance();
	XGrabPointer(display, win, False, Button1MotionMask |
		ButtonReleaseMask, GrabModeAsync, GrabModeAsync,
		None, fluxbox->getMoveCursor(), CurrentTime);

	if (m_windowmenu.get() && m_windowmenu->isVisible())
		m_windowmenu->hide();

	fluxbox->maskWindowEvents(client.window, this);

	if (! screen->doOpaqueMove()) {
		fluxbox->grab();

		frame.move_x = frame.x;
		frame.move_y = frame.y;
		frame.move_ws = screen->getCurrentWorkspaceID();
		frame.resize_w = frame.width + screen->getBorderWidth2x();
		frame.resize_h = ((shaded) ? frame.title_h : frame.height) +
			screen->getBorderWidth2x();

		if (screen->doShowWindowPos())
			screen->showPosition(frame.x, frame.y);

		XDrawRectangle(display, screen->getRootWindow(), screen->getOpGC(),
			frame.move_x, frame.move_y,
			frame.resize_w, frame.resize_h);
	}
}

void FluxboxWindow::stopMoving() {
	moving = false;
	Fluxbox *fluxbox = Fluxbox::instance();
					
	fluxbox->maskWindowEvents(0, (FluxboxWindow *) 0);

	if (! screen->doOpaqueMove()) {
		XDrawRectangle(display, screen->getRootWindow(), screen->getOpGC(),
	 		 frame.move_x, frame.move_y, frame.resize_w,
		 	 frame.resize_h);

		configure(frame.move_x, frame.move_y, frame.width, frame.height);
		fluxbox->ungrab();
	} else
		configure(frame.x, frame.y, frame.width, frame.height);

	screen->hideGeometry();
	XUngrabPointer(display, CurrentTime);
	
	XSync(display, False); //make sure the redraw is made before we continue
}

void FluxboxWindow::pauseMoving() {
	screen->hideGeometry(); // otherwise our window gets raised above it
	if (screen->doOpaqueMove()) {
		return;
	}

	// remove old rectangle
	XDrawRectangle(display, screen->getRootWindow(), screen->getOpGC(),
			frame.move_x, frame.move_y, frame.resize_w, frame.resize_h);

	Fluxbox::instance()->ungrab();
	if (workspace_number != frame.move_ws) {
		// get it out of view
		frame.save_x = frame.x;
		frame.save_y = frame.y;
		frame.x = screen->getWidth()/2;
		frame.y = screen->getHeight()+screen->getTabHeight()+10; // +10 to be safe
		configure(frame.x, frame.y, frame.width, frame.height);
	}
}


void FluxboxWindow::resumeMoving() {
	if (screen->doOpaqueMove()) {
		setInputFocus();
		screen->raiseFocus();
		if (screen->doShowWindowPos())
			screen->showPosition(frame.x, frame.y);
		return;
	}

	if (workspace_number != frame.move_ws) {
		frame.x = frame.save_x;
		frame.y = frame.save_y;
	} else {
		// back on home workspace, display window
		configure(frame.x, frame.y, frame.width, frame.height);
	}
	Fluxbox::instance()->grab();
	setInputFocus();
	screen->raiseFocus();
	if (screen->doShowWindowPos())
		screen->showPosition(frame.move_x, frame.move_y);
	XSync(display, false);
	XDrawRectangle(display, screen->getRootWindow(), screen->getOpGC(),
		frame.move_x, frame.move_y, frame.resize_w,
		frame.resize_h);
}


void FluxboxWindow::startResizing(XMotionEvent *me, bool left) {
	resizing = true;
	Fluxbox *fluxbox = Fluxbox::instance();
	XGrabPointer(display, me->window, false, ButtonMotionMask |	ButtonReleaseMask, 
		GrabModeAsync, GrabModeAsync, None,
		((left) ? fluxbox->getLowerLeftAngleCursor() : fluxbox->getLowerRightAngleCursor()),
		CurrentTime);

	int gx, gy;
	frame.grab_x = me->x - screen->getBorderWidth();
	frame.grab_y = me->y - screen->getBorderWidth2x();
	frame.resize_x = frame.x;
	frame.resize_y = frame.y;
	frame.resize_w = frame.width + screen->getBorderWidth2x();
	frame.resize_h = frame.height + screen->getBorderWidth2x();

	if (left)
		left_fixsize(&gx, &gy);
	else
		right_fixsize(&gx, &gy);

	if (screen->doShowWindowPos())
		screen->showGeometry(gx, gy);

	XDrawRectangle(display, screen->getRootWindow(), screen->getOpGC(),
		frame.resize_x, frame.resize_y,
		frame.resize_w-1, frame.resize_h-1);
}

void FluxboxWindow::stopResizing(Window win) {
	resizing = false;
	
	XDrawRectangle(display, screen->getRootWindow(), screen->getOpGC(),
		frame.resize_x, frame.resize_y,
		frame.resize_w-1, frame.resize_h-1);

	screen->hideGeometry();

	if (win && win == frame.left_grip)
		left_fixsize();
	else
		right_fixsize();

	
	configure(frame.resize_x, frame.resize_y,
		frame.resize_w - screen->getBorderWidth2x(),
		frame.resize_h - screen->getBorderWidth2x());
	
	if (tab)
		tab->resize();
			
	Fluxbox::instance()->ungrab();
	XUngrabPointer(display, CurrentTime);
}

//finds and redraw the icon label
void FluxboxWindow::updateIcon() {
	if (Fluxbox::instance()->useIconBar()) {
		const IconBar *iconbar = 0;
		const IconBarObj *icon = 0;
		if ((iconbar = screen->getToolbar()->iconBar()) != 0) {
			if ((icon = iconbar->findIcon(this)) != 0)
				iconbar->draw(icon, icon->width());
		}
	}
}

void FluxboxWindow::createTitlebar() {

	frame.title = createChildWindow(frame.window); //create titlebar win
	if (decorations.titlebar) {	//have titlebar decorations?		
		Fluxbox *fb = Fluxbox::instance();
		fb->saveWindowSearch(frame.title, this);	//save titlebar win
		frame.label = createChildWindow(frame.title); //create label win in titlebar
		fb->saveWindowSearch(frame.label, this);	//save label win
	}
}

void FluxboxWindow::destroyTitlebar() {
	Fluxbox *fb = Fluxbox::instance();
	
	while ( !buttonlist.empty()) {	//destroy all buttons on titlebar
		fb->removeWindowSearch(buttonlist.back().win);	
		XDestroyWindow(display, buttonlist.back().win);
		buttonlist.pop_back();
	}
	
	if (frame.title) {
		if (frame.ftitle) {
			image_ctrl->removeImage(frame.ftitle);
			frame.ftitle = 0;
		}

		if (frame.utitle) {
			image_ctrl->removeImage(frame.utitle);
			frame.utitle = 0;
		}

		if (frame.flabel) {
			image_ctrl->removeImage(frame.flabel);
			frame.flabel = 0;
		}

		if( frame.ulabel) {
			image_ctrl->removeImage(frame.ulabel);
			frame.ulabel = 0;
		}

		fb->removeWindowSearch(frame.label);
		fb->removeWindowSearch(frame.title);

		XDestroyWindow(display, frame.label);
		frame.label = 0;
		XDestroyWindow(display, frame.title);
		frame.title = 0;
	}

	
}

void FluxboxWindow::createHandle() {

	if (!decorations.handle)
		return;
		
	Fluxbox *fluxbox = Fluxbox::instance();
	frame.handle = createChildWindow(frame.window); //create handle win
	fluxbox->saveWindowSearch(frame.handle, this);	//save handle win

	frame.left_grip = // create left handle
		createChildWindow(frame.handle, fluxbox->getLowerLeftAngleCursor());
	fluxbox->saveWindowSearch(frame.left_grip, this); //save left handle

	frame.right_grip = // create right handle
		createChildWindow(frame.handle, fluxbox->getLowerRightAngleCursor());
	fluxbox->saveWindowSearch(frame.right_grip, this); //save right handle
	
}

void FluxboxWindow::destroyHandle() {
	if (frame.fhandle) {
		image_ctrl->removeImage(frame.fhandle);
		frame.fhandle = 0;
	}

	if (frame.uhandle) {
		image_ctrl->removeImage(frame.uhandle);
		frame.uhandle = 0;
	}

	if (frame.fgrip) {
		image_ctrl->removeImage(frame.fgrip);
		frame.fgrip = 0;
	}

	if (frame.ugrip) {
		image_ctrl->removeImage(frame.ugrip);
		frame.ugrip = 0;
	}
	Fluxbox *fluxbox = Fluxbox::instance();

	if (frame.right_grip != 0) {
		fluxbox->removeWindowSearch(frame.right_grip);
		XDestroyWindow(display, frame.right_grip);
		frame.right_grip = 0;
	}
	
	if (frame.left_grip != 0) {
		fluxbox->removeWindowSearch(frame.left_grip);
		XDestroyWindow(display, frame.left_grip);
		frame.left_grip = 0;	
	}
	
	if (frame.handle != 0) {
		fluxbox->removeWindowSearch(frame.handle);
		XDestroyWindow(display, frame.handle);
		frame.handle = 0;
	}

}

void FluxboxWindow::checkTransient() {
	// remove us from parent
	if (client.transient_for != 0) {
		client.transient_for->client.transients.remove(this);
	}
	client.transient_for = 0;
	
	// determine if this is a transient window
	Window win;
	if (!XGetTransientForHint(display, client.window, &win))
		return;
	
	if (win == client.window)
		return;
	
	if (win == screen->getRootWindow() && win != 0) {
		modal = true;
		return;
	}
	
	client.transient_for = Fluxbox::instance()->searchWindow(win);
	if (client.transient_for != 0 &&
		client.window_group != None && win == client.window_group) {		
		client.transient_for = Fluxbox::instance()->searchGroup(win, this);
	}
	
	// make sure we don't have deadlock loop in transient chain
	for (FluxboxWindow *w = this; w != 0; w = w->client.transient_for) {
		if (w == w->client.transient_for) {
			w->client.transient_for = 0;
#ifdef DEBUG	
			cerr<<"w = client.transient_for";
#endif // DEBUG
			break;
		}
	}
	
	if (client.transient_for != 0) {
		client.transient_for->client.transients.push_back(this);
		// make sure we only have on instance of this
		client.transient_for->client.transients.unique(); 
		stuck = client.transient_for->stuck;
	}
	
}

void FluxboxWindow::restore(bool remap) {
	XChangeSaveSet(display, client.window, SetModeDelete);
	XSelectInput(display, client.window, NoEventMask);

	restoreGravity();

	XUnmapWindow(display, frame.window);
	XUnmapWindow(display, client.window);

	XSetWindowBorderWidth(display, client.window, client.old_bw);
		XEvent dummy;
	if (! XCheckTypedWindowEvent(display, client.window, ReparentNotify,
			 &dummy)) {
#ifdef DEBUG
		fprintf(stderr,
			I18n::instance()->getMessage(
				 FBNLS::WindowSet, FBNLS::WindowUnmapNotifyReparent,		
				 "FluxboxWindow::restore: reparent 0x%lx to "
				 "root.\n"), client.window);
#endif // DEBUG
		XReparentWindow(display, client.window, screen->getRootWindow(),
			client.x, client.y);
	}

	if (remap)
		XMapWindow(display, client.window);

}


void FluxboxWindow::timeout() {
	if (tab)
		tab->raise();
	screen->getWorkspace(workspace_number)->raiseWindow(this);
}


void FluxboxWindow::changeBlackboxHints(BaseDisplay::BlackboxHints *net) {
	if ((net->flags & BaseDisplay::ATTRIB_SHADED) &&
			((blackbox_attrib.attrib & BaseDisplay::ATTRIB_SHADED) !=
			(net->attrib & BaseDisplay::ATTRIB_SHADED)))
		shade();

	if ((net->flags & (BaseDisplay::ATTRIB_MAXVERT | BaseDisplay::ATTRIB_MAXHORIZ)) &&
			((blackbox_attrib.attrib & (BaseDisplay::ATTRIB_MAXVERT | BaseDisplay::ATTRIB_MAXHORIZ)) !=
				(net->attrib & (BaseDisplay::ATTRIB_MAXVERT | BaseDisplay::ATTRIB_MAXHORIZ)))) {
		if (maximized) {
			maximize(0);
		} else {
			int m = 0;

			if ((net->flags & BaseDisplay::ATTRIB_MAXHORIZ) && (net->flags & BaseDisplay::ATTRIB_MAXVERT))
				m = ((net->attrib & (BaseDisplay::ATTRIB_MAXHORIZ | BaseDisplay::ATTRIB_MAXVERT)) ?	1 : 0);
			else if (net->flags & BaseDisplay::ATTRIB_MAXVERT)
				m = ((net->attrib & BaseDisplay::ATTRIB_MAXVERT) ? 2 : 0);
			else if (net->flags & BaseDisplay::ATTRIB_MAXHORIZ)
				m = ((net->attrib & BaseDisplay::ATTRIB_MAXHORIZ) ? 3 : 0);

			maximize(m);
		}
	}

	if ((net->flags & BaseDisplay::ATTRIB_OMNIPRESENT) &&
			((blackbox_attrib.attrib & BaseDisplay::ATTRIB_OMNIPRESENT) !=
			(net->attrib & BaseDisplay::ATTRIB_OMNIPRESENT)))
		stick();

	if ((net->flags & BaseDisplay::ATTRIB_WORKSPACE) &&
			(workspace_number !=  net->workspace)) {

		if (getTab()) // disconnect from tab chain before sending it to another workspace
			getTab()->disconnect();

		screen->reassociateWindow(this, net->workspace, true);

		if (screen->getCurrentWorkspaceID() != net->workspace)
			withdraw();
		else 
			deiconify();
	}

	if (net->flags & BaseDisplay::ATTRIB_DECORATION) {
		old_decoration = static_cast<Decoration>(net->decoration);
		setDecoration(old_decoration);
	}

}


void FluxboxWindow::upsize() {
	// convert client.width/height into frame sizes

	frame.bevel_w = screen->getBevelWidth();
	frame.mwm_border_w = screen->getFrameWidth() * decorations.border;
	
	frame.title_h = screen->getWindowStyle()->font.height() + 
		(frame.bevel_w*2 + 2)*decorations.titlebar;

	frame.label_h = (frame.title_h - (frame.bevel_w * 2)) * decorations.titlebar;
	frame.button_w = frame.button_h = frame.label_h - 2;

	frame.y_border = (frame.title_h + screen->getBorderWidth()) * decorations.titlebar;
	frame.border_h = client.height + (frame.mwm_border_w * 2);

	frame.y_handle = frame.y_border + frame.border_h +
							(screen->getBorderWidth() * decorations.handle);
	frame.handle_h = decorations.handle * screen->getHandleWidth();

	frame.grip_w = (frame.button_h * 2) * decorations.handle;
	frame.grip_h = screen->getHandleWidth() * decorations.handle;

	frame.width = client.width + (frame.mwm_border_w * 2);
	frame.height = frame.y_handle + frame.handle_h;

	frame.snap_w = frame.width + screen->getBorderWidth2x();
	frame.snap_h = frame.height + screen->getBorderWidth2x();
}


void FluxboxWindow::downsize() {
	// convert frame.width/height into client sizes

	frame.y_handle = frame.height - frame.handle_h;
	frame.border_h = frame.y_handle - frame.y_border -
							(screen->getBorderWidth() * decorations.handle);

	client.x = frame.x + frame.mwm_border_w + screen->getBorderWidth();
	client.y = frame.y + frame.y_border + frame.mwm_border_w +
							screen->getBorderWidth();

	client.width = frame.width - (frame.mwm_border_w * 2);
	client.height = frame.height - frame.y_border -
						(frame.mwm_border_w * 2) - frame.handle_h -
						(screen->getBorderWidth() * decorations.handle);

	frame.y_handle = frame.border_h + frame.y_border + screen->getBorderWidth();

	frame.snap_w = frame.width + screen->getBorderWidth2x();
	frame.snap_h = frame.height + screen->getBorderWidth2x();
}


void FluxboxWindow::right_fixsize(int *gx, int *gy) {
	// calculate the size of the client window and conform it to the
	// size specified by the size hints of the client window...
	int dx = frame.resize_w - client.base_width - (frame.mwm_border_w * 2) -
		screen->getBorderWidth2x();
	int dy = frame.resize_h - frame.y_border - client.base_height -
		frame.handle_h - (screen->getBorderWidth() * (2+decorations.border)) - (frame.mwm_border_w * 2);

	if (dx < (signed) client.min_width)
		dx = client.min_width;
	if (dy < (signed) client.min_height)
		dy = client.min_height;
	if (client.max_width > 0 && (unsigned) dx > client.max_width)
		dx = client.max_width;
	if (client.max_height > 0 && (unsigned) dy > client.max_height)
		dy = client.max_height;

	dx /= client.width_inc;
	dy /= client.height_inc;

	if (gx) *gx = dx;
	if (gy) *gy = dy;

	dx = (dx * client.width_inc) + client.base_width;
	dy = (dy * client.height_inc) + client.base_height;

	frame.resize_w = dx + (frame.mwm_border_w * 2) + screen->getBorderWidth2x();
	frame.resize_h = dy + frame.y_border + frame.handle_h +
			(frame.mwm_border_w * 2) +	(screen->getBorderWidth() *
			(2+decorations.border));
}


void FluxboxWindow::left_fixsize(int *gx, int *gy) {
	// calculate the size of the client window and conform it to the
	// size specified by the size hints of the client window...
	int dx = frame.x + frame.width - frame.resize_x - client.base_width -
		(frame.mwm_border_w * 2);
	int dy = frame.resize_h - frame.y_border - client.base_height -
		frame.handle_h - (screen->getBorderWidth() * (2+decorations.border)) - (frame.mwm_border_w * 2);

	if (dx < (signed) client.min_width) dx = client.min_width;
	if (dy < (signed) client.min_height) dy = client.min_height;
	if (client.max_width > 0 && (unsigned) dx > client.max_width) dx = client.max_width;
	if (client.max_height > 0 && (unsigned) dy > client.max_height) dy = client.max_height;

	dx /= client.width_inc;
	dy /= client.height_inc;

	if (gx) *gx = dx;
	if (gy) *gy = dy;

	dx = (dx * client.width_inc) + client.base_width;
	dy = (dy * client.height_inc) + client.base_height;

	frame.resize_w = dx + (frame.mwm_border_w * 2) + screen->getBorderWidth2x();
	frame.resize_x = frame.x + frame.width - frame.resize_w +
			 screen->getBorderWidth2x();
	frame.resize_h = dy + frame.y_border + frame.handle_h +
			 (frame.mwm_border_w * 2) + (screen->getBorderWidth() *
			 (2+decorations.border));
}
